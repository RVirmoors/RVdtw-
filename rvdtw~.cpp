#include <rvdtw~.h>

// TO DO: check marker detection, write result markers[x][2]

extern "C"
{

int C74_EXPORT main(void) {	
	t_class *c = class_new("RVdtw~", (method)RVdtw_new, (method)RVdtw_free, (long)sizeof(t_RVdtw), 
		0L, A_GIMME, 0);
	class_dspinit(c);
	
	class_addmethod(c, (method)RVdtw_mfccs,		"mfccs",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_input,		"input",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_scoresize,	"score_size",	A_GIMME, 0); // not needed for users
	class_addmethod(c, (method)RVdtw_read,		"read",		A_DEFSYM,0);
	class_addmethod(c, (method)RVdtw_write,		"write",	A_DEFSYM,0);

	class_addmethod(c, (method)RVdtw_gotomarker, "gotomarker",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_gotoms,	"gotoms",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_start,		"start",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_stop,		"stop",		A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_follow,	"follow",		A_GIMME, 0);

	class_addmethod(c, (method)RVdtw_dblclick,	"dblclick",	A_CANT, 0);	
	class_addmethod(c, (method)RVdtw_notify,	"notify",	A_CANT, 0);

	class_addmethod(c, (method)RVdtw_dsp,		"dsp",		A_CANT, 0);		// Old 32-bit MSP dsp chain compilation for Max 5 and earlier
	class_addmethod(c, (method)RVdtw_dsp64,		"dsp64",	A_CANT, 0);		// New 64-bit MSP dsp chain compilation for Max 6
	class_addmethod(c, (method)RVdtw_assist,	"assist",	A_CANT, 0);
	
	class_register(CLASS_BOX, c);
	RVdtw_class = c;

	return 0;
}


void *RVdtw_new(t_symbol *s, long argc, t_atom *argv) {
	t_RVdtw *x = (t_RVdtw *)object_alloc(RVdtw_class);
	x->rv = new Raskell();	

	if (x) {
		x->rv->max = x;
		dsp_setup((t_pxobject *)x, 1);	// MSP inlets: arg is # of inlets
		// outlets, RIGHT to LEFT:
		x->out_tempo = floatout((t_object *) x);
		x->out_mfccs = listout((t_object *) x);
		x->out_dump = listout((t_object *) x);
		x->out_h = intout((t_object *) x);
		x->out_t = intout((t_object *) x);
		x->rv->init(s, argc, argv);
	}
	return (x);
}

void RVdtw_free(t_RVdtw *x) {
	dsp_free((t_pxobject*)x);
	object_free(x->rv->l_buffer_reference);
}


//***********************************************************************************************

void RVdtw_assist(t_RVdtw *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET)			// inlet
    { 
                sprintf(s, "int: set length, mfccs: add frame");
    } 
    else							// outlets
    {	
		switch (a) 
        {
            case 0: sprintf(s, "t"); break;
            case 1: sprintf(s, "h"); break;
            case 2: sprintf(s, "dump"); break;                   
			case 3: sprintf(s, "computed MFCC frames"); break;
			case 4: sprintf(s, "tempo multiplier"); break;
        }	
    }
}

void RVdtw_mfccs(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	// TODO AICI CONVERT DIN ATOMS IN ARRAY
	for (long i = 0; i < argc; i++)
		x->rv->tmfcc[i] = atom_getfloat(argv+i);
	x->rv->mfccs(argc);
}

void RVdtw_input(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	long v = atom_getlong(argv);
	x->rv->input(v);
}

void RVdtw_scoresize(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	long v = atom_getlong(argv);
	x->rv->score_size(v);
}

void RVdtw_read(t_RVdtw *x, t_symbol *s) {
	defer_low(x,(method)RVdtw_do_read,s,0,0L);
}

void RVdtw_do_read(t_RVdtw *x, t_symbol *s) {
	if (!x->rv->do_read(s))
		x->rv->set_buffer(s);
}

void RVdtw_read_line(t_RVdtw *x, t_symbol *s) {
	//post("read %.2f percent...", (float)(x->rv->iter * 100) / x->rv->ysize );
	if(x->rv->read_line())
		defer_low(x,(method)RVdtw_read_line,0,0,0L);
}

void RVdtw_write(t_RVdtw *x, t_symbol *s) {
	defer_low(x,(method)RVdtw_do_write,s,0,0L);
}

void RVdtw_do_write(t_RVdtw *x, t_symbol *s) {
	x->rv->do_write(s);
}

void RVdtw_gotomarker(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	long v = atom_getlong(argv);
	x->rv->gotomarker(v);
}

void RVdtw_gotoms(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	long v = atom_getlong(argv);
	x->rv->gotoms(v);
}

void RVdtw_start(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	post("START");
	x->rv->reset();
	x->rv->input(IN_LIVE); // 1 = SCORE; 2 = LIVE; 0 = closed
}

void RVdtw_stop(t_RVdtw *x, t_symbol *s) {
	post("STOP");
	x->rv->reset();
	x->rv->input(0); // 1 = SCORE; 2 = LIVE; 0 = closed
}

