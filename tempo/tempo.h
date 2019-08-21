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

#include "oDTW.h"
#include <vector>
#include <deque>

// tempo tracking params
#define SEN 1 // 0.9 // 0.98//1
#define ELA 1 // 1

// tempo modes:
#define OFF 0
#define DTW 1
#define BEAT 2

// tempo models:
#define T_DTW 0
#define T_P 1
#define T_PID 2
#define Kp 0.017 //0.017 //0.5 // 0.1467
#define Ki 0.0003//0.0004 // 0.0007 // 0.01 // 0.005051
#define Kd 0.4 //2.5
#define T_DEQ 3
#define T_ARZT 4

using namespace std;

class TempoModel
{
public:
	// constructor & destructor
	TempoModel(oDTW *warp_);
	~TempoModel();

	// set Sensitivity to tempo fluctuations [0...1]
    void setSensitivity(float sen);
    
    // set Elasticity of tempo model [0...1...]
    void setElasticity(float ela);
    
private:
	// internal vars
    oDTW *warp;
    float sr;
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
    
    // beat tracking vars
    vector<vector<float> > acc_beats; // acc_beats[0][]: beats, acc_beats[1][]: tempo
    vector<vector<float> > y_beats; // y_beats[0][]: beats, y_beats[1][]: diffs to acco
    unsigned int acc_iter, b_iter;
    double prev_h_beat;
    float b_stdev, minerr;
    float elast_beat; // elasticity modulation by beat accuracy
    bool beat_due;
    double ref_tempo;

	// internal methods
    double calc_tempo(int mode);
    
    // beat methods
    void beat_switch();
    int calc_beat_diff(double cur_beat, double prev_beat, double ref_beat);
    unsigned int update_beat_iter(unsigned int beat_index, vector<float> *beat_vector, double ref_beat);
    double calc_beat_tempo();
    void add_beat(unsigned int pos, double tempo);
    short tempo_mode; // 0: insensitive, 1: DTW track, 2: beat track
};

