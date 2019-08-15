// oDTW.h
//
// Copyright (C) 2014-2017 Grigore Burloiu
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

#include <vector>

// == int defines ==

// previous dtw step:
#define NEW_ROW 1
#define NEW_COL 2
#define NEW_BOTH 3

// marker matrix:
#define M_SCORED 0
#define M_HOOK 1
#define M_ACC 2
#define M_IDEAL 3
#define M_LIVE 4

// back:
#define B_ERROR 0
#define B_T 1
#define B_H 2
#define B_TEMPO 3

// == DTW params ==
#define	FSIZE 128//32 // DTW window length (# of input frames)
#define BSIZE 512//128 // backwards DTW win length; should be larger than fsize

#define MAXLENGTH 500000 //maximum input file length (# of frames)
#define VERY_BIG  (1e10)
#define MAX_RUN 64// 3 //64//50000    minimum 4; should not surpass fsize
#define ALPHA 0.1 //1
#define MID 1 //0.5; //1.9; //1.5
#define SIDE 2


using namespace std;

class oDTW
{
public:
	// constructor & destructor
	oDTW(int windowSize_ = FSIZE, int backWindowSize_ = BSIZE, bool backActive_ = true, unsigned int params_ = 12);
	~oDTW();

	// set Score (reference) size (number of FV frames). 
	//		(any previously existing data in the internal vars is removed.)
    // returns # of params if succeeded, 0 if wrong/missing length v.
	unsigned int setScoreSize(long v);

	// add a Score (reference) feature vector. Returns the # of the current vector (between 0 and ysize).
	unsigned int processScoreFV(double *tfeat);

	// add a Live (target) feature vector
	void processLiveFV(double *tfeat);

	// add a Marker to the Score ref.
	void addMarkerToScore(unsigned int frame = 0);

	// add a Marker to the Live target. Returns the # of the added marker.
	unsigned int addMarkerToLive(unsigned int frame = 0);
    
    // get frame number for a certain (score) marker
    unsigned int getMarkerFrame(long here);
    
    // get marker count
    unsigned int getMarkerCount();
    
    // get markers
    double getMarker(unsigned int i, unsigned int j);
    
	// empty all the internal vars, except the Score, and point back to the start.
	void start();

	// get current live time T (in frames)
	unsigned int getT();

	// get current reference time H
	unsigned int getH();
    
    // set new reference time. Use with caution - causes the system to effectively jump in time!
    void setH(unsigned int to_h);

	// get historic H value
	unsigned int getHistory(unsigned int from_t);
    
    // get Y score value
    double getY(unsigned int i, unsigned int j);
    
    // get oDTW window size
    unsigned int getFsize();
    
    // get backwards path vector[4]: error, t, h, local tempo
    vector<vector<double> > getBackPath();
    
    // are we processing live FVs, and has the end not been reached?
    bool isRunning();
    
    // is the whole score loaded?
    bool isScoreLoaded();
    
    // set # of params
    void setParams(int params_);

private:
	// internal methods
	void init_dtw();
	void distance(unsigned int i, unsigned int j);	// bsize x bsize
	unsigned int get_inc();
	void calc_dtw(unsigned int i, unsigned int j);	// fsize x fsize
	bool dtw_process();
	void increment_t();
	void increment_h();
	bool decrease_h();
	void dtw_back();								// bsize x bsize

	// internal vars
	int fsize, bsize;
	long iter, ysize;
	unsigned int params, t, t_mod, h, h_mod, previous;
	vector<vector<double> > x, y;
	vector<unsigned int> history, b_path;
    vector<vector<double> > b_err;
	vector<vector<double> > Dist, dtw, b_dtw, b_move;
	unsigned int b_start, bh_start;
	double b_avgerr;
	float mid_weight, top_weight, bot_weight;

	vector<vector<double> > markers;
	unsigned int runCount, maxRunCount, m_iter, m_ideal_iter, m_count;

	bool back_active;
    bool score_loaded;
};

