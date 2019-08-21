// tempo.h
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

#pragma once

#include "defines.h"
#include "oDTW.h"
#include "BTrack.h"
#include <vector>
#include <deque>

using namespace std;

class TempoModel
{
public:
	// constructor & destructor
	TempoModel(oDTW *warp_);
	~TempoModel();

	// reset all vars
	void start();

	// set Sensitivity to tempo fluctuations [0...1]
    void setSensitivity(float sen);
    
    // set Elasticity of tempo model [0...1...]
    void setElasticity(float ela);

	// get "real" score coordinate, produced by applying the TempoModel to the oDTW path
	double getHreal();

	// set new score time. Use with caution - causes the system to effectively jump in time!
	void setHreal(unsigned int to_h);

	// get tempo
	double getTempo();

	// get tempo mode (OFF / oDTW / BEAT)
	short getTempoMode();

	// add an y / acco beat to a specific frame pos
	void addBeat(unsigned int pos, double tempo, short dest);

	// get total number of acco beats
	unsigned int getAccBeats();

	// get total number of Y reference beats
	unsigned int getYBeats();

	// reset y_beats array
	void clearYBeats();

	// return tempo if beat #beat_index found at position pos, or zero if not
	double beatAt(unsigned int pos, unsigned int beat_index, short dest);

	// perform beat tracking. Returns TRUE if beat in current *in frame, FALSE otherwise
	// in: (audio) input frame
	// source: input select (live / reference, see defines.h)
	// dest: buffer target (main / acco, see defines.h)
	// frame_index: frames elapsed in selected buffer
	bool performBeat(double *in, short source, short dest, unsigned int frame_index);

	void incrementAccIter();

	// compute frame differences between Y and Acco. Returns overall average diff.
	float computeYAccbeatDiffs();

	void computeTempo();
    
private:
	// internal vars
    oDTW *warp;
	BTrack *beat;

    deque<unsigned int> Deque;
    double tempo, tempotempo, tempo_avg;
    int tempo_model;
    double pivot1_t, pivot1_h, pivot1_tp, pivot2_t, pivot2_h, pivot2_tp;
    float integral; // for PID model
    deque<double> errors;
    deque<double> tempos; // for DEQ_ARZT model
    int t_passed;
    double last_arzt;
    float sensitivity; // tempo fluctuations
    float elasticity; // tempo response amp.
    float error; // tempo tracking error vs DTW path / beats
	vector<vector<double> > b_err; // backwards oDTW path error
	double h_real; // "real" score coordinate produced by applying the TempoModel to the oDTW path
    
    // beat tracking vars
    vector<vector<float> > acc_beats; // acc_beats[0][]: beats; acc_beats[1][]: tempo
    vector<vector<float> > y_beats; // y_beats[0][]: beats; y_beats[1][]: diffs to acco
    unsigned int acc_iter, b_iter;
    double prev_h_beat;
    float b_stdev, minerr;
    float elast_beat; // elasticity modulation by beat accuracy
    bool beat_due;
    double ref_tempo;

	// internal methods
    double calc_tempo(int mode);
    
    // internal beat methods
    int calc_beat_diff(double cur_beat, double prev_beat, double ref_beat);
    unsigned int update_beat_iter(unsigned int beat_index, vector<float> *beat_vector, double ref_beat);
    double calc_beat_tempo();
    short tempo_mode; // 0: insensitive, 1: DTW track, 2: beat track
};

