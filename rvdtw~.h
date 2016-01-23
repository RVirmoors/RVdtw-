#define RV_VERSION 0.2
// TO DO: read in chunks
// TO DO: goto h

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "z_dsp.h"			// required for MSP objects
#include "ext_buffer.h"		// MSP buffer


#include <fftw3.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <deque>
#include <sstream>

// FFT params
#define	fsize 128// 128//32 // DTW window length (# of input frames)
#define bsize 128//128 // backwards DTW win length; should be larger than fsize
#define WINDOW_SIZE 2048
#define HOP_SIZE 512

// DTW params
#define MAXLENGTH 5000000 //maximum input file length (# of frames)
#define VERY_BIG  (1e10)
//#define THRESH 0 //0.4 // base threshold for marker admission
#define MAX_RUN 64//50000  //3  minimum 4; should not surpass fsize
#define ALPHA 1//1

// compression params
#define COMP_THRESH -140
#define COMP_RELEASE 2
#define COMP_RATIO 8

// int defines
#define NEW_ROW 1
#define NEW_COL 2
#define NEW_BOTH 3

#define IN_SCORE 1
#define IN_LIVE 2

#define M_SCORED 0
#define M_HOOK 1
#define M_IDEAL 2
#define M_LIVE 3

using namespace std;

class Raskell;

extern "C" {
	// struct to represent the object's state
	typedef struct _RVdtw {
		t_pxobject		ob;			// the object itself (t_pxobject in MSP instead of t_object)
		Raskell*		rv;

		void* out_t;
		void* out_h;
		void* out_mfccs;
		void* out_tempo;
		void* out_dump;
	} t_RVdtw;


	// method prototypes
	void *RVdtw_new(t_symbol *s, long argc, t_atom *argv);
	void RVdtw_free(t_RVdtw *x);

	void RVdtw_assist(t_RVdtw *x, void *b, long m, long a, char *s);
	void RVdtw_mfccs(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_input(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_scoresize(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_read(t_RVdtw *x, t_symbol *s);
	void RVdtw_do_read(t_RVdtw *x, t_symbol *s); // in deferred thread
	void RVdtw_write(t_RVdtw *x, t_symbol *s);
	void RVdtw_do_write(t_RVdtw *x, t_symbol *s); // in deferred thread

	void RVdtw_gotomarker(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_gotoms(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_start(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);
	void RVdtw_stop(t_RVdtw *x, t_symbol *s);
	void RVdtw_follow(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv);

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
	t_uint16 params, t, t_mod, h, h_mod, h2, h_mod2, previous, iter;
	vector<vector<t_atom_float> > x, y; 
	vector<t_uint16> history;
	vector<vector<double> > b_path;
	vector<vector<double> > Dist, dtw, b_dtw, b_move;
	deque<double> Deque;
	t_uint16 b_start;
	float mid_weight, side_weight;
	bool follow;

	vector<vector<t_uint16> > markers;
	float tempo;
	t_uint16 pivot1_t, pivot1_h, pivot2_t, pivot2_h;
	double tempo_prob;

	t_uint16 runCount, maxRunCount, m_iter, m_ideal_iter, m_count, input_sel;
	t_atom dump[50];

	//		DSP vars:
	t_uint16 active_frames;
	vector<t_uint16> frame_index;
	float SampleRate;
	vector<vector<double> > frame, banks;
	double *in, *logEnergy, *tmfcc;
	vector<double> window;
	int m;
	t_atom *mfcc_frame;
	fftw_complex *out;
    fftw_plan plan, dct;

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
	void perform(double *in, long sampleframes);

	// inlet methods:
	void mfccs(t_uint16 argc);
	void score_size(long v);
	void marker(t_symbol *s);
	void reset();
	void input(long v);
	void gotomarker(long v);
	void gotoms(long v);
	//void start();

	// MFCC extraction methods:
	void dspinit();
	void fillBanks();
	void hamming(int windowLength);
	void add_sample_to_frames(double sample);
	void reset_mfcc();
	void calc_mfcc(t_uint16 frame_to_fft);

	// DTW methods:
	void init_dtw();
	void distance(t_uint16 i, t_uint16 j);	
	t_uint16 get_inc();
	void calc_dtw(t_uint16 i, t_uint16 j);
	void dtw_process();
	double compress(double value, bool active);
	void increment_t();
	void increment_h();
	void decrease_h();
	void dtw_back();

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
	void set_buffer(t_symbol *s);
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