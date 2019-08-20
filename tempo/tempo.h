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

	// internal methods
    double calc_tempo(int mode);
};