void RVdtw_follow(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	if (atom_getlong(argv) == 0) {
		x->rv->follow = FALSE;
		post("online DTW follow: OFF");
	} else {
		x->rv->follow = TRUE;
		post("online DTW follow: ON");
	}
}

// this lets us double-click to open up the buffer~ it references
void RVdtw_dblclick(t_RVdtw *x) {
	buffer_view(buffer_ref_getobject(x->rv->l_buffer_reference));
}

// this handles notifications when the buffer appears, disappears, or is modified.
t_max_err RVdtw_notify(t_RVdtw *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
	if (msg == x->rv->ps_buffer_modified)
		x->rv->set_buffer(x->rv->buf_name);
	return buffer_ref_notify(x->rv->l_buffer_reference, s, msg, sender, data);
}



//***********************************************************************************************

// this function is called when the DAC is enabled, and "registers" a function for the signal chain in Max 5 and earlier.
// In this case we register the 32-bit, "RVdtw_perform" method.
void RVdtw_dsp(t_RVdtw *x, t_signal **sp, short *count)
{
	//post("my sample rate is: %f", sp[0]->s_sr);
	dsp_add(RVdtw_perform, 3, x, sp[0]->s_vec, sp[0]->s_n); // ins & sampleframes
}


// this is the Max 6 version of the dsp method -- it registers a function for the signal chain in Max 6,
// which operates on 64-bit audio signals.
void RVdtw_dsp64(t_RVdtw *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	//post("my sample rate is: %f", samplerate);
	object_method(dsp64, gensym("dsp_add64"), x, RVdtw_perform64, 0, NULL);
}


//***********************************************************************************************

// this is the 32-bit perform method for Max 5 and earlier
t_int *RVdtw_perform(t_int *w) {
	// DO NOT CALL post IN HERE, but you can call defer_low (not defer)
	
	// args are in a vector, sized as specified in RVdtw_dsp method
	// w[0] contains &RVdtw_perform, so we start at w[1]
	t_RVdtw *x = (t_RVdtw *)(w[1]);
	t_float *in = (t_float *)(w[2]);
	int n = (int)w[3];

	x->rv->perform((double *)in, n);
	// you have to return the NEXT pointer in the array OR MAX WILL CRASH
	return w + 5;
}


// this is 64-bit perform method for Max 6
void RVdtw_perform64(t_RVdtw *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	t_double *in = ins[0];		// we get audio for each inlet of the object from the **ins argument
	int n = sampleframes;
	x->rv->perform(in, n);
}


} // end extern C


Raskell::Raskell() {
	
		ysize = t = h = runCount = iter = m_iter = m_ideal_iter = t_mod = h_mod = 0; // current position for online DTW: (t,h)
		tempo = 1;
		m_count = 0; // one marker is mandatory (to start)
		previous = 0; // 0 = none; 1 = Row; 2 = Column; 3 = Both
		input_sel = IN_SCORE; // 1 = SCORE; 2 = LIVE; 0 = closed		
		follow = TRUE;

		mid_weight = 1.9; //1.5
		side_weight = 1; //1

		maxRunCount = MAX_RUN; // tempo between 1/x and x
		params = 40; // ysize; // # of MFCCs at input

		SampleRate = sys_getsr();
		active_frames = WINDOW_SIZE / HOP_SIZE;
		// number of frames that will be computed at any one time
		m = 48; // number of Mel filterbanks

		dspinit();
		l_buffer_reference = NULL;
		score_name = live_name = "";
						
		in = fftw_alloc_real(WINDOW_SIZE);
		logEnergy = fftw_alloc_real(m);		// 48 filter banks
		mfcc_frame = (t_atom*)sysmem_newptrclear(params * sizeof(t_atom_float));
		out = fftw_alloc_complex(WINDOW_SIZE/2 + 1);
		tmfcc = fftw_alloc_real(m);
        plan = fftw_plan_dft_r2c_1d(WINDOW_SIZE, in, out, FFTW_MEASURE); // FFT real to complex FFTW_MEASURE 
		dct = fftw_plan_r2r_1d(m, logEnergy, tmfcc, FFTW_REDFT10, NULL);

		post ("RV object created");
}


Raskell::~Raskell() {
		sysmem_freeptr(mfcc_frame);
		fftw_destroy_plan(plan);
        fftw_free(in); fftw_free(out);
		fftw_free(logEnergy);
		fftw_free(tmfcc);
		fftw_cleanup();
		
		//file_close();
}

void Raskell::init(t_symbol *s,  long argc, t_atom *argv) {
		f_open = FALSE;
		f_fh = 0;
		f_spool = 0;
		f_data = 0;
		ps_nothing = gensym("");
		ps_buffer_modified = gensym("buffer_modified");	

		post("RVdtw version %f", RV_VERSION);

		buf_name = (argc) ? atom_getsym(argv) : gensym("");
		
		if (argc) {
			if(!do_read(buf_name))
				set_buffer(buf_name); // first argument
		}
		else post("no score preloaded");

		srand(744);
}

