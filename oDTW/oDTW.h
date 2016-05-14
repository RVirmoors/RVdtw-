#pragma once

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "z_dsp.h"			// required for MSP objects
#include "ext_buffer.h"		// MSP buffer
#include "defines.h"		// int defines

#include <vector>

using namespace std;

class oDTW
{
public:
	// constructor & destructor
	oDTW(int windowSize_ = 128, int backWindowSize_ = 32, bool backActive_ = true);
	~oDTW();

	// set Score (reference) size (number of FV frames). 
	//		(any previously existing data in the internal vars is removed.)
	void setScoreSize(long v);

	// add a Score (reference) feature vector
	void processScoreFV(double *tfeat);

	// add a Live (target) feature vector
	void processLiveFV(double *tfeat);

	// add a Marker to the Score ref.
	void addMarkerToScore();

	// add a Marker to the Live target
	void addMarkerToLive();

	// empty all the internal vars, except the Score, and point back to the start.
	void start();

	// get current live time T (in frames)
	t_uint16 getT();

	// get current reference time H
	t_uint16 getH();

	// set new reference time. Use with caution - causes the system to effectively jump in time!
	t_uint16 setH();

	// get historic H value
	t_uint16 getHistory(t_uint16 t_);

private:
	// internal methods
	void init_dtw();
	void distance(t_uint16 i, t_uint16 j);	
	t_uint16 get_inc();
	void calc_dtw(t_uint16 i, t_uint16 j);
	void dtw_process();
	void increment_t();
	void increment_h();
	bool decrease_h();
	void dtw_back();

	// internal vars
	int fsize, bsize;
	long ysize;	
	t_uint16 params, t, t_mod, h, h_mod, previous, iter;
	vector<vector<t_atom_float> > x, y; 
	vector<t_uint16> history, b_path;
	vector<vector<double> > b_err;
	vector<vector<double> > Dist, dtw, b_dtw, b_move;
	t_uint16 b_start, bh_start;
	double b_avgerr;
	float mid_weight, top_weight, bot_weight;
	bool follow;

	vector<vector<double> > markers;
	t_uint16 runCount, maxRunCount, m_iter, m_ideal_iter, m_count, input_sel;
	t_atom dump[50];

	bool back_active;
};

