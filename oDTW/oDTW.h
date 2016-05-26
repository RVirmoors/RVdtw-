#pragma once

#include "defines.h"		// int defines

#include <vector>


// DTW params
#define	FSIZE 128//32 // DTW window length (# of input frames)
#define BSIZE 512//128 // backwards DTW win length; should be larger than fsize

#define MAXLENGTH 500000 //maximum input file length (# of frames)
#define VERY_BIG  (1e10)
#define MAX_RUN 64// 3 //64//50000    minimum 4; should not surpass fsize
#define ALPHA 1
#define MID 0.5 //0.5; //1.9; //1.5
#define SIDE 1
#define SEN 1 // 0.9 // 0.98//1
#define ELA 1 // 1



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

	// add a Score (reference) feature vector
	void processScoreFV(double *tfeat);

	// add a Live (target) feature vector. Returns TRUE if running, FALSE if end reached.
	bool processLiveFV(double *tfeat);

	// add a Marker to the Score ref.
	void addMarkerToScore(unsigned int frame);

	// add a Marker to the Live target (should be used for testing only)
	void addMarkerToLive(unsigned int frame);
    
    // get frame number for a certain (score) marker
    unsigned int getMarkerFrame(long here);

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

private:
	// internal methods
	void init_dtw();
	void distance(unsigned int i, unsigned int j);	
	unsigned int get_inc();
	void calc_dtw(unsigned int i, unsigned int j);
	bool dtw_process();
	void increment_t();
	void increment_h();
	bool decrease_h();
	void dtw_back();

	// internal vars
	int fsize, bsize;
	long ysize;	
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
};