void Raskell::perform(double *in, long sampleframes) {
	if ((h <= ysize) && (iter < ysize) && in[0]) { //&&notzero(ins, sampleframes
		t_uint32 i, j;
		for (i=0; i < sampleframes; i++) {
			add_sample_to_frames(in[i]);
		}
		for(i=0; i<active_frames; i++) {
			if(frame_index[i]==0) {		// if frame is full, then compute
				for(j=0; j<WINDOW_SIZE; j++)
					frame[i][j] *= window[j]; // apply hamming window
				calc_mfcc(i); // get tmfcc[]
				// if signal is loud enough, then compress the first MFCC coefficient (loudness)
				if (tmfcc[0] > COMP_THRESH) 
					tmfcc[0] = compress(tmfcc[0], true);//tmfcc[0] /= 8;
				else tmfcc[0] = compress(tmfcc[0], false);
				mfccs(params);
			}
		}
	}
}

void Raskell::mfccs(t_uint16 argc) {
	if ((ysize > 0) && (input_sel > 0))  { // Y matrix was init'd, let's populate it
		t_uint16 i, j;
		double match=1, match_prev=1;
		if (params != argc) {
			post("new number of params: %d", argc);
			params = (t_uint16)argc;
			score_size(ysize);
		}

		if ((input_sel == IN_SCORE) && (iter < ysize)) { // build Y matrix (offline)
			for (j=0; j<argc; j++) {
				y[iter][j] = tmfcc[j];
			}
			iter++; //iterate thru Y
			if (iter == ysize)
				post("SCORE data fully loaded: %i frames, %i markers", ysize, m_count); 
		}
	
		if (input_sel == IN_LIVE) { // new X feature entered
			iter++;

			for (i=0; i<argc; i++) {
				x[t%bsize][i] = tmfcc[i];
			}
			

			if (t == 0)//xsize-1)  // is the window full?
				init_dtw();
			else {
				dtw_process(); // calculate next DTW step
				dtw_back(); // calculate backwards DTW for tempo
			}
		}	// end input_sel==2 check
	} // end input_sel>0 check
}

void Raskell::score_size(long v) {
	long i, j;
	if ((v < MAXLENGTH) && (v > 0)) {
		ysize = v;
		// we have ysize -> MEMORY ALLOCATION
		y.clear();
		y.resize(ysize);
		for (i=0; i<ysize; i++) {
			y[i].resize(params);
		}
		post("Score Matrix memory alloc'd, size %i * %i", ysize, params);	

		reset(); // x, dtw arrays

		markers.clear();
		markers.resize(ysize);
		for (i=0; i<ysize; i++)
			markers[i].resize(4); // 0: scored, 1: hook 0/1, 2: live ideal, 3: live detected
	}
	else post("wrong/missing length");	
}

void Raskell::marker(t_symbol * s) {
	// add new marker
	if ((input_sel == IN_SCORE) && ysize) {
		markers[m_iter][M_SCORED] = iter; // new marker at position iter
		post("Marker entered at position %i", iter);
		markers[m_iter][M_HOOK] = (t_uint16)s;// ? atom_getlong(av) : 0;
		//post("MARKER_HOOK: %i", markers[m_iter][1]);
		m_count++;
		m_iter++;
	}
	else if (input_sel == IN_LIVE) {
		markers[m_ideal_iter][M_IDEAL] = iter;
		post("Marker (ideal) entered at position %i", iter);
		m_ideal_iter++;
	}
	
}

void Raskell::reset() {
	t = t_mod = h = h_mod = runCount = iter = m_iter = m_ideal_iter = 0; // current position for online DTW: (t,h)
	tempo = 1;
	previous = 0; // 0 = none; 1 = Row; 2 = Column; 3 = Both
	
	short i;

	reset_mfcc();

	x.clear();
	x.resize(bsize);
	for (i=0; i<bsize; i++) {
		x[i].resize(params);
	}

	history.clear();
	history.resize(bsize);
	pivot1_t = pivot1_h = pivot2_t = pivot2_h = 0;

	b_path.clear();
	b_path.resize(bsize*2);
	for (i=0; i<bsize*2; i++) {
		b_path[i].resize(2); // t & h
	}

	Dist.clear();
	Dist.resize(bsize);
	for (i=0; i<bsize; i++) {
		Dist[i].resize(bsize);
	}
		
	dtw.clear();
	dtw.resize(fsize);
	for (i=0; i<fsize; i++) {
		dtw[i].resize(fsize);
		fill(dtw[i].begin(), dtw[i].end(), VERY_BIG);
	}

	b_dtw.clear();
	b_dtw.resize(bsize);
	for (i=0; i<bsize; i++) {
		b_dtw[i].resize(bsize);
		fill(b_dtw[i].begin(), b_dtw[i].end(), VERY_BIG);
	}

	b_move.clear();
	b_move.resize(bsize);
	for (i=0; i<bsize; i++) {
		b_move[i].resize(bsize);
	}
	post("At start position!");
}

