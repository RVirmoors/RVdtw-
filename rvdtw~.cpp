// rvdtw~.cpp
//
// 2014-2016 Grigore Burloiu

#include <rvdtw~.h>

extern "C"
{

int C74_EXPORT main(void) {	
	t_class *c = class_new("RVdtw~", (method)RVdtw_new, (method)RVdtw_free, (long)sizeof(t_RVdtw), 
		0L, A_GIMME, 0);
	class_dspinit(c);
	
	class_addmethod(c, (method)RVdtw_feats,		"feats",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_input,		"input",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_scoresize,	"score_size",	A_GIMME, 0); // not needed for users
	class_addmethod(c, (method)RVdtw_read,		"read",		A_DEFSYM,0);
	class_addmethod(c, (method)RVdtw_readacco,	"readacco",	A_DEFSYM,0);
	class_addmethod(c, (method)RVdtw_write,		"write",	A_DEFSYM,0);

	class_addmethod(c, (method)RVdtw_gotomarker, "gotomarker",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_gotoms,	"gotoms",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_start,		"start",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_stop,		"stop",		A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_follow,	"follow",		A_GIMME, 0);	
	class_addmethod(c, (method)RVdtw_sensitivity,	"sensitivity",	A_GIMME, 0);
	class_addmethod(c, (method)RVdtw_elasticity,	"elasticity",	A_GIMME, 0);

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
		x->out_feats = listout((t_object *) x);
		x->out_dump = listout((t_object *) x);
		x->out_h = intout((t_object *) x);
		x->out_t = intout((t_object *) x);
		x->rv->init(s, argc, argv);
	}
	return (x);
}

void RVdtw_free(t_RVdtw *x) {
	delete x->rv->beat;
	x->rv->beat = NULL;
	delete x->rv->chroma;
	x->rv->chroma = NULL;

	dsp_free((t_pxobject*)x);
	object_free(x->rv->l_buffer_reference);
}


//***********************************************************************************************

void RVdtw_assist(t_RVdtw *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET)			// inlet
    { 
                sprintf(s, "int: set length, feats: add frame");
    } 
    else							// outlets
    {	
		switch (a) 
        {
            case 0: sprintf(s, "t"); break;
            case 1: sprintf(s, "h"); break;
            case 2: sprintf(s, "misc dump"); break;                   
			case 3: sprintf(s, "computed feat frames"); break;
			case 4: sprintf(s, "tempo multiplier"); break;
        }	
    }
}

void RVdtw_feats(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	for (long i = 0; i < argc; i++)
		x->rv->tfeat[i] = atom_getfloat(argv+i);
	x->rv->feats(argc);
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
	atom v[1];
	atom_setlong(v, B_SOLO),
	defer_low(x,(method)RVdtw_do_read,s,1,v);
}

void RVdtw_readacco(t_RVdtw *x, t_symbol *s) {
	x->rv->acc_beats.clear();
	x->rv->acc_beats.resize(2);
	x->rv->acc_iter = 0;	
	x->rv->beat->updateHopAndFrameSize(HOP_SIZE, HOP_SIZE*2);
	atom v[1];
	atom_setlong(v, B_ACCO),
	defer_low(x,(method)RVdtw_do_read,s,1,v);
}

