// rvdtw~.h
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

#define RV_VERSION 0.302

/*

Version history:
0.302 - updates for M4L/MIDIacco support
0.3 - added beat tracking module

*/

// TO DO: make separate class for TEMPO MODELS
// TO DO: linear interpolation of SEN between beats ?

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "z_dsp.h"			// required for MSP objects
#include "ext_buffer.h"		// MSP buffer
#include "defines.h"		// int defines

//#define USE_FFTW	// preprocessor define!
#ifdef USE_FFTW
#include <fftw3.h>
#endif
#include "chromagram.h"
#include "BTrack.h"
#include "oDTW.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>


// FFT params
#define WINDOW_SIZE 2048
#define HOP_SIZE 512


// compression params
#define COMP_THRESH -140
#define COMP_RELEASE 2
#define COMP_RATIO 8

#define SEN 1 // 0.9 // 0.98//1
#define ELA 1 // 1


using namespace std;

class Raskell;

extern "C" {
	// struct to represent the object's state
	typedef struct _RVdtw {
		t_pxobject		ob;			// the object itself (t_pxobject in MSP instead of t_object)
		Raskell*		rv;

		void* out_t;
		void* out_h;
		void* out_beats;
		void* out_tempo;
		void* out_dump;
	} t_RVdtw;


	// method prototypes
	void *RVdtw_new(t_symbol *s, long argc, t_atom *argv);
	void RVdtw_free(t_RVdtw *x);

	void RVdtw_assist(t_RVdtw *x, void *b, long m, long a, char *s);
	void RVdtw_feats(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_input(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_scoresize(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_beat(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);

	void RVdtw_read(t_RVdtw *x, t_symbol *s);
	void RVdtw_readacco(t_RVdtw *x, t_symbol *s);
	void RVdtw_do_read(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv); // in deferred thread
	void RVdtw_write(t_RVdtw *x, t_symbol *s);
	void RVdtw_do_write(t_RVdtw *x, t_symbol *s); // in deferred thread

	void RVdtw_gotomarker(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_gotoms(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_start(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_stop(t_RVdtw *x, t_symbol *s);
	void RVdtw_follow(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_sensitivity(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_elasticity(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_getscoredims(t_RVdtw *x, t_symbol *s);
	void RVdtw_dumpscore(t_RVdtw *x, t_symbol *s);
	void RVdtw_do_dumpscore(t_RVdtw *x, t_symbol *s); // in deferred thread

	void RVdtw_dblclick(t_RVdtw *x);
	t_max_err RVdtw_notify(t_RVdtw *x, t_symbol *s, t_symbol *msg, void *sender, void *data);

	void RVdtw_dsp(t_RVdtw *x, t_signal **sp, short *count);
	void RVdtw_dsp64(t_RVdtw *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

	t_int *RVdtw_perform(t_int *w);
	void RVdtw_perform64(t_RVdtw *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

	// global class pointer variable
	static t_class *RVdtw_class = NULL;

} // end extern "C"

class Raskell {
public:
	t_RVdtw *max;

	// internal vars:
	long ysize;
	//t_uint16 t, t_mod, h, h_mod, previous, iter;
    t_uint16 iter;
	double h_real;
	//vector<vector<t_atom_float> > x, y;
	//vector<t_uint16> history, b_path;
	vector<vector<double> > b_err;
	//vector<vector<double> > Dist, dtw, b_dtw, b_move;
	t_uint16 b_start, bh_start, bsize;
	//double b_avgerr;
	//float mid_weight, top_weight, bot_weight;
	bool follow;

    unsigned int params;    // number of params in feature vectors
	unsigned int features;  // MFCCs or chroma vectors
	//vector<vector<double> > markers;

	deque<t_uint16> Deque;
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
    unsigned int fsize;

	//t_uint16 runCount, maxRunCount, m_iter, m_ideal_iter, m_count;
    t_uint16 input_sel;
	t_atom dump[50];

	//		DSP vars:
	long dsp_vector_size; 
	t_uint16 active_frames;
	vector<t_uint16> frame_index;
	float SampleRate;
	vector<vector<double> > frame, banks;
	double *in, *logEnergy, *tfeat;//, *samp;
	vector<double> window;
	int m; // Mel filterbanks
#ifdef USE_FFTW
	fftw_complex *out;
    fftw_plan plan, dct;
#endif
	BTrack *beat;
	Chromagram *chroma;
	vector<double> chr;
    oDTW *warp;

	//		beat tracking vars:
	vector<vector<float> > acc_beats; // acc_beats[0][]: beats, acc_beats[1][]: tempo
	vector<vector<float> > y_beats; // y_beats[0][]: beats, y_beats[1][]: diffs to acco
	t_uint16 acc_iter, b_iter;
	double prev_h_beat;
	float b_stdev, minerr;
	float elast_beat; // elasticity modulation by beat accuracy
	bool beat_due;
	double ref_tempo;

	//		file handling vars:
	t_filehandle f_fh;			
	short f_open;			/* spool flag */
	short f_spool;
	t_uint16 **f_data;			/* read in data */
	long f_size;
	t_symbol *ps_nothing;
	string score_name, live_name;

	Raskell();
	~Raskell();
	void init(t_symbol *s,  long argc, t_atom *argv);
	void perform(double *in, long sampleframes, int dest);

	// inlet methods:
	void feats(t_uint16 argc);
	void score_size(long v);
	void marker(t_symbol *s);
	void reset();
	void input(long v);
	void gotomarker(long v);
	void gotoms(long v);
	void getscoredims();
	void dumpscore();

	// feat extraction methods:
	bool zeros(double *in, long sampleframes);
	void dspinit();
	void fillBanks();
	void hamming(int windowLength);
	void add_sample_to_frames(double sample);
	void reset_feat();
	void calc_mfcc(t_uint16 frame_to_fft);
	double compress(double value, bool active);

	// DTW methods:
//	void init_dtw();
//	void distance(t_uint16 i, t_uint16 j);	
//	t_uint16 get_inc();
//	void calc_dtw(t_uint16 i, t_uint16 j);
//	void dtw_process();
//	void increment_t();
//	void increment_h();
//	bool decrease_h();
//	void dtw_back();

	// tempo model methods:
	double calc_tempo(int mode);

	// beat methods:
    void beat_switch();
	int calc_beat_diff(double cur_beat, double prev_beat, double ref_beat);
	t_uint16 update_beat_iter(t_uint16 beat_index, vector<float> *beat_vector, double ref_beat);
	double calc_beat_tempo();
	void add_beat(t_uint16 pos, double tempo);
	short tempo_mode; // 0: insensitive, 1: DTW track, 2: beat track

	// file input / output methods:
	bool do_read(t_symbol *s);
	void file_open(char *name);
	bool read_line();
	void file_close();
	char *sgets(char *str, int num);
	void do_write(t_symbol *s);

	// buffer stuff
	t_symbol *ps_buffer_modified;	
	t_symbol *buf_name;
	t_buffer_ref *l_buffer_reference;
	void set_buffer(t_symbol *s, int dest);
};
/*
// ===== helper functions =====

std::string to_string (float number) {
    std::ostringstream buff;
    buff<<number;
    return buff.str();   
}

std::string to_string (t_uint16 number) {
    std::ostringstream buff;
    buff<<number;
    return buff.str();   
}*/