void Raskell::input(long v) {
	// select input source: SCORE or LIVE 
	reset_mfcc();
	input_sel = v;
	if((input_sel == IN_LIVE)&& !ysize) {
		object_error((t_object *)this, "initialize SCORE first!"); 
		input_sel = 0;
		m_iter = 0;
		m_ideal_iter = 0;
	}
	switch (input_sel) {
		case 0 : post("input selection is: OFF"); break;
		case IN_SCORE : post("input selection is: SCORE"); break;
		case IN_LIVE : post("input selection is: LIVE"); break;
	}
	
	if (input_sel == IN_LIVE) {
		markers[m_count][0] = ysize-fsize; // add END marker
		m_iter = 0; // start listening for event markers
		iter = 0;
	}
}

void Raskell::gotomarker(long v) {
	if ((v >= m_count) || (v < 0))
		object_error((t_object *)this, "marker %i doesn't exist!", v);
	else {
		v = markers[v][0] * HOP_SIZE;
		double ms = v / SampleRate * 1000.f;
		gotoms(ms);
	}
	
}

void Raskell::gotoms(long v) {
	post("Starting from %i ms", v);
	h = v * (double)(SampleRate / 1000.f / HOP_SIZE);
	h_mod = h % fsize;
	//t = h;
	//t_mod = h_mod;
}


// ======================== DSP stuff ================

void Raskell::dspinit() {
	fillBanks();
	hamming(WINDOW_SIZE); // build window vector

	reset_mfcc(); // sets frame_indexes, resizes frame vectors.
	post("DSP ready!");
}

void Raskell::fillBanks() {
	// compute Mel filter banks
	double * cf;

	//calculate the center frequencies: mel(f) = 2595 * log10(1 + f/700)
	cf = new double[m+2];
	for (int i = 0; i < m+2; i++) {
		cf[i] = (WINDOW_SIZE/SampleRate)*700*(exp((i*2595.0*log10(1+SampleRate/1400.0)/(m+1))/1127.0)-1);
		cf[i] = (int)cf[i];
	}

	//fill in each bank
	banks.clear();
	banks.resize(m);
	for (int i = 1; i <= m; i++) {
		banks[i-1].reserve(WINDOW_SIZE/2);
		for (t_uint16 j = 0; j < WINDOW_SIZE/2; j++) {
			if (j < cf[i-1]) {
				banks[i-1].push_back(0);
			}
			else if (j >= cf[i-1] && j < cf[i]) {
				banks[i-1].push_back((double)
					(j-cf[i-1])/(cf[i]-cf[i-1]) );
			}
			else if (j >= cf[i] && j < cf[i+1]) {
				banks[i-1].push_back((double)
					(cf[i+1]-j)/(cf[i+1]-cf[i]) );
			}
			else {
				banks[i-1].push_back(0);
			}
		}
	}
	
	delete cf;
}

void Raskell::hamming(int windowLength) {
	window.clear();
	window.reserve(windowLength);
	for(int i = 0; i < windowLength; i++) {
		window.push_back(0.54 - (0.46 * cos( 2 * PI * (i / ((windowLength - 1) * 1.0)))) );
	}
}

void Raskell::add_sample_to_frames(double sample) {
	t_uint16 i;
	for(i=0; i<active_frames; i++) {
		frame[i][frame_index[i]] = sample;
		frame_index[i]++;
		frame_index[i]%=WINDOW_SIZE;
	}
}

void Raskell::reset_mfcc() {
	frame.clear();
	frame.resize(active_frames);
	for (int i=0; i<active_frames; i++)
		frame[i].resize(WINDOW_SIZE);

	frame_index.clear();
	frame_index.reserve(active_frames);
	for(t_uint16 i=0; i<active_frames; i++)
		frame_index.push_back(i * HOP_SIZE);
}

void Raskell::calc_mfcc(t_uint16 frame_to_fft) {
	t_uint16 i, j;//, K=(WINDOW_SIZE/2)+1;
	for(i=0; i<WINDOW_SIZE; i++) {
		in[i] = frame[frame_to_fft][i];
	}
	fftw_execute(plan); // make FFT

	for (int i = 0; i < m; i++) {
		logEnergy[i] = 0.0;
		for (int j = 0; j < WINDOW_SIZE/2; j++) {
			logEnergy[i] += sqrt(out[j][0]*out[j][0]+out[j][1]*out[j][1])*banks[i][j];
		}
		//take log
		logEnergy[i] = log10(logEnergy[i]);
	}
	
	//take DCT
	fftw_execute(dct);
}

// ========================= DTW ============================

void Raskell::init_dtw() {
	outlet_int(max->out_t, t); // t==0 in the beginning
	outlet_int(max->out_h, h);
	distance(t, h); // calculate Dist[t_mod][h_mod]

	for (t_uint16 i=0; i<fsize; i++) {
		for (t_uint16 j=0; j<fsize; j++) {
			dtw[i][j] = VERY_BIG;
		}
	}
	dtw[t_mod][h_mod] = Dist[t%bsize][h%bsize]; // initial starting point

	increment_t();
	increment_h();
}