void RVdtw_do_read(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	long v = atom_getlong(argv); // B_SOLO or B_ACCO
	if (!x->rv->do_read(s)) // if input is not a text file
		x->rv->set_buffer(s, v); // then treat it as a buffer
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

void RVdtw_sensitivity(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	x->rv->sensitivity = atom_getfloat(argv);
	post("Sensitivity to tempo fluctuations is %f", x->rv->sensitivity);
}

void RVdtw_elasticity(t_RVdtw *x, t_symbol *s, long argc, t_atom *argv) {
	x->rv->elasticity = atom_getfloat(argv);
	post("Elasticity of tempo model is %f", x->rv->elasticity);
}

// this lets us double-click to open up the buffer~ it references
void RVdtw_dblclick(t_RVdtw *x) {
	buffer_view(buffer_ref_getobject(x->rv->l_buffer_reference));
}

// this handles notifications when the buffer appears, disappears, or is modified.
t_max_err RVdtw_notify(t_RVdtw *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
	if (msg == x->rv->ps_buffer_modified && x->rv->input_sel == IN_SCORE)
		x->rv->set_buffer(x->rv->buf_name, B_SOLO);
	return buffer_ref_notify(x->rv->l_buffer_reference, s, msg, sender, data);
}



//***********************************************************************************************

// this function is called when the DAC is enabled, and "registers" a function for the signal chain in Max 5 and earlier.
// In this case we register the 32-bit, "RVdtw_perform" method.
void RVdtw_dsp(t_RVdtw *x, t_signal **sp, short *count)
{
	//post("my sample rate is: %f", sp[0]->s_sr);
	x->rv->beat->updateHopAndFrameSize(sp[0]->s_n, sp[0]->s_n*2);
	dsp_add(RVdtw_perform, 3, x, sp[0]->s_vec, sp[0]->s_n); // ins & sampleframes
}


// this is the Max 6 version of the dsp method -- it registers a function for the signal chain in Max 6,
// which operates on 64-bit audio signals.
void RVdtw_dsp64(t_RVdtw *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	//post("my sample rate is: %f", samplerate);
	x->rv->beat->updateHopAndFrameSize(maxvectorsize, maxvectorsize*2);
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

	x->rv->perform((double *)in, n, B_SOLO);
	// you have to return the NEXT pointer in the array OR MAX WILL CRASH
	return w + 5;
}


// this is 64-bit perform method for Max 6
void RVdtw_perform64(t_RVdtw *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	t_double *in = ins[0];		// we get audio for each inlet of the object from the **ins argument
	int n = sampleframes;
	x->rv->perform(in, n, B_SOLO);
}


} // end extern C

		

Raskell::Raskell() {	
		//ysize = t = h = h_real = runCount = iter = m_iter = m_ideal_iter = t_mod = h_mod = 0; // current position for online DTW: (t,h)
		//b_avgerr = b_start = bh_start = 0;
		tempo = tempo_avg = 1;
		tempotempo = 0;
//		m_count = 0; // one marker is mandatory (to start)
//		previous = 0; // 0 = none; 1 = Row; 2 = Column; 3 = Both
		input_sel = IN_SCORE; // 1 = SCORE; 2 = LIVE; 0 = closed
		follow = true;
		sensitivity = SEN; // sens. to tempo fluctuations
		elasticity = ELA;
    
		elast_beat = 1;
        beat_due = false;
        acc_iter = b_iter = prev_h_beat = iter = 0;
        ref_tempo = 120;
		integral = t_passed = last_arzt = 0;
		tempos.clear(); errors.clear();
		y_beats.clear();
		y_beats.resize(2);
		acc_beats.clear();
		acc_beats.resize(2);

		SampleRate = sys_getsr();
		active_frames = WINDOW_SIZE / HOP_SIZE;
		dsp_frame_size = 64;

		beat = new BTrack();
		chroma = new Chromagram(WINDOW_SIZE, SampleRate); 
		chroma->setChromaCalculationInterval(WINDOW_SIZE);
        warp = new oDTW();

		l_buffer_reference = NULL;
		score_name = live_name = "";
		
		features = CHROMA;
		tempo_model = T_PID;
        params = m = 12;
//		tfeat = malloc(m * sizeof(double));
    
#ifdef USE_FFTW
//		samp = fftw_alloc_real(WINDOW_SIZE);
						
		if (features == MFCCS) {			
			params = 40; // # of feats to be used
			// number of frames that will be computed at any one time:
			m = 48; // number of Mel filterbanks
			in = fftw_alloc_real(WINDOW_SIZE);
			logEnergy = fftw_alloc_real(m);		// 48 filter banks
			out = fftw_alloc_complex(WINDOW_SIZE/2 + 1);
			tfeat = fftw_alloc_real(m);
			plan = fftw_plan_dft_r2c_1d(WINDOW_SIZE, in, out, FFTW_MEASURE); // FFT real to complex FFTW_MEASURE 
			dct = fftw_plan_r2r_1d(m, logEnergy, tfeat, FFTW_REDFT10, NULL);
		} else if (features == CHROMA) {
			tfeat = fftw_alloc_real(m);
		}
#endif

		dspinit();
		//post ("RV object created");
}


Raskell::~Raskell() {
#ifdef USE_FFTW
	fftw_destroy_plan(plan);
    fftw_free(in); fftw_free(out);
	fftw_free(logEnergy);
	fftw_free(tfeat);
	fftw_cleanup();
#endif
		//file_close();
}

