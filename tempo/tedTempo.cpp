// tedTempo.cpp
//
// Copyright (C) 2014-2019 Grigore Burloiu
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef TED

#include "tempo.h"

TempoModel::TempoModel(oDTW *warp_) :
    warp(warp_)
{
	beat = new BTrack();
    sensitivity = SEN; // sens. to tempo fluctuations
    elasticity = ELA;
	start();
}


TempoModel::~TempoModel() {
}

// ====== public methods ==========

void TempoModel::start() {
	h_real = 0;
	tempo = tempo_avg = 1;
	tempotempo = 0;

	elast_beat = 1;
	beat_due = false;
	acc_iter = b_iter = prev_h_beat = 0;
	ref_tempo = 120;
	integral = t_passed = last_arzt = 0;
	minerr = 0;
	tempos.clear(); errors.clear();
	tempo_mode = OFF;
	tempo_model = T_PID;

	pivot1_t = pivot1_h = pivot2_t = pivot2_h = 0;
	pivot1_tp = pivot2_tp = 1;

	y_beats.clear();
	y_beats.resize(2);
	acc_beats.clear();
	acc_beats.resize(2);

	beat->updateHopAndFrameSize(HOP_SIZE, HOP_SIZE);
}

void TempoModel::setSensitivity(float sen) {
    sensitivity = sen;
}

void TempoModel::setElasticity(float ela) {
    elasticity = ela;
}

double TempoModel::getHreal() {
	return h_real;
}

void TempoModel::setHreal(unsigned int to_h) {
	h_real = to_h;
}

double TempoModel::getTempo() {
	return tempo;
}

short TempoModel::getTempoMode() {
	return tempo_mode;
}

void TempoModel::addBeat(unsigned int pos, double tempo, short dest) {
	if (dest == BUF_ACCO) {
		acc_beats[A_BEATPOS].push_back(pos);
		acc_beats[A_TEMPO].push_back(tempo);
	}
	else if (dest == BUF_MAIN) {
		y_beats[Y_BEATPOS].push_back(pos);  // beat pos
		y_beats[Y_DIFF].push_back(0);	// diff from acco beat (computed later)
	}
}

unsigned int TempoModel::getAccBeats() {
	return acc_beats[A_BEATPOS].size();
}

unsigned int TempoModel::getYBeats() {
	return y_beats[Y_BEATPOS].size();
}

void TempoModel::clearYBeats() {
	y_beats.clear();
	y_beats.resize(2);
}

double TempoModel::beatAt(unsigned int pos, unsigned int beat_index, short dest) {
	if (dest == BUF_MAIN) {
		if (y_beats[Y_BEATPOS][beat_index] == pos) return 1;
	}
	else if (dest == BUF_ACCO) {
		if (acc_beats[A_BEATPOS][beat_index] == pos) return acc_beats[A_TEMPO][beat_index];
	}
	return 0;
}

bool TempoModel::performBeat(double *in, short source, short dest, unsigned int frame_index) {
	beat->processAudioFrame(in);

	if (beat->beatDueInCurrentFrame()) { // beat detected
		
		switch (dest) {
		case (BUF_MAIN):
			if (source != IN_LIVE && frame_index) { 
				// looking at reference
				y_beats[Y_BEATPOS].push_back(frame_index);  // beat pos
				y_beats[Y_DIFF].push_back(0);	// diff from acco beat (computed at the end, once the whole score is loaded)
			}
			else if (y_beats[Y_BEATPOS].size() && h_real) { 
				// looking at live target
				float diff_y, diff_acc;
				// check if live beat aligns with predicted beats
				b_iter = update_beat_iter(b_iter, &y_beats[0], h_real);
				diff_y = calc_beat_diff(h_real, prev_h_beat, y_beats[0][b_iter]);
				if (acc_beats[A_BEATPOS].size()) {
					acc_iter = update_beat_iter(acc_iter, &acc_beats[A_BEATPOS], h_real);
					diff_acc = calc_beat_diff(h_real, prev_h_beat, acc_beats[A_BEATPOS][acc_iter]);

					ref_tempo = acc_beats[A_TEMPO][acc_iter];

					// average the last 3 diffs between Y and ACCO beats
					minerr = 0;
					int i = 0;
					if (b_iter > 3)
						i = b_iter - 3;
					for (; i <= b_iter; i++) {
						minerr += y_beats[1][b_iter];
					}
					minerr /= 3;
				}
				else
					diff_acc = VERY_BIG;

				prev_h_beat = h_real;
				beat_due = true;
			}
			break;

		case (BUF_ACCO): // looking at accompaniment
			if (acc_iter) {
				addBeat(acc_iter, beat->getCurrentTempoEstimate(), BUF_ACCO);
			}
			//post("tempo %f at beat %d", beat->getCurrentTempoEstimate(), acc_iter);
			break;
		}
		return true; // beat, work done
	}
	return false; // no beat, did nothing
}

