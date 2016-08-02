#include "oDTW.h"


oDTW::oDTW(int windowSize_, int backWindowSize_, bool backActive_, unsigned int params_) :
		fsize(windowSize_),
		bsize(backWindowSize_),
		back_active(backActive_),
        params(params_)
{
    ysize = t = h = runCount = iter = m_iter = m_ideal_iter = t_mod = h_mod = 0; // current position for online DTW: (t,h)
    b_start = bh_start = 0; // backwards DTW vars
    score_loaded = false;
    
    m_count = 0; // one marker is mandatory (to start)
    previous = 0; // 0 = none; 1 = Row; 2 = Column; 3 = Both

    mid_weight = MID;
    top_weight = SIDE;
    bot_weight = SIDE;
    
    maxRunCount = MAX_RUN; // tempo between 1/x and x
    
}


oDTW::~oDTW() {

}

// ====== public methods ==========

unsigned int oDTW::setScoreSize(long v) {
    long i;
    if ((v < MAXLENGTH) && (v > bsize)) {
        iter = 0;
        score_loaded = false;
        ysize = v;
        // we have ysize -> MEMORY ALLOCATION
        y.clear();
        y.resize(ysize);
        for (i=0; i<ysize; i++) {
            y[i].resize(params);
        }

        start(); // x, dtw arrays
        history.resize(ysize * 3); // leave room for t be 3 times longer than h
        b_path.resize(bsize);
        
        markers.clear();
        markers.resize(ysize);
        for (i=0; i<ysize; i++)
            markers[i].resize(5); // 0: scored, 1: ??, 2: ??, 3: live ideal, 4: live detected
        return ysize;
    }
    else return 0;
}

unsigned int oDTW::processScoreFV(double *tfeat) {
    if (iter < ysize) {
        unsigned int j;
        for (j=0; j < params; j++) {
            y[iter][j] = tfeat[j];
        }
        iter++; //iterate thru Y
    }
    if (iter == ysize) {
        score_loaded = true;
    }
	return iter;
}

void oDTW::processLiveFV(double *tfeat) {
    if (t == 0)//xsize-1)  // is the window full?
        init_dtw();
    else {
        unsigned int i;
        for (i=0; i < params; i++) {
            x[t%bsize][i] = tfeat[i];
        }
        if (dtw_process()) // compute oDTW path until we reach h == ysize
			if(back_active)
				dtw_back(); // calculate backwards DTW
    }
}

void oDTW::addMarkerToScore(unsigned int frame) {
	if (frame) {
		markers[m_iter][M_SCORED] = frame;
	} else {
		markers[m_iter][M_SCORED] = iter-1;
	}
	m_count++;
	m_iter++;
}

unsigned int oDTW::addMarkerToLive(unsigned int frame) {
	if (m_ideal_iter < m_count) {
		if (frame)
			markers[m_ideal_iter][M_IDEAL] = frame;
		else
			markers[m_ideal_iter][M_IDEAL] = t;
		m_ideal_iter++;
	}
	return m_ideal_iter;
}

unsigned int oDTW::getMarkerFrame(long here) {
    return markers[here][M_SCORED];
}

unsigned int oDTW::getMarkerCount() {
    return m_count;
}

double oDTW::getMarker(unsigned int i, unsigned int j) {
    if (i <= m_count && j < 5)
        return markers[i][j];
    else
        return 0;
}