void Raskell::init(t_symbol *s,  long argc, t_atom *argv) {
    f_open = FALSE;
    f_fh = 0;
    f_spool = 0;
    f_data = 0;
    ps_nothing = gensym("");
    ps_buffer_modified = gensym("buffer_modified");
    
    post("RVdtw version %.2f", RV_VERSION);
    
    buf_name = (argc) ? atom_getsym(argv) : gensym("");
    
    if (argc) {
        if(!do_read(buf_name))
            set_buffer(buf_name, B_SOLO); // first argument
    }
    else post("no score preloaded");
    
    fsize = warp->getFsize();
    
    srand(744);
}

bool Raskell::zeros(double *in, long sampleframes) {
	for (long i = 0; i < sampleframes; i++) {
		if (in[i]) return false;
	}
	return true;
}

void Raskell::perform(double *in, long sampleframes, int dest) {

	beat->processAudioFrame(in);
//	if(beat->beatDueInCurrentFrame())
//		post("beat! t= %f", beat->getCurrentTempoEstimate());
	
	if(beat->beatDueInCurrentFrame()) { // beat detected
		switch (dest) {
		case (B_SOLO) : 
			if (input_sel != IN_LIVE && iter) { // looking at reference
				y_beats[0].push_back(iter);  // beat pos
				y_beats[1].push_back(0);	// diff from acco beat (computed below in feats())

			} else if(y_beats[0].size() && h_real) { // looking at live target
				float diff_y, diff_acc;
				//post("live beat! %d", h);
				// check if live beat aligns with predicted beats
				b_iter = update_beat_iter(b_iter, &y_beats[0], h_real);
				diff_y = calc_beat_diff(h_real, prev_h_beat, y_beats[0][b_iter]);
				if (acc_beats[0].size()) {
					acc_iter = update_beat_iter(acc_iter, &acc_beats[0], h_real);
					diff_acc = calc_beat_diff(h_real, prev_h_beat, acc_beats[0][acc_iter]);
                    
                    ref_tempo = acc_beats[1][acc_iter];
                    
                    // average the last 3 diffs between Y and ACCO beats
                    minerr = 0;
                    int i = 0;
                    if (b_iter > 3)
                        i = b_iter - 3;
                    for(; i <= b_iter; i++) {
                        minerr += y_beats[1][b_iter];
                    }
                    minerr /= 3;
                    // output minerr
                    atom_setsym(dump, gensym("beat_err"));
                    atom_setlong(dump+1, minerr);
                    outlet_list(max->out_dump, 0L, 2, dump);
                    
				} else 
					diff_acc = VERY_BIG;				
				prev_h_beat = h_real;

				beat_due = true;
			}
			break;

		case (B_ACCO) : // looking at accompaniment
			if (acc_iter) {
				acc_beats[0].push_back(acc_iter);
				acc_beats[1].push_back(beat->getCurrentTempoEstimate());
			}
			//post("tempo %f at beat %d", beat->getCurrentTempoEstimate(), acc_iter);
			break;
		}	
	}

//	if (dest == B_ACCO || warp->isRunning()) {
		
		//post("in %f", in[0]);
		std::vector<double> chr;
		t_uint32 i, j;
		//post("adding %d samples to frames of %d size", sampleframes, WINDOW_SIZE);
		for (i=0; i < sampleframes; i++) {
			add_sample_to_frames(in[i]);
		}
		for(i=0; i<active_frames; i++) {
			if(frame_index[i]==0) {		// if frame is full, then compute feature vector
				//post("frame is full # %d. first sample: %f", iter, frame[i][0]);
				if (dest != B_ACCO) {
					if (zeros(in, sampleframes)) {
						for (i=0; i<params; i++)
							tfeat[i] = 0.0001;
					}
					else {
						switch (features) {
							case (MFCCS) :	
								for(j=0; j<WINDOW_SIZE; j++)
									frame[i][j] *= window[j]; // apply hamming window
								calc_mfcc(i); // get tfeat[]
								// if signal is loud enough, then compress the first MFCC coefficient (loudness)
								if (tfeat[0] > COMP_THRESH) 
									tfeat[0] = compress(tfeat[0], true);//tfeat[0] /= 8;
								else tfeat[0] = compress(tfeat[0], false);
								break;					
							case (CHROMA) :
								chroma->processAudioFrame(frame[i]);
								if (chroma->isReady()) {
									chr = chroma->getChromagram();						
									tfeat = &chr[0];
								}
								break;
						}
					}

					feats(params);
				}
				if (dest == B_ACCO)
					acc_iter++; // count frames for ACCO beat marking
			}
		}
//	}
}