void Raskell::distance(t_uint16 i, t_uint16 j) {
	t_uint16 k, imod = i%bsize, jmod=j%bsize;
	// build distance matrix
	double total;
	total = 0;

	if ((x[imod][0]==0)||(y[j][0]==0)) { // if either is undefined
		total = VERY_BIG;
		//post("WARNING input is zero, t=%i h=%i", i, j);
	} else for (k = 0; k < params; k++) {
		total = total + ((x[imod][k] - y[j][k]) * (x[imod][k] - y[j][k])); // distance computation 
	}
	if (total < 0.0001)
		total = 0;
	total = sqrt(total);
	total += ALPHA;

	Dist[imod][jmod] = total;
	//post("Dist[%i][%i] = %f", imod, jmod, total);
}

void Raskell::dtw_process() {
	short debug = 0;
	t_uint16 inc = get_inc();
	t_uint16 j, jstart;
	if(debug) post("next 1:row/2:column/3:both : %i", inc);

	// it's possible to have several Y/h hikes for each X/t feature:
	while((inc == NEW_ROW) && (h < ysize) && (h != markers[0][0] + fsize-1)) {
		outlet_int(max->out_h, h);

		if (t<bsize)	jstart = 0;
		else			jstart = t-bsize;
		for (j = jstart; j < t; j++) {
			//if(debug) post("calc HAP! j=%i h=%i", j, h);
			distance(j, h); // calculate distance for new row
			//if ((j>0)&&(h>0)) 
			calc_dtw(j, h); // calc DTW for new row
		}
		increment_h();
		previous = 1;
		runCount++;
		inc = get_inc(); // get new inc
		if(debug) post("HAP! next 1:row/2:column/3:both : %i", inc);
	}
				
	if (h < ysize) { // unless we've reached the end of Y...
					
		if (inc != NEW_ROW) { // make new Column
			// build X matrix (online)
			outlet_int(max->out_t, t);
			if (h<bsize)	jstart = 0;
			else			jstart = h-bsize;

			for(j=jstart; j<h; j++) {
				//if (j>=0) {
				distance(t, j); // calculate distance for new column
				calc_dtw(t, j); // calc DTW for new column 
				//}
			}
			increment_t();
		}
		if (inc != NEW_COL) { // make new Row
			outlet_int(max->out_h, h);
			if (t<bsize)	jstart = 0;
			else			jstart = t-bsize;
			for (j=jstart; j<t; j++) {
				distance(j, h); // calculate distance for new row
				//if ((j>0)&&(h>0)) 
				calc_dtw(j, h); // calc DTW for new row
			}
			increment_h();
		}
		if (inc == previous)
			runCount++;
		else
			runCount = 1;
		if (inc != NEW_BOTH)
			previous = inc;
	}
	else { // h = ysize
		post("End reached!");
	//	input(0);
		RVdtw_stop(max, 0);
	}
}

t_uint16 Raskell::get_inc() {
	if (!follow) return NEW_BOTH; // if alignment is OFF, just go diagonally

	// helper function for online DTW, choose Row (h) / Column (t) incrementation
	t_uint16 i, next = 0;
	t_uint16 tmin1 = (t_mod+fsize-1) % fsize;
	t_uint16 hmin1 = (h_mod+fsize-1) % fsize;
	t_uint16 tmin2 = (t_mod+fsize-2) % fsize;
	t_uint16 hmin2 = (h_mod+fsize-2) % fsize;
	double min = VERY_BIG;

	if (runCount > maxRunCount) {// tempo limit reached...
		post("MAXRUNCOUNT");
		if (previous == NEW_ROW)
			return NEW_COL;
		else
			return NEW_ROW;
	}

	for (i=0; i<fsize; i++) // if minimum is in row h..
		if(dtw[i][hmin1] < min) {
			min = dtw[i][hmin1];
			next = NEW_ROW; // ..then increment row next
		}
	for (i=0; i<fsize; i++) // if minimum is in column t..
		if(dtw[tmin1][i] <= min) {
			min = dtw[tmin1][i];
			next = NEW_COL; // ..then increment column next
		}
	if(dtw[tmin1][hmin1] <= min) // otherwise...
		next = NEW_BOTH; // ... increment BOTH.
	//post("min: %f", min);
	return next;
}

void Raskell::calc_dtw(t_uint16 i, t_uint16 j) {
	// calculate DTW matrix
	double top, mid, bot, cheapest;
	t_uint16 imin1 = (i+fsize-1) % fsize;
	t_uint16 imin2 = (i+fsize-2) % fsize;
	t_uint16 jmin1 = (j+fsize-1) % fsize;
	t_uint16 jmin2 = (j+fsize-2) % fsize;
	t_uint16 imod = i % fsize;
	t_uint16 jmod = j % fsize;		

	top = dtw[imin2][jmin1] + Dist[i%bsize][j%bsize] * side_weight;
	mid = dtw[imin1][jmin1] + Dist[i%bsize][j%bsize] * mid_weight;
	bot = dtw[imin1][jmin2] + Dist[i%bsize][j%bsize] * side_weight;

	if( (top < mid) && (top < bot))	{ 
		cheapest = top;
		}
	else if (mid <= bot) {
		cheapest = mid;
		}
	else { 
		cheapest = bot;
		}
	dtw[imod][jmod] = cheapest;
	//post("dtw[%i][%i] = %f", i, j, cheapest);
}