void TempoModel::incrementAccIter() {
	acc_iter++;
}

float TempoModel::computeYAccbeatDiffs() {
	// compare Y beats & ACC beats
	b_iter = acc_iter = 0;
	float diff = 0;

	while (b_iter < y_beats[0].size() && acc_iter < acc_beats[A_BEATPOS].size()) {
		b_iter = update_beat_iter(b_iter, &y_beats[0], int(acc_beats[A_BEATPOS][acc_iter]));
		if (b_iter)
			y_beats[1][b_iter] = calc_beat_diff(y_beats[Y_BEATPOS][b_iter],
				y_beats[Y_BEATPOS][b_iter - 1], acc_beats[A_BEATPOS][acc_iter]);
		else
			y_beats[1][b_iter] = y_beats[Y_BEATPOS][b_iter] - acc_beats[A_BEATPOS][acc_iter];
		diff = (float)(y_beats[Y_DIFF][b_iter] + acc_iter * diff) / (acc_iter + 1); // CMA
		acc_iter++;
	}

	b_stdev = minerr = 0;
	for (int i = 0; i < b_iter; i++)
		b_stdev += (diff - y_beats[Y_DIFF][i]) * (diff - y_beats[Y_DIFF][i]);
	b_stdev /= y_beats[Y_BEATPOS].size();
	b_stdev = sqrt(b_stdev);
	//post("std deviation between Y and ACC beats: %f", b_stdev);

	// return mean diff between beats
	return diff;
}

void TempoModel::computeTempo() {
	unsigned int t = warp->getT();
	unsigned int h = warp->getH();
	error = (h - h_real);

	double beat_tempo = calc_beat_tempo();
	int beat_length = (acc_iter) ? acc_beats[A_BEATPOS][acc_iter] - acc_beats[A_BEATPOS][acc_iter - 1] : warp->getFsize();

	static int waiting = 0;
	unsigned int bsize = warp->getBsize();
	int step = bsize / 8;
	unsigned int b_start = t % bsize;
	b_err = warp->getBackPath();

	// sensitivity to tempo fluctuations:
	if (abs(error) > pow(1.f - sensitivity, 2)*100.f + abs(minerr)) {
		if (abs(b_err[b_start][B_ERROR]) > 15 && abs(tempo_avg - beat_tempo) > 0.15 && beat_due) {
			// DTW is way off, beat tracker takes over
			//			post("oDTW is off, BT on! %f != %f. beat length = %d", tempo_avg, beat_tempo, beat_length);
			waiting = beat_length;
		}
		if (waiting) {
			tempo = beat_tempo;
			tempo_mode = BEAT;
		}
		else {
			// done, move back from BEAT tracker to oDTW tracker
			if (tempo_mode == BEAT) {
				h_real = h;
				//				h_real += tempo;
				//if (DEBUG) post("moved H_real to H");
			}
			tempo = calc_tempo(tempo_model);
			tempo_mode = DTW;
		}
	}
	else if (abs(error) <= 1 && tempo_mode && t > step) {
		//        post("disengage");
		tempo = b_err[(b_start - step + bsize) % bsize][B_TEMPO];
		tempo_mode = OFF;
	}

	if (waiting > 0) waiting--;

	// update h_real
	if (tempo > 0.01) {
		if (tempo > 3 || tempo < 0.33) {
			//if (DEBUG) post("OUT OF CONTROL tempo = %f, mode %d", tempo, tempo_mode);
			tempo = beat_tempo;
			tempo_mode = BEAT;
			waiting = (int)(warp->getFsize() / beat_length) * beat_length;
		}
		h_real += tempo;
	}

	// compute smoothed beat-based tempo
	if (t > step * 2) {
		tempo_avg += b_err[(b_start - step + bsize) % bsize][B_TEMPO] / step;
		tempo_avg -= b_err[(b_start - (step + 8) + bsize) % bsize][B_TEMPO] / step;
	}
}