t_uint16 Raskell::update_beat_iter(t_uint16 beat_iter, vector<float> *beat_vector, double ref_beat) {
	if (beat_iter < beat_vector->size()-1) {
		// while the next beat is closer than the current one, increment iterator
		while( (beat_iter < beat_vector->size()-1) && 
			abs(ref_beat - beat_vector->at(beat_iter)) > abs(ref_beat - beat_vector->at(beat_iter+1)) )
			beat_iter++;
	}
//	post("iter %d", beat_iter);
	return beat_iter;
}

int Raskell::calc_beat_diff(double cur_beat, double prev_beat, double ref_beat) {
	int newdiff = cur_beat - ref_beat;
    if (abs(newdiff) > (cur_beat - prev_beat)/4) { // reverse phase
		if (cur_beat < ref_beat)
			newdiff = cur_beat + (cur_beat - prev_beat)/2 - ref_beat;
		else
			newdiff = cur_beat - (cur_beat - prev_beat)/2 - ref_beat;
    }
	return newdiff;
}

void Raskell::feats(t_uint16 argc) {
	//post("processing feats %f ", tfeat[0]);

	if ((ysize > 0) && (input_sel > 0))  { // Y matrix was init'd, let's populate it
		if (params != argc) {
			post("new number of params: %d", argc);
			params = (t_uint16)argc;
            warp->setParams(params);
			score_size(ysize);
		}

		if (input_sel == IN_SCORE) { // build Y matrix (offline)
            iter = warp->processScoreFV(tfeat);
           // post("processed frame: %d -> %f", iter, tfeat[0]);
            
            if (warp->isScoreLoaded()) { // if iter == ysize
                post("SCORE data fully loaded: %i frames, %i ACC beats, %i Y beats.",
                     ysize, acc_beats[0].size(), y_beats[0].size());
                input(0);
                if (acc_beats[0].size()) {
                    // compare Y beats & ACC beats
                    b_iter = acc_iter = 0;
                    float diff = 0;
                    while (b_iter < y_beats[0].size() && acc_iter < acc_beats[0].size()) {
                        b_iter = update_beat_iter(b_iter, &y_beats[0], int(acc_beats[0][acc_iter]));
                        if (b_iter)
                            y_beats[1][b_iter] = calc_beat_diff(y_beats[0][b_iter],
                                                                y_beats[0][b_iter-1], acc_beats[0][acc_iter]);
                        else
                            y_beats[1][b_iter] = y_beats[0][b_iter] - acc_beats[0][acc_iter];
                        diff = (float)(y_beats[1][b_iter] + acc_iter * diff) / (acc_iter + 1); // CMA
                        acc_iter++;
                    }
                    post ("average diff between Y and ACC beats: %f", diff);
                    b_stdev = minerr = 0;
                    for ( int i = 0; i < b_iter; i++)
                        b_stdev += (diff-y_beats[1][i]) * (diff-y_beats[1][i]);
                    b_stdev /= y_beats[0].size();
                    b_stdev = sqrt(b_stdev);
                    post ("std deviation between Y and ACC beats: %f", b_stdev);
                }
            }
		}
	
		if (input_sel == IN_LIVE) { // new X feature entered
			iter++;
            warp->processLiveFV(tfeat);
			// build X matrix (online)
            //post ("h = %i, ysize = %i", warp->getH(), ysize);

            if (warp->isRunning()) {
                // calculate next DTW step
                t_uint16 t = warp->getT();
                t_uint16 h = warp->getH();
                outlet_int(max->out_t, t);
                if (t && (h >= warp->getHistory(t-1)))
                    outlet_int(max->out_h, h);
                // output certainty
                atom_setsym(dump, gensym("b_err"));
//                atom_setlong(dump+1, b_err[b_start][0]); // TODO
//                atom_setfloat(dump+2, b_avgerr);
                atom_setfloat(dump+3, tempo_avg);
                outlet_list(max->out_dump, 0L, 4, dump);
                
                b_err = warp->getBackPath();
                bsize = b_err.size();
//                post("bsize is %i", bsize);
                b_start = t % bsize;
                
                int step = bsize / 4;
                
                if (t > step * 2) {
                    tempo_avg += b_err[(b_start- step   +bsize)%bsize][3] / step;
                    tempo_avg -= b_err[(b_start-(step*2)+bsize)%bsize][3] / step;
                }
                
                // choose tempo model: oDTW-based or beat-based
                beat_switch();
            }
            else {
                post("End reached!");
                //	input(0);
                RVdtw_stop(max, 0);
            }
			
		}	// end input_sel==2 check
	} // end input_sel>0 check
}