void Raskell::dtw_back() {
	short debug = 0;
	b_start = t % bsize;
	history[b_start] = h % bsize;
	if (t >= bsize && h >= bsize) {
		double top, mid, bot, cheapest;
		t_uint16 i, j;
		Deque.clear();		
		if(debug) post("b_start is %i", b_start);
		b_dtw[b_start][history[b_start]] = Dist[b_start][history[b_start]]; // starting point
		b_move[b_start][history[b_start]] = NEW_BOTH;

		// compute backwards DTW (circular i--), and b_move
		for (i = b_start+bsize-1 % bsize; i != b_start; i = (i+bsize-1) % bsize) { 
			for (j = history[b_start]+bsize-1 % bsize; j != history[b_start]; j = (j+bsize-1) % bsize) { 
				t_uint16 imod = i % bsize;
				t_uint16 imin1 = (i+1) % bsize;		
				t_uint16 imin2 = (i+2) % bsize;
				t_uint16 jmod = j % bsize;
				t_uint16 jmin1 = (j+1) % bsize;
				t_uint16 jmin2 = (j+2) % bsize;

				top = b_dtw[imin2][jmin1] + Dist[imod][jmod] * side_weight;
				mid = b_dtw[imin1][jmin1] + Dist[imod][jmod] * mid_weight;
				bot = b_dtw[imin1][jmin2] + Dist[imod][jmod] * side_weight;

				if( (top < mid) && (top < bot))	{ 
					cheapest = top; b_move[imod][jmod] = NEW_ROW;
					}
				else if (mid <= bot) {
					cheapest = mid; b_move[imod][jmod] = NEW_BOTH;
					}
				else { 
					cheapest = bot; b_move[imod][jmod] = NEW_COL;
					}
				b_dtw[imod][jmod] = cheapest;
			}
		}
		i = (b_start+bsize-1) % bsize; 
		j = (history[b_start]+bsize-1) % bsize; 
		int p = 0;
		while (i != b_start && j != history[b_start]) {
			if (b_move[i][j] == NEW_ROW) {
				j = (j+bsize-1) % bsize;
			}
			else if (b_move[i][j] == NEW_COL) {
				i = (i+bsize-1) % bsize;
			}
			else if (b_move[i][j] == NEW_BOTH) {
				i = (i+bsize-1) % bsize; 
				j = (j+bsize-1) % bsize;
			}
			b_path[p][0] = i; b_path[p][1] = j;
			p++;
		}
		// TODO: check how B-dtw destination relates to the regular dtw one:
		if(debug) {
			if (i == b_start && j == history[b_start]) {
				post("path confirmed!");
			} else if (i == b_start) {
				post("T high");
			} else post("H high");
		}

		double Min = VERY_BIG, Sum;
		int K = 3, L = 10;
		for (i = 0; i < p - K; i++) {
			if (i < L) { // http://www.infoarena.ro/deque-si-aplicatii
				bool popped = false;
				while (!Deque.empty() && b_dtw[b_path[i][0]][b_path[i][1]] <= b_dtw[b_path[Deque.front()][0]][b_path[Deque.front()][1]] ) {
					Deque.pop_front();   
					popped = true;
				}
				if (popped || Deque.empty()) Deque.push_front(i);
			}
			Sum = b_dtw[b_path[Deque.front()][0]][b_path[Deque.front()][1]] + b_dtw[b_path[i+K][0]][b_path[i+K][1]];
			if (Sum < Min) {
				tempo_prob = Min = Sum;
				pivot1_t = b_path[Deque.front()][0];
				pivot1_h = b_path[Deque.front()][1];
				pivot2_t = b_path[i+K][0];
				pivot2_h = b_path[i+K][1];
				// recover wrap around bsize:
				if (pivot1_t < pivot2_t) pivot1_t += bsize;
				if (pivot1_h < pivot2_h) pivot1_h += bsize;
			}
		}
	}
}

void Raskell::increment_t() {/*
	history[t_mod] = h;
	// calculate tempo multiplier based on window history:
	if (t == 0) tempo = 1;
	else {
		if (t < xsize) {
			t_uint16 start = 0;
			tempo = (float)(history[t_mod] - history[start]) / (float)(t);
		}
		else {
			t_uint16 start = (t_mod+1)%xsize;
			tempo = (float)(history[t_mod] - history[start]) / (float)(xsize-1);
		}
	}

	outlet_float(max->out_tempo, tempo);*/

	// move on to next t
	t++;
	t_mod = (t_mod+1)%fsize;
}