// ====== internal methods ==========

double TempoModel::calc_tempo(int mode) {
    unsigned int t = warp->getT();
    if (t == 0) return 1;
    
    unsigned int h = warp->getH();
    
    int second = 86; // number of frames per second (44100/512)
    b_err = warp->getBackPath();
    unsigned int bsize = warp->getBsize();
    unsigned int b_start = t % bsize;
    int step = bsize / 40;
    
    static double last_tempo = 1;
    double new_tempo = tempo;
    static unsigned int last_beat = 1;
    static double old_tempo = 1; // for ARZT model
    // for DEQ model
    static double Min = VERY_BIG, Sum;
    bool got_new_tempo = false;
    int K = second, L = second / 2;
    unsigned int counter = 0;
    
    switch(mode) {
        case T_DTW:
            if (t <= second)
                new_tempo = 1;
            else {
                if(beat_due) {
                    // updated every beat
 //                   if (DEBUG) post("update tempo, beat ela: %f", elast_beat);
                    new_tempo = (double)(warp->getHistory(t) - warp->getHistory(last_beat)) / (t - last_beat);
                    last_beat = t;
                }
            }
            break;
        case T_P:
            if (t <= second)
                new_tempo = 1;
            else {
                if (beat_due) {
                    // tempo model in Papiotis10, updated every beat
   //                 if(elast_beat == 1) {if (DEBUG) post("update tempo 1");}
   //                 else post ("%.2f", elast_beat);
                    new_tempo = (double)(warp->getHistory(t) - warp->getHistory(last_beat)) / (t - last_beat);
                    double boost = error / ((double)(t - last_beat)*10);
                    new_tempo += boost;
                    last_beat = t;
                }
            }
            break;
        case T_PID:
            if (t <= step)
                new_tempo = 1;
            else {
                errors.push_back(error);
                if(errors.size() > step)
                    errors.pop_front();
                if (abs(error) < 20.f) //anti-windup
                    integral += error;
                if (!(t%step)) {
                    // PID tempo model
                    new_tempo = (double)(warp->getHistory(t) - warp->getHistory(t-step)) / step;
                    float derivate = (error - errors[0]) / step;
                    //post("err = %f // int = %f // der = %f", Kp*error, Ki*integral, Kd*derivate);
                    double boost = (Kp*error + Ki*integral + Kd * derivate) / ((double)(t - last_beat));
                    new_tempo += boost;
//                    if (DEBUG) post("new tempo %f, hop size %d, b # %d", new_tempo, beat->getHopSize(), t/step);
                    last_beat = t;
                }
            }
            break;
        case T_DEQ:
            // get tempo pivots from history via deque: http://www.infoarena.ro/deque-si-aplicatii
            // first pivot is within L steps from path origin
            // 2nd pivot is at least K steps away from 1st, AND minimizes SUM(var_tempo[pivots])
            if (t < bsize || h < bsize) {
                new_tempo = 1;//warp->getHistory(t) - warp->getHistory(t-1); // not yet active
                //                post("EARLY");
            }
            else {
                pivot1_t = pivot1_h = pivot2_t = pivot2_h = 0;
                pivot1_tp = pivot2_tp = 1;
                Deque.clear();
                int j = 0;
                for (int i = (b_start-1+bsize)%bsize;
                     j < bsize - K - 2;
                     i = (i-1+bsize)%bsize) { // for i++
                    if (j < L) { // if i < L
                        bool popped = false;
                        if (!Deque.empty()) {
                            float derr_i = 0;    // v_t1
                            float derr_q = 0;    // v_t2
                            for (int it = 0; it < step; it++) { // summing tempo deviations
                                derr_i += abs(b_err[(i+it - step + bsize) %bsize][B_TEMPO] -
                                              b_err[(i+it+1-step + bsize) %bsize][B_TEMPO]);
                                derr_q += abs(b_err[(Deque.front() - it+bsize)%bsize][B_TEMPO] -
                                              b_err[(Deque.front()-1-it+bsize)%bsize][B_TEMPO]);
                            }
                            while (!Deque.empty() && derr_i < derr_q) {
                                Deque.pop_front();
                                popped = true;
                            }
                        }
                        if (popped || Deque.empty()) Deque.push_front((i-step+bsize)%bsize);
                    }
                    if (!Deque.empty()) {
                        unsigned int iplusK = (i-step-K+bsize)%bsize;
                        float derr_ipK = 0;
                        float derr_q = 0;
                        for (int it = 0; it < step; it++) { // summing tempo deviations
                            derr_ipK += abs(b_err[(iplusK- it  + bsize)%bsize][B_TEMPO] -
                                            b_err[(iplusK-it-i + bsize)%bsize][B_TEMPO]);
                            derr_q   +=    abs(b_err[(Deque.front()- it  + bsize)%bsize][B_TEMPO] -
                                               b_err[(Deque.front()-it-1 +    bsize)%bsize][B_TEMPO]);
                        }
                        Sum = derr_q + derr_ipK;
                        //                        post("Sum is %f, Min is %f", Sum, Min);
                        if (Sum <= Min) {
                            if (Sum < Min) {
                                // if found new Min
                                counter = 0;
                                Min = Sum;
                                pivot1_t = b_err[Deque.front()][B_T];
                                pivot1_h = b_err[Deque.front()][B_H];
                                pivot1_tp = b_err[Deque.front()][B_TEMPO];
                                pivot2_t = b_err[iplusK][B_T];
                                pivot2_h = b_err[iplusK][B_H];
                                pivot2_tp = b_err[iplusK][B_TEMPO];
                                got_new_tempo = true;
                            } else {
                                if (pivot1_t) counter++;
                                pivot1_t = (b_err[Deque.front()][B_T] + counter * pivot1_t) / (counter + 1); // CMA
                                pivot1_h = (b_err[Deque.front()][B_H] + counter * pivot1_h) / (counter + 1); // CMA
                                pivot1_tp = (b_err[Deque.front()][B_TEMPO] + counter * pivot1_tp) / (counter + 1); // CMA
                                pivot2_t = (b_err[iplusK][B_T] + counter * pivot2_t) / (counter + 1); // CMA
                                pivot2_h = (b_err[iplusK][B_H] + counter * pivot2_h) / (counter + 1); // CMA
                                pivot2_tp = (b_err[iplusK][B_TEMPO] + counter * pivot2_tp) / (counter + 1); // CMA
                                got_new_tempo = true;
                            }
                        }
                    }
                    j++;
                }
                if (got_new_tempo && pivot1_t != pivot2_t) {
                    // compute new tempo, avoiding divide by 0
                    new_tempo = (pivot1_h - pivot2_h) / (pivot1_t - pivot2_t);
                    tempotempo = (pivot1_tp - pivot2_tp);
                    t_passed = pivot1_t;
                    //post("pivots %f : %f, Min=%f tt=%f", pivot1_t, pivot2_t, Min, tempotempo);
                }
                // correct err
                if (error > 2) {
                    //post("+ %f", abs(tempotempo) + 0.0001);
                    new_tempo += abs(tempotempo) + 0.0001;
                } else if (error < -2) {
                    //post("- %f", abs(tempotempo) + 0.0001);
                    new_tempo -= abs(tempotempo) + 0.0001;
                }
            }
            break;
        case T_ARZT:
            if (t < bsize || h < bsize)
                new_tempo = 1;//history[t] - history[t-1]; // not yet active
            else {
                int i = (b_start-step+bsize)%bsize, j = 0;
                float derr_i = 0;
                for (int it = 0; it < step; it++) { // summing tempo deviations
                    derr_i += abs(b_err[(i+it - step + bsize) %bsize][B_TEMPO] -
                                  b_err[(i+it+1-step + bsize) %bsize][B_TEMPO]);
                }
                while (j < 2*K && b_err[i][1] > t_passed + 4) {
                    if(derr_i == 0) {
                        got_new_tempo = true;
                        t_passed = b_err[i][B_T];
                        last_arzt = b_err[i][B_TEMPO];
                        if (2*K - j > 8) { // spread them out
                            j += 4;
                            i = (i-4+bsize)%bsize;
                        }
                    }
                    j++;
                    i = (i-1+bsize)%bsize;
                }
                if (got_new_tempo) {
                    tempos.push_back(last_arzt);
                    //post("Added tempo %f, total %d tempos, t = %d", last_arzt, tempos.size(), t);
                    if (tempos.size() == 6)
                        tempos.pop_front();
                    if (tempos.size() > 3) {
                        new_tempo = i = 0;
                        for (deque<double>::iterator it = tempos.begin(); it!=tempos.end(); ++it) {
                            new_tempo += *it * (i+1);    // sum of tempos[i]*(n-i)
                            i++;
                        }
                        new_tempo /= (tempos.size() * (tempos.size()+1) / 2); // sum of 1..n
                        //post("mean = %f", tempo);
                        old_tempo = new_tempo;
                        t_passed = t;
                    }
                }
                // add PID ajustment:
                if (abs(error) < 30.f) //anti-windup
                    integral += error;
                float boost = (Kp*error + Ki*integral) / (10 * (abs(old_tempo - new_tempo) * (t - t_passed)) +1);
                if (abs(boost) <= abs(new_tempo - old_tempo))
                    new_tempo += boost;
            }
            break;
    }
    if (new_tempo != last_tempo) {
        new_tempo = last_tempo + (new_tempo - last_tempo) * elasticity * elast_beat;
        last_tempo = new_tempo;
    }
    beat_due = false;
    return new_tempo;
}

int TempoModel::calc_beat_diff(double cur_beat, double prev_beat, double ref_beat) {
	// helper: returns # of frames between current live beat and its corresponding reference/score beat
	int newdiff = cur_beat - ref_beat;
	if (abs(newdiff) > (cur_beat - prev_beat) / 4) { // reverse phase
		if (cur_beat < ref_beat)
			newdiff = cur_beat + (cur_beat - prev_beat) / 2 - ref_beat;
		else
			newdiff = cur_beat - (cur_beat - prev_beat) / 2 - ref_beat;
	}
	return newdiff;
}

unsigned int TempoModel::update_beat_iter(unsigned int beat_index, vector<float> *beat_vector, double ref_beat) {
	// helper: finds closest beat index to the "ref_beat" coordinate
	if (beat_index < beat_vector->size() - 1) {
		// while the next beat is closer than the current one, increment iterator
		while ((beat_index < beat_vector->size() - 1) &&
			abs(ref_beat - beat_vector->at(beat_index)) > abs(ref_beat - beat_vector->at(beat_index + 1)))
			beat_index++;
	}
	return beat_index;
}

double TempoModel::calc_beat_tempo() {
	return (beat->getCurrentTempoEstimate() / ref_tempo);
}

#endif
