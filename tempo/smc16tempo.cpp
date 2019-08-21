// smc16tempo.cpp
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

#include "tempo.h"

TempoModel::TempoModel(oDTW *warp_) :
    warp(warp_)
{
    sensitivity = SEN; // sens. to tempo fluctuations
    elasticity = ELA;
    
    tempo = tempo_avg = 1;
    tempotempo = 0;
    
    elast_beat = 1;
    beat_due = false;
    acc_iter = b_iter = prev_h_beat = 0;
    ref_tempo = 120;
    integral = t_passed = last_arzt = 0;
    tempos.clear(); errors.clear();
    
    y_beats.clear();
    y_beats.resize(2);
    acc_beats.clear();
    acc_beats.resize(2);
}


TempoModel::~TempoModel() {
}

// ====== public methods ==========

void TempoModel::setSensitivity(float sen) {
    sensitivity = sen;
}

void TempoModel::setElasticity(float ela) {
    elasticity = ela;
}

// ====== internal methods ==========

double TempoModel::calc_tempo(int mode) {
    unsigned int t = warp->getT();
    if (t == 0) return 1;
    
    unsigned int h = warp->getH();
    
    int second = 86; // number of frames per second (44100/512)
    vector<vector<double> > b_err = warp->getBackPath();
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
                                derr_i += abs(b_err[(i+it - step + bsize) %bsize][3] -
                                              b_err[(i+it+1-step + bsize) %bsize][3]);
                                derr_q += abs(b_err[(Deque.front() - it+bsize)%bsize][3] -
                                              b_err[(Deque.front()-1-it+bsize)%bsize][3]);
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
                            derr_ipK += abs(b_err[(iplusK- it  + bsize)%bsize][3] -
                                            b_err[(iplusK-it-i + bsize)%bsize][3]);
                            derr_q   +=    abs(b_err[(Deque.front()- it  + bsize)%bsize][3] -
                                               b_err[(Deque.front()-it-1 +    bsize)%bsize][3]);
                        }
                        Sum = derr_q + derr_ipK;
                        //                        post("Sum is %f, Min is %f", Sum, Min);
                        if (Sum <= Min) {
                            if (Sum < Min) {
                                // if found new Min
                                counter = 0;
                                Min = Sum;
                                pivot1_t = b_err[Deque.front()][1];
                                pivot1_h = b_err[Deque.front()][2];
                                pivot1_tp = b_err[Deque.front()][3];
                                pivot2_t = b_err[iplusK][1];
                                pivot2_h = b_err[iplusK][2];
                                pivot2_tp = b_err[iplusK][3];
                                got_new_tempo = true;
                            } else {
                                if (pivot1_t) counter++;
                                pivot1_t = (b_err[Deque.front()][1] + counter * pivot1_t) / (counter + 1); // CMA
                                pivot1_h = (b_err[Deque.front()][2] + counter * pivot1_h) / (counter + 1); // CMA
                                pivot1_tp = (b_err[Deque.front()][3] + counter * pivot1_tp) / (counter + 1); // CMA
                                pivot2_t = (b_err[iplusK][1] + counter * pivot2_t) / (counter + 1); // CMA
                                pivot2_h = (b_err[iplusK][2] + counter * pivot2_h) / (counter + 1); // CMA
                                pivot2_tp = (b_err[iplusK][3] + counter * pivot2_tp) / (counter + 1); // CMA
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
                    derr_i += abs(b_err[(i+it - step + bsize) %bsize][3] -
                                  b_err[(i+it+1-step + bsize) %bsize][3]);
                }
                while (j < 2*K && b_err[i][1] > t_passed + 4) {
                    if(derr_i == 0) {
                        got_new_tempo = true;
                        t_passed = b_err[i][1];
                        last_arzt = b_err[i][3];
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