void Raskell::increment_h() {
	h++;
	h_mod = (h_mod+1)%fsize;
	if (h == markers[m_iter][0]) {
		markers[m_iter][M_LIVE] = t; // marker detected at time "t"
		post("marker detected at t = %i, h = %i", t, h);
		atom_setsym(dump, gensym("marker"));
		atom_setlong(dump+1, m_iter);
		outlet_list(max->out_dump, 0L, 2, dump);
		if (m_iter < m_count)
			m_iter++;
		else {
			post("End reached!");
			//input(0);
			RVdtw_stop(max, 0);
		}
			
	}
}

// ========================= FILE i/o =================
bool Raskell::do_read(t_symbol *s) {
	//post("reading");
	short vol,err;
	char ps[MAX_PATH_CHARS];
	t_fourcc type;

	file_close();
	if (s==ps_nothing) {
		if (open_dialog(ps,&vol,&type,0L,0))
			return false;
	} else {
		strcpy(ps,s->s_name);
		if (locatefile_extended(ps,&vol,&type,&type,-1)) {
			post("No score file exists, accessing buffer~");
			return false;
		}
	}
	err = path_opensysfile(ps,vol,&f_fh,READ_PERM);
	if (err) {
		object_error((t_object *)this, "%s: error %d opening file",ps,err);
		return false;
	}
	file_open(ps);
	return true;
}

void Raskell::file_open(char *name) {
	t_ptr_size size;
	if (f_spool)
		f_open = TRUE;
	else {
		sysfile_geteof(f_fh, &size);
		if (!(f_data = (t_uint16 **)sysmem_newhandle(size)))
			object_error((t_object *)this, "%s too big to read", name);
		else {
			sysmem_lockhandle((t_handle)f_data,1);
			sysfile_readtextfile(f_fh, (t_handle)f_data, 0, TEXT_NULL_TERMINATE);
			//post("the file has %ld characters", size);
			f_size = size;
			if(input_sel != IN_LIVE) {
				score_name = name;
				// start reading SCORE frames				
				// count # of mfcc's:
				long lines=0;
				char *eol = strchr((char*)f_data[0], '\n');
				while(eol != NULL) {
					lines++;
					eol=strchr(eol+1,'\n'); // read next line
				}
				score_size(lines);
				// enter frames and markers:
				RVdtw_read_line(max, 0);
			} 
			else {
			// read LIVE frames: instant simulation
				live_name = name;
				RVdtw_read_line(max, 0);
			}
		}
		sysfile_close(f_fh);
	}
	f_spool = FALSE;
}

bool Raskell::read_line() {	
	double f_mfcc[50];
	t_uint16 i;
	char buf[1024];
	for(i=0; i<params; i++)
		f_mfcc[i] = 0;
	while ( sgets( buf, sizeof( buf )) ) { //&& i) {
		int length = sscanf(buf, "%lf, %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
			&f_mfcc[0], &f_mfcc[1], &f_mfcc[2], &f_mfcc[3], &f_mfcc[4], 
			&f_mfcc[5], &f_mfcc[6], &f_mfcc[7], &f_mfcc[8], &f_mfcc[9],  
			&f_mfcc[10], &f_mfcc[11], &f_mfcc[12], &f_mfcc[13], &f_mfcc[14], 
			&f_mfcc[15], &f_mfcc[16], &f_mfcc[17], &f_mfcc[18], &f_mfcc[19], 
			&f_mfcc[20], &f_mfcc[21], &f_mfcc[22], &f_mfcc[23], &f_mfcc[24], 
			&f_mfcc[25], &f_mfcc[26], &f_mfcc[27], &f_mfcc[28], &f_mfcc[29],
			&f_mfcc[30], &f_mfcc[31], &f_mfcc[32], &f_mfcc[33], &f_mfcc[34], 
			&f_mfcc[35], &f_mfcc[36], &f_mfcc[37], &f_mfcc[38], &f_mfcc[39], &f_mfcc[40]);
		if(length > (params+1) && input_sel != IN_LIVE) {
			post("NOTE: number of params increased to %i", params);
			params = length-1; // set number of parameters (increase if necessary)
			for (i=0; i<bsize; i++) {
				x[i].resize(params);
			}
			for (i=0; i<ysize; i++) {
				y[i].resize(params);
			}
			mfcc_frame = (t_atom*)sysmem_resizeptr(mfcc_frame, params * sizeof(t_atom_float));
		}

		for(t_uint16 i=0; i<params; i++) 
			//atom_setfloat(mfcc_frame+i-1, f_mfcc[i]); // f_mfcc[0] = order no.
			tmfcc[i] = f_mfcc[i+1];
		// if signal is loud enough, then compress the first MFCC coefficient (loudness)
		if (tmfcc[0] > COMP_THRESH) 
			tmfcc[0] = compress(tmfcc[0], true);//tmfcc[0] /= 8;
		else tmfcc[0] = compress(tmfcc[0], false);
		//post("min is %.2f", min0);
		//tmfcc[0]=0.1;
		mfccs(params); 
		if(strstr(buf, "marker"))//-buf==0) // marker
			marker(NULL);
	}
	return false;
}

