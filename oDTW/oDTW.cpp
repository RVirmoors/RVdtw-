#include "oDTW.h"


oDTW::oDTW(int windowSize_, int backWindowSize_, bool backActive_, unsigned int params_) :
		fsize(windowSize_),
		bsize(backWindowSize_),
		back_active(backActive_),
        params(params_)
{
    ysize = t = h = runCount = m_iter = m_ideal_iter = t_mod = h_mod = 0; // current position for online DTW: (t,h)
    b_avgerr = b_start = bh_start = 0; // backwards DTW vars
    
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
    if ((v < MAXLENGTH) && (v > 0)) {
        ysize = v;
        // we have ysize -> MEMORY ALLOCATION
        y.clear();
        y.resize(ysize);
        for (i=0; i<ysize; i++) {
            y[i].resize(params);
        }

        start(); // x, dtw arrays
        history.resize(ysize);
        b_path.resize(bsize);
        
        markers.clear();
        markers.resize(ysize);
        for (i=0; i<ysize; i++)
            markers[i].resize(5); // 0: scored, 1: ??, 2: ??, 3: live ideal, 4: live detected
        return ysize;
    }
    else return 0;
}

void oDTW::processScoreFV(double *tfeat) {
    
}

bool oDTW::processLiveFV(double *tfeat) {
    
}

void oDTW::addMarkerToScore(unsigned int frame) {
    markers[m_iter][M_SCORED] = frame;
    m_count++;
    m_iter++;
}

void oDTW::addMarkerToLive(unsigned int frame) {
    markers[m_ideal_iter][M_IDEAL] = frame;
    m_ideal_iter++;
}

unsigned int oDTW::getMarkerFrame(long here) {
    return markers[here][M_SCORED];
}


void oDTW::start() {
    t = t_mod = h = h_mod = runCount = m_iter = m_ideal_iter = 0; // current position for online DTW: (t,h)
    b_avgerr = b_start = bh_start = 0;
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
    
}

// ====== internal methods ==========

void oDTW::init_dtw() {
    distance(t, h); // calculate Dist[t_mod][h_mod]
    
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
    while((inc == NEW_ROW) && (h < ysize) && (h != markers[0][0] + fsize-1)) {
        
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
    else { // h = ysize
        return false;
    }
}

void oDTW::increment_t() {
    
}

void oDTW::increment_h() {
    
}

bool oDTW::decrease_h() {
    
}

void oDTW::dtw_back() {
    
}