void oDTW::start() {
    t = t_mod = h = h_mod = runCount = m_iter = m_ideal_iter = 0; // current position for online DTW: (t,h)
    b_start = bh_start = 0;
    previous = 0; // 0 = none; 1 = Row; 2 = Column; 3 = Both
    top_weight = bot_weight = SIDE;
    
    short i;
    
    x.clear();
    x.resize(bsize);
    for (i=0; i<bsize; i++) {
        x[i].resize(params);
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
    
    history.clear();
    
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
    
    b_path.clear();
    
    b_err.clear();
    b_err.resize(bsize);
    for (i=0; i<bsize; i++) {
        b_err[i].resize(4); // error, t, h, local tempo
    }
}

unsigned int oDTW::getT() {
    return t;
}

unsigned int oDTW::getH() {
    return h;
}

void oDTW::setH(unsigned int to_h) {
    h = to_h;
    h_mod = h % fsize;
}

unsigned int oDTW::getHistory(unsigned int from_t) {
    return history[from_t];
}

double oDTW::getY(unsigned int i, unsigned int j) {
    return y[i][j];
}



unsigned int oDTW::getFsize() {
    return fsize;
}

vector<vector<double> > oDTW::getBackPath() {
    return b_err;
}

bool oDTW::isRunning() {
    if ((h < ysize) && (h > 0)) return true;
    else return false;
}

bool oDTW::isScoreLoaded() {
    return score_loaded;
}

void oDTW::setParams(int params_) {
    if (params_ != params) {
        params = params_;
        start();
    }
}

// ====== internal methods ==========

void oDTW::init_dtw() {
    distance(t, h);
    
    for (unsigned int i=0; i<fsize; i++) {
        for (unsigned int j=0; j<fsize; j++) {
            dtw[i][j] = VERY_BIG;
        }
    }
    dtw[t_mod][h_mod] = Dist[t%bsize][h%bsize]; // initial starting point
    
    increment_t();
    increment_h();
    history[1] = 1;
}

void oDTW::distance(unsigned int i, unsigned int j) {
    unsigned int k, imod = i%bsize, jmod=j%bsize;
    // build distance matrix
    double total;
    total = 0;
    
    if ((x[imod][0]==0)||(y[j][0]==0)) { // if either is undefined
        total = VERY_BIG;
        //post("WARNING input is zero, t=%i h=%i", i, j);
    } else for (k = 0; k < params; k++) {
        total = total + ((x[imod][k] - y[j][k]) * (x[imod][k] - y[j][k])); // distance computation
        //total = total + abs(x[imod][k] - y[j][k]); // L1 distance computation
    }
    if (total < 0.0001)
        total = 0;
    total = sqrt(total);
    total += ALPHA;
    
    Dist[imod][jmod] = total;
    //post("Dist[%i][%i] = %f", imod, jmod, total);
}

unsigned int oDTW::get_inc() {    
    // helper function for online DTW, choose Row (h) / Column (t) incrementation
    unsigned int i, next = 0;
    unsigned int tmin1 = (t_mod+fsize-1) % fsize;
    unsigned int hmin1 = (h_mod+fsize-1) % fsize;
//    unsigned int tmin2 = (t_mod+fsize-2) % fsize;
//    unsigned int hmin2 = (h_mod+fsize-2) % fsize;
    double min = VERY_BIG;

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

void oDTW::calc_dtw(unsigned int i, unsigned int j) {
    // calculate DTW matrix
    double top, mid, bot, cheapest;
    unsigned int imin1 = (i+fsize-1) % fsize;
    unsigned int imin2 = (i+fsize-2) % fsize;
    unsigned int jmin1 = (j+fsize-1) % fsize;
    unsigned int jmin2 = (j+fsize-2) % fsize;
    unsigned int imod = i % fsize;
    unsigned int jmod = j % fsize;
    
    top = dtw[imin2][jmin1] + Dist[i%bsize][j%bsize] * top_weight;
    mid = dtw[imin1][jmin1] + Dist[i%bsize][j%bsize] * mid_weight;
    bot = dtw[imin1][jmin2] + Dist[i%bsize][j%bsize] * bot_weight;
    
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

bool oDTW::dtw_process() {
    unsigned int inc = get_inc();
    unsigned int j, jstart;
    
    // it's possible to have several Y/h hikes for each X/t feature:
    while((inc == NEW_ROW) && (h < ysize)) {
        
        if (t<bsize)	jstart = 0;
        else			jstart = t-bsize;
        for (j = jstart; j < t; j++) {
            distance(j, h); // calculate distance for new row
            calc_dtw(j, h); // calc DTW for new row
        }
        increment_h();
        previous = NEW_ROW;
        runCount++;
        inc = get_inc(); // get new inc
    }
				
    if (h < ysize) { // unless we've reached the end of Y...
        
        if (inc != NEW_ROW) { // make new Column
            
            if (h<bsize)	jstart = 0;
            else			jstart = h-bsize;
            
            for(j=jstart; j<h; j++) {
                distance(t, j); // calculate distance for new column
                calc_dtw(t, j); // calc DTW for new column
            }
            increment_t();
        }
        
        if (inc != NEW_COL) { // make new Row
            
            if (t<bsize)	jstart = 0;
            else			jstart = t-bsize;
            
            for (j=jstart; j<t; j++) {
                distance(j, h); // calculate distance for new row
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
        
        history[t] = h;
        return true;
    }
	
	increment_t();
	history[t] = h;
    return false; 
}

void oDTW::increment_t() {
    t++;
    t_mod = (t_mod+1)%fsize;
}

void oDTW::increment_h() {
    h++;
    h_mod = (h_mod+1)%fsize;
    
    if (h >= markers[m_iter][M_SCORED]) {
        //if (h_real >= markers[m_iter][M_SCORED]) {
        markers[m_iter][M_LIVE] = t; // marker detected at time "t"
		m_iter++;
        //markers[m_iter][M_HOOK] = dtw_certainty;
        /* TO DO
        markers[m_iter][M_ACC] = h_real;
        post("marker detected at t = %i, h = %i, h_real = %f", t, h, h_real);
        atom_setsym(dump, gensym("marker"));
        atom_setlong(dump+1, m_iter);
        outlet_list(max->out_dump, 0L, 2, dump);
        if (m_iter < m_count)
            m_iter++;
         */
    }
}

bool oDTW::decrease_h() {
    /* TO DO
     vector<double> diff;
     long i, j, ends;
     long begin = h - MAX_RUN + 1;
     static long min_begin = begin;
     if (begin < min_begin) {
     post("begin = %i; min = %i", begin, min_begin);
     return false;
     }
     else
     min_begin = begin;
     
     post("moving h back %i steps", MAX_RUN);
     ends = MAX_RUN / 4;
     
     // make phantom y continuous with existing y
     diff.clear();
     diff.resize(params);
     for(i = 0; i < params; i++) {
     for(j = 0; j < ends; j++)
     diff[i] += (y[h-j][i] - y[begin+j][i]);
     diff[i] /= ends;
     }
     for(long k = begin; k <= h; k++) {
     for(i = 0; i < params; i++)
     y[k][i] += diff[i];
     }
     
     while(h > begin) {
     if (m_iter && h == markers[m_iter-1][0])
     m_iter--; // cancel any markers
     h--;
     }
     h_mod = h%fsize;
     
     for(i = t-bsize; i < t; i++)
     for(j = h-bsize; j < h; j++) {
     distance(i, j); // calculate distance 
     calc_dtw(i, j); // calc DTW cost
     }
     return true;
     */
    
}

void oDTW::dtw_back() {
    b_start = t % bsize;
    bh_start = h % bsize;
    
    b_err[b_start][0] = 0.f; // to be computed after t > bsize, below
    b_err[b_start][1] = t;
    b_err[b_start][2] = h;
    b_err[b_start][3] = 1.f; // local tempo:
    
    if (t >= bsize && h >= bsize) { //&& (t % 2)
        double top, mid, bot, cheapest;
        unsigned int i, j;
        b_path.clear();
        // post("b_start is %i", b_start);
        b_dtw[b_start][bh_start] = Dist[b_start][bh_start]; // starting point
        b_move[b_start][bh_start] = NEW_BOTH;
        
        // compute backwards DTW (circular i--), and b_move
        for (i = b_start+bsize-1 % bsize; i != b_start; i = (i+bsize-1) % bsize) { // t-1 ... t-bsize
            for (j = bh_start+bsize-1 % bsize; j != bh_start; j = (j+bsize-1) % bsize) { // h-1 ... h-bsize
                unsigned int imod = i % bsize;
                unsigned int imin1 = (i+1) % bsize;
                unsigned int imin2 = (i+2) % bsize;
                unsigned int jmod = j % bsize;
                unsigned int jmin1 = (j+1) % bsize;
                unsigned int jmin2 = (j+2) % bsize;
                
                top = b_dtw[imin2][jmin1] + Dist[imod][jmod] * 0.2;
                mid = b_dtw[imin1][jmin1] + Dist[imod][jmod] * mid_weight;
                bot = b_dtw[imin1][jmin2] + Dist[imod][jmod] * 0.2;
                
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
        unsigned int b_t = t, b_h = h;
        i = (b_start+bsize-1) % bsize;		// t-1
        j = (bh_start+bsize-1) % bsize;		// h-1
        b_path.push_back(0);
        int p = 1;
        while (i != b_start && j != bh_start) {
            if (b_move[i][j] == NEW_ROW) {
                j = (j+bsize-1) % bsize;	 // j--
                b_h--;
                p++;
            }
            else if (b_move[i][j] == NEW_COL) {
                i = (i+bsize-1) % bsize;	// i--
                b_t--;
                b_path.push_back(p);
            }
            else if (b_move[i][j] == NEW_BOTH) {
                i = (i+bsize-1) % bsize;	// i--
                j = (j+bsize-1) % bsize;	// j--
                b_t--; b_h--;
                p++;
                b_path.push_back(p);
            }
        }
        
        b_err[b_start][0] = history[b_t] - b_h;
        //float diff = b_err[b_start][0] - b_err[(b_start-1+bsize)%bsize][0];
        //if (diff < 0)
        //	b_err[b_start][0] -= diff / 2;
        
//     if (tempo_mode != 2) { // if beat-tracker doesn't have control
        if (b_err[b_start][0] > 5) { // gotta go DOWN
            if (bot_weight < 20) bot_weight += fabs(b_err[b_start][0]) / 50;
            //post("bot w = %f", bot_weight);
        }
        else if (b_err[b_start][0] < -5) {	// gotta go UP
            if (top_weight < 20) top_weight += fabs(b_err[b_start][0]) / 50;
            //post("top w = %f", top_weight);
        }
        else {
            top_weight = bot_weight = SIDE;
        }
//     }
        int step = bsize / 4;
        
        if (t > step * 2) {
            // compute local tempo based on back DTW history:
            b_err[(b_start-step+bsize)%bsize][3] =
            (float)(h - b_err[(b_start-(step*2)+bsize)%bsize][2] + 1) / (t - b_err[(b_start-(step*2)+bsize)%bsize][1] + 1);
        }
    }
}