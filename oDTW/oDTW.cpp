#include "oDTW.h"


oDTW::oDTW(int windowSize_, int backWindowSize_, bool backActive_) : 
		fsize(windowSize_),
		bsize(backWindowSize_),
		back_active(backActive_)
{

}


oDTW::~oDTW() {

}