void Raskell::score_size(long v) {
	if ((ysize = warp->setScoreSize(v))) {
		post("Score Matrix memory alloc'd, size %i * %i", ysize, params);
        y_beats.clear();
        y_beats.resize(2);
	}
	else post("wrong/missing length");
}

void Raskell::marker(t_symbol *s) {
	// add new marker
	if ((input_sel == IN_SCORE) && ysize) {
        warp->addMarkerToScore(iter);
	}
    else if (input_sel == IN_LIVE) {
        unsigned int i = warp->addMarkerToLive(iter);
//        post("Marker (ideal) entered at position %i : %i", i, warp->getMarker(i-1, M_IDEAL));
	}
}

void Raskell::reset() {
    warp->start();

    iter = h_real = 0;
    tempo = tempo_avg = 1;
    tempotempo = 0;
	integral = t_passed = last_arzt = 0;	
	tempos.clear(); errors.clear();
	acc_iter = b_iter = prev_h_beat = 0;
	tempo_mode = 0;

	reset_feat();

	pivot1_t = pivot1_h = pivot2_t = pivot2_h = 0;
	pivot1_tp = pivot2_tp = 1;
    
    outlet_int(max->out_t, warp->getT()); // t==0 in the beginning
    outlet_int(max->out_h, warp->getH());
    outlet_float(max->out_tempo, 1);

	post("At start position!");
}

void Raskell::input(long v) {
	// select input source: SCORE or LIVE 
	reset_feat();
	input_sel = v;
	if((input_sel == IN_LIVE) && !ysize) {
		object_error((t_object *)this, "initialize SCORE first!"); 
		input_sel = 0;
	}
	switch (input_sel) {
		case 0 : post("input selection is: OFF"); break;
		case IN_SCORE : post("input selection is: SCORE"); break;
		case IN_LIVE : post("input selection is: LIVE"); break;
		case OUT_IO : post("inputs OFF, ready to write history!"); break;
	}
	
	if (input_sel == IN_LIVE) {
		//markers[m_count][0] = ysize-fsize; // add END marker
        warp->start();
		iter = 0;
		b_iter = 0;
		acc_iter = 0;
	}
	if (ref_tempo)	beat->setTempo(ref_tempo);
	//post ("reference tempo set at %.2f BPM", ref_tempo);
}

void Raskell::gotomarker(long v) {
    unsigned int to_h = warp->getMarkerFrame(v);
	if (!to_h)
		object_error((t_object *)this, "marker %i doesn't exist!", v);
	else {
		v = to_h * HOP_SIZE;
		double ms = v / SampleRate * 1000.f;
		gotoms(ms);
	}
	
}

void Raskell::gotoms(long v) {
	post("Starting from %i ms", v);
	unsigned int to_h = v * (double)(SampleRate / 1000.f / HOP_SIZE);
    warp->setH(to_h);
    h_real = to_h;
}


// ======================== DSP stuff ================