double Raskell::compress(double value, bool active) {
	static t_uint16 timer = 1;
	if (active) {
		value = COMP_THRESH + (value - COMP_THRESH) * COMP_RATIO;
		timer = 1;
	}		
	else {
		if (timer < COMP_RELEASE) {
			float ratio = (COMP_RELEASE - timer) / COMP_RELEASE * (COMP_RATIO-1) + 1;
			value = COMP_THRESH + (value - COMP_THRESH) * ratio;
			timer++;
			//post("RELEASING %i value: %.2f", timer, value);
		}		
	}
	if (value > 0) value == 0;
	return value;
}

void Raskell::file_close() {
	if (f_open) {
		sysfile_close(f_fh);
		f_fh = 0;
		f_open = FALSE;
	}
	if (f_data) {
		sysmem_lockhandle((t_handle)f_data,0);
		sysmem_freehandle((t_handle)f_data);
		f_data = 0;
	}
}

char* Raskell::sgets(char *str, int num) {
	// helper function, emulate fgets via http://stackoverflow.com/questions/2068975/can-cs-fgets-be-coaxed-to-work-with-a-string-not-from-a-file
	char *next = (char*)f_data[0];
	int  numread = 0;

	while ( numread + 1 < num && *next ) {
		int isnewline = ( *next == '\n' );
		*str++ = *next++;
		numread++;
		// newline terminates the line but is included
		if ( isnewline )
			break;
	}

	if ( numread == 0 )
		return NULL;  // "eof"

	// must have hit the null terminator or end of line
	*str = '\0';  // null terminate this string
	// set up input for next call
	strcpy((char*)f_data[0], next);
	return str;
}

// ==================

void Raskell::do_write(t_symbol *s) {
	t_fourcc filetype = 'TEXT', type;
	short vol, err;
	char filename[512];

	// prepare buffer
	long long i, j;
	string buf = "";
	if (input_sel != IN_LIVE) { // in SCORE mode, save the score
		strcpy(filename, "score.txt");
		for (i = 0; i < ysize; i++) {
			buf += to_string(i) + ",";
			for (j = 0; j < params; j++) {
				buf += " " + to_string((long double)y[i][j]);
			}
			buf += ";\n";
		}
	}
	else { // in LIVE mode, save the test results
		filetype = 'CSV';
		strcpy(filename, "trace");
		buf+="Parameters:, DTW size, FFT size, FFT hop, ALPHA\n,";
		buf+= to_string((long long)fsize) + ", " + to_string((long long)WINDOW_SIZE) + 
			", " + to_string((long long)HOP_SIZE) + ", " + to_string((long long)ALPHA);
		
		buf+="\nMarker Trace Results for," + score_name + ", vs, " + live_name;
		buf+="\nID, Score pos, HOOK, Live ideal, Live detected";
		for (i = 0; i <= m_count; i++) {
			buf += "\n" + to_string(i);
			for (j = 0; j < 4; j++)
				buf += ", " + to_string((long long)markers[i][j]);
		}
	}

	//file_close();
	if (s==ps_nothing) {
		if(saveasdialog_extended(filename, &vol, &type, &filetype, 1))
			return;
	} else {
		strcpy(filename,s->s_name);
		vol = path_getdefault();
	}
	
	// convert C++ string to C string
	char * cstr = new char [buf.length()+1];
	strcpy(cstr, buf.c_str());
	t_ptr_size size = strlen(cstr);

	if (filetype == 'CSV') strcat(filename, ".csv");

	err = path_createsysfile(filename, vol, 'TEXT', &f_fh);
    if (err)
        return;

	err = sysfile_write(f_fh, &size, cstr);
	if (err) {
		object_error((t_object *)this, "%s: error %d writing file", filename, err);
		return;
	}
	else post("file written: %s", filename);
	sysfile_close(f_fh);
}

// ====================

void Raskell::set_buffer(t_symbol *s) {

	if (!l_buffer_reference)
		l_buffer_reference = buffer_ref_new((t_object*)max, s);
	else
		buffer_ref_set(l_buffer_reference, s);
	if (buffer_ref_exists(l_buffer_reference))
		post("loaded buffer %s", s->s_name);
	buf_name = s;

	t_buffer_obj *b = buffer_ref_getobject(l_buffer_reference);

	if (b) {
		double msr = buffer_getsamplerate(b);
		if (msr != SampleRate) {
			object_error((t_object *)this, "Buffer contents are wrong sample rate! Please load a %.1f Hz file, or adjust the system rate",
				SampleRate);
			return;
		}
		long frames = buffer_getframecount(b);
		post("Buffer is %d samples long", frames);
		if (input_sel == IN_SCORE) {// make new score
			score_size((long)(frames / HOP_SIZE - active_frames + 1));
			iter = 1;
			marker(NULL); // add marker at start (position 1)
		}
			

		int sampleframes = 64;

		float* sample = buffer_locksamples(b);
		
		if (sample)
			for (long i = 0; i < frames; i++) {
				double samp = sample[i];
				perform(&samp, 1);
			}
		buffer_unlocksamples(b);
	}

}