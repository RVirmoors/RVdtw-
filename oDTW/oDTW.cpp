#include "oDTW.h"


oDTW::oDTW(int windowSize_, int backWindowSize_, bool backActive_) : 
		fsize(windowSize_),
		bsize(backWindowSize_),
		back_active(backActive_)
{

}


oDTW::~oDTW() {

}

// ====== public methods ==========

void oDTW::setScoreSize(long v) {
    
}

void oDTW::processScoreFV(double *tfeat) {
    
}

void oDTW::processLiveFV(double *tfeat) {
    
}

void oDTW::addMarkerToScore() {
    
}

void oDTW::addMarkerToLive() {
    
}

void oDTW::start() {
    
}

t_uint16 oDTW::getT() {
    
}

t_uint16 oDTW::getH() {
    
}

t_uint16 oDTW::setH() {
    
}

t_uint16 oDTW::getHistory(t_uint16 t_) {
    
}

// ====== internal methods ==========

void oDTW::init_dtw() {
    
}

void oDTW::distance(t_uint16 i, t_uint16 j) {
    
}

t_uint16 oDTW::get_inc() {
    
}

void oDTW::calc_dtw(t_uint16 i, t_uint16 j) {
    
}

void oDTW::dtw_process() {
    
}

void oDTW::increment_t() {
    
}

void oDTW::increment_h() {
    
}

bool oDTW::decrease_h() {
    
}

void oDTW::dtw_back() {
    
}