void Raskell::dspinit() {
	fillBanks();
	hamming(WINDOW_SIZE); // build window vector

	reset_feat(); // sets frame_indexes, resizes frame vectors.
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

void Raskell::reset_feat() {
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
#ifdef USE_FFTW
	t_uint16 i;//, K=(WINDOW_SIZE/2)+1;
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
#endif
}


double Raskell::compress(double value, bool active) {
	static t_uint16 timer = 1;
	//if (t < 4)
        timer = COMP_RELEASE;
	if (value > 0) {	// upper thresh (limiter)
		value = -COMP_THRESH / COMP_RATIO;
	} else {			// lower thresh
		if (active) {
			value = (value - COMP_THRESH) / COMP_RATIO;
			timer = 1;
		}		
		else {
			if (timer < COMP_RELEASE) {
				float ratio = timer * COMP_RATIO;
				value = (value - COMP_THRESH) / ratio;
				timer++;
//				post("RELEASING %i value: %.2f", timer, value);
			}		
		}
	}
	return value;
}

// ========================= DTW ============================


double Raskell::calc_tempo(int mode) {
    unsigned int t = warp->getT();
	if (t == 0) return 1;
    
    unsigned int h = warp->getH();
    
	int second = SampleRate / HOP_SIZE; // number of frames per second
	static double last_tempo = 1;
	double new_tempo = tempo;
	static t_int16 last_beat = 1;
	static double old_tempo = 1; // for ARZT model
	// for DEQ model
	static double Min = VERY_BIG, Sum;
	bool got_new_tempo = false;
	int K = second, L = second / 2;
	t_uint16 counter = 0;
    


	switch(mode) {
		case T_DTW:
			if (t <= second)
				new_tempo = 1;
			else { 			
				if(beat_due) {
					// updated every beat
					post("update tempo, beat ela: %f", elast_beat);
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
					if(elast_beat == 1) post("update tempo 1");
					else post ("%.2f", elast_beat);
					new_tempo = (double)(warp->getHistory(t) - warp->getHistory(last_beat)) / (t - last_beat);
					double boost = error / ((double)(t - last_beat)*10);
					new_tempo += boost;
					last_beat = t;
				}
			}
			break;
		case T_PID:
			if (t <= second)
				new_tempo = 1;
			else { 		
				errors.push_back(error);
				if(errors.size() > (t - last_beat))
					errors.pop_front();
				if (abs(error) < 20.f) //anti-windup
					integral += error;
				if (beat_due) {
					// PID tempo model		
					new_tempo = (double)(warp->getHistory(t) - warp->getHistory(last_beat)) / (t - last_beat);
					float derivate = (error - errors[0]) / (t - last_beat);
					//post("err = %f // int = %f // der = %f", Kp*error, Ki*integral, Kd*derivate);
					double boost = (Kp*error + Ki*integral + Kd * derivate) / ((double)(t - last_beat));
					new_tempo += boost;
					last_beat = t;
					//post("new tempo %f", new_tempo);
				}
			}
			break;
		case T_DEQ:
			// get tempo pivots from history via deque: http://www.infoarena.ro/deque-si-aplicatii
			// first pivot is within L steps from path origin
			// 2nd pivot is at least K steps away from 1st, AND minimizes SUM(var_tempo[pivots])
			if (t < bsize || h < bsize)
				new_tempo = warp->getHistory(t) - warp->getHistory(t-1); // not yet active
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
							float derr_i = abs(b_err[(i-40+bsize)%bsize][3] - b_err[(i-41+bsize)%bsize][3]) +
								abs(b_err[(i-41+bsize)%bsize][3] - b_err[(i-42+bsize)%bsize][3]);
							float derr_q = abs(b_err[(Deque.front())%bsize][3] - 
												b_err[(Deque.front()-1+bsize)%bsize][3]) + 
											abs(b_err[(Deque.front()-1+bsize)%bsize][3] - 
												b_err[(Deque.front()-2+bsize)%bsize][3])	;
							while (!Deque.empty() && derr_i < derr_q) {
								Deque.pop_front();   
								popped = true;
							}					
						}
						if (popped || Deque.empty()) Deque.push_front((i-40+bsize)%bsize);
					}
					if (!Deque.empty()) {
						t_uint16 iplusK = (i-40-K+bsize)%bsize;
						float derr_ipK = abs(b_err[(iplusK)%bsize][3] - 
												b_err[(iplusK-1+bsize)%bsize][3]) +
											abs(b_err[(iplusK-1+bsize)%bsize][3] - 
												b_err[(iplusK-2+bsize)%bsize][3]);
						float derr_q = abs(b_err[(Deque.front())%bsize][3] - 
												b_err[(Deque.front()-1+bsize)%bsize][3]) +
										abs(b_err[(Deque.front()-1+bsize)%bsize][3] - 
												b_err[(Deque.front()-2+bsize)%bsize][3]);
						Sum = derr_q + derr_ipK;
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
				int i = (b_start-40+bsize)%bsize, j = 0;
				float derr_i = abs(b_err[(i-40+bsize)%bsize][3] - b_err[(i-41+bsize)%bsize][3]) +
								abs(b_err[(i-41+bsize)%bsize][3] - b_err[(i-42+bsize)%bsize][3]);
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
							new_tempo += *it * (i+1);	// sum of tempos[i]*(n-i)
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

void Raskell::beat_switch() {
    unsigned int t = warp->getT();
    unsigned int h = warp->getH();
    error = (h - h_real);
    
	double beat_tempo = calc_beat_tempo();
    int beat_length = (acc_iter) ? acc_beats[0][acc_iter] - acc_beats[0][acc_iter-1] : fsize;

	static int waiting = 0;
	// sensitivity to tempo fluctuations:
	if (abs(error) > pow(1.f-sensitivity, 2)*100.f + abs(minerr)) {
		post ("waiting %i . tempo %f - %f beat_tempo", waiting, tempo_avg, beat_tempo);
		if (abs(b_err[b_start][0]) > 15 && abs(tempo_avg - beat_tempo) > 0.15) {
			// DTW is way off, beat tracker takes over
			post("big error! %f != %f", tempo_avg, beat_tempo);
			waiting = (int)(fsize / beat_length) * beat_length;
		}
		if (waiting) {
			tempo = beat_tempo;		
			tempo_mode = 2;
		} else {
			if (tempo_mode == 2) {
				h_real = h;
				h_real += tempo;
				post("moved H_real to H");
			}
			tempo = calc_tempo(tempo_model);	
			tempo_mode = 1;
		}
	}		
	else if (abs(error) <= 1 && tempo_mode && t > 40) {
		tempo = b_err[(b_start-40+bsize)%bsize][3];
		tempo_mode = 0;
	}

	if(waiting > 0) waiting--;

	if (tempo > 0.01) {
		if (tempo < 3 && tempo > 0.33) {
			outlet_float(max->out_tempo, tempo);
			h_real += tempo;
		} else {
			post("OUT OF CONTROL tempo = %f", tempo);
			tempo = beat_tempo;			
			h_real += tempo;
			waiting = (int)(fsize / beat_length) * beat_length;
			//h_real = h;
			//h = h_real;
			//h_mod = h%fsize;
		}
	}
	// output real H
	if (h_real > 8 && !(t % 9)) {
		atom_setsym(dump, gensym("h_real"));
		atom_setfloat(dump+1, h_real);
		// output real H (scaled)
		atom_setfloat(dump+2, h_real / ysize);
		// output active tempo calculation toggle
		atom_setlong(dump+3, tempo_mode);
		outlet_list(max->out_dump, 0L, 4, dump);
	}
}

double Raskell::calc_beat_tempo() {
	/*if (beat->beatDueInCurrentFrame()) {
		post ("h= %d becomes %f", h, h_real);
		h = h_real;
		h_mod = h%fsize;
	}*/
	return (beat->getCurrentTempoEstimate() / ref_tempo);
}

//void Raskell::increment_h() {

	/*
	elast_beat = 1;
	float dist = 3/(abs(y_beats[1][b_iter])+1);
	if (h > y_beats[0][b_iter] - dist && h < y_beats[0][b_iter]) {
		elast_beat = ( y_beats[0][b_iter] - h ) / dist;
	}
	dist = 10/(abs(y_beats[1][b_iter])+1);
	if (h < y_beats[0][b_iter] + dist && h >= y_beats[0][b_iter]) {
		elast_beat = ( h - y_beats[0][b_iter] ) / dist;
	}

	if (b_iter+1 < y_beats[1].size()) {
		dist = 3/(abs(y_beats[1][b_iter+1])+1);
		if (h > y_beats[0][b_iter+1] - dist && h < y_beats[0][b_iter+1]) {
			elast_beat = ( y_beats[0][b_iter+1] - h ) / dist;
		}
		dist = 10/(abs(y_beats[1][b_iter+1])+1);
		if (h < y_beats[0][b_iter+1] + dist && h >= y_beats[0][b_iter+1]) {
			elast_beat = ( h - y_beats[0][b_iter+1] ) / dist;
		}
	}*/


//}



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
			post("No %s file exists, accessing buffer~", ps);
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
				acc_beats.clear();
				acc_beats.resize(2);
				score_name = name;
				// start reading SCORE frames				
				// count # of framess:
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
	double f_feat[50];
	t_uint16 i;
	char buf[1024];
	for(i=0; i<params; i++)
		f_feat[i] = 0;
	while ( sgets( buf, sizeof( buf )) ) { //&& i) {
		int length = sscanf(buf, "%lf, %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
			&f_feat[0], &f_feat[1], &f_feat[2], &f_feat[3], &f_feat[4], 
			&f_feat[5], &f_feat[6], &f_feat[7], &f_feat[8], &f_feat[9],  
			&f_feat[10], &f_feat[11], &f_feat[12], &f_feat[13], &f_feat[14], 
			&f_feat[15], &f_feat[16], &f_feat[17], &f_feat[18], &f_feat[19], 
			&f_feat[20], &f_feat[21], &f_feat[22], &f_feat[23], &f_feat[24], 
			&f_feat[25], &f_feat[26], &f_feat[27], &f_feat[28], &f_feat[29],
			&f_feat[30], &f_feat[31], &f_feat[32], &f_feat[33], &f_feat[34], 
			&f_feat[35], &f_feat[36], &f_feat[37], &f_feat[38], &f_feat[39], &f_feat[40]);
		if(length > (params+1) && input_sel != IN_LIVE) {
			post("NOTE: number of params increased to %i", params);
			params = length-1; // set number of parameters (increase if necessary)
			warp->setParams(params);
		}

		for(t_uint16 i=0; i<params; i++) 
			// f_feat[0] = order no.
			tfeat[i] = f_feat[i+1];
		
		switch (features) {
		case (MFCCS) :
			// if signal is loud enough, then compress the first feat coefficient (loudness)
			if (tfeat[0] > COMP_THRESH) 
				tfeat[0] = compress(tfeat[0], true);//tfeat[0] /= 8;
			else tfeat[0] = compress(tfeat[0], false);
			break;
		}

		if(strstr(buf, "marker"))//-buf==0) // marker
			marker(NULL);
		if(strstr(buf, " beat")) {
			y_beats[0].push_back(iter);  // beat pos
			y_beats[1].push_back(0);	// diff from acco beat (computed below in feats())
			//post("new Y beat at %d", iter);
		}
		char* beat_temp = strstr(buf, "accobeat");
		if(beat_temp) {
			acc_beats[0].push_back(iter);  // beat pos
			double temp;
			sscanf(beat_temp, "accobeat %lf", &temp);
			acc_beats[1].push_back(temp);  // beat pos
			//post("new ACC beat at %d tempo %lf", iter, temp);
		}
		feats(params); 

	}
	return false;
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
	if (input_sel == OUT_IO) { // in IO mode, report history
		filetype = 'CSV';
		strcpy(filename, "io");
		for (i = 0; i < warp->getH(); i++) {
			buf += to_string((long long)(warp->getHistory(i))) + "\n";
		}
	} else if (input_sel == IN_SCORE) { // in SCORE mode, save the score
		strcpy(filename, "score.txt");
		b_iter = acc_iter = 0;
		for (i = 0; i < ysize; i++) {
			buf += to_string(i) + ",";
			for (j = 0; j < params; j++) {
				buf += " " + to_string((long double)(warp->getY(i,j)));
			}
			if (y_beats[0].size() && i == y_beats[0][b_iter]) {
				buf += " beat";
				b_iter++;
			}
			if (acc_beats[0].size() && i == acc_beats[0][acc_iter]) {
				buf += " accobeat " + to_string((long long)acc_beats[1][acc_iter]);
				acc_iter++;
			}
			buf += ";\n";
		}
	}
	else { // in LIVE mode, save the test results
		filetype = 'CSV';
		strcpy(filename, "trace");
		buf+="Parameters:, DTW size, B-DTW size, FFT size, FFT hop, ALPHA, MAX_RUN, COMP_THRESH\n,";
		buf+= to_string((long long)fsize) + ", " +
			to_string((long long)bsize) + ", " +
			to_string((long long)WINDOW_SIZE) + ", " + 
			to_string((long long)HOP_SIZE) + ", " + 
			to_string((long long)ALPHA) + ", " + 
			to_string((long long)MAX_RUN) + ", " + 
			to_string((long long)COMP_THRESH);		
		buf+="\nMarker Trace Results for," + score_name + ", vs, " + live_name;
		buf+="\nID, Score pos, Certainty, Live accomp, Live ideal, Live detected";
		for (i = 0; i <= warp->getMarkerCount(); i++) {
			buf += "\n" + to_string(i);
			for (j = 0; j < 5; j++)
                buf += ", " + to_string((long double)(warp->getMarker(i,j)));
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

void Raskell::set_buffer(t_symbol *s, int dest) {

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
		if (dest == B_SOLO && input_sel == IN_SCORE) {// make new score
			score_size((long)(frames / HOP_SIZE));
			warp->addMarkerToScore(1); // add marker at start (position 1)
			iter = 0;
		}

		float* sample = buffer_locksamples(b);

		if (sample) {
			double samp[HOP_SIZE];
			long framesread = 0;
			beat->updateHopAndFrameSize(HOP_SIZE, HOP_SIZE*2);
			for (long i = 0; i < frames-HOP_SIZE; i += HOP_SIZE) {			
				std::copy(sample+i, sample+i+HOP_SIZE, samp); // convert float to double
				//post("sample[%d] %f , double %f", i, sample[i], samp[0]);
				perform(samp, HOP_SIZE, dest);
				framesread ++;
			}
			post("Done reading buffer. %d frames. %d ACC beats. %d Y beats.", framesread, acc_beats[0].size(), y_beats[0].size());
		}
		buffer_unlocksamples(b);
	}
}