# oDTW
An Online Dynamic Time Warping C++ library. 

Adapted from the original algorithm by Simon Dixon presented in:
* "An On-Line Time Warping Algorithm for Tracking Musical Performances", S. Dixon. In IJCAI, pp. 1727-1728. 2005.

A full description of the library is in my (upcoming) PhD thesis:
* "Dynamic Music Representations for Real-Time Performance", G. Burloiu, PhD Thesis, "Politehnica" University of Bucharest, 2016.

## Usage

1 / Include the library header.
```
#include "oDTW.h"
```
2 / Instantiate the oDTW object
```
oDTW *dtw = new oDTW();
// OR: oDTW *dtw = new oDTW(128, 512, false, 12);
```
The four parameters are: 
* forward (online) DTW matrix size (default 128)
* backward (offline) DTW matrix size (default 512)
* use backward DTW for path confirmation / weighting adjustment (default true)
* no. of parameters in a feature vector (default 12, appropriate for a Chromagram vector)

3 / Set the score size. This is the number of score frames the object will allocate memory for.
```
dtw->setScoreSize(ysize);
```

4 / In a loop, fill a feature vector with values and then add them to the score.
```
double f_feat[12];
// ... fill vector
dtw->processScoreFV(f_feat);
```
When you're done, you can check whether the score is fully loaded:
```
if (dtw->isScoreLoaded()) { 
	dtw->start(); // start "listening" for live features
	//...
}
```

5 / In the "live" mode, you similarly fill a feature vector and process it frame by frame:
```
dtw->processLiveFV(f_feat);
```
At any point, you can check whether the end has been reached, and do something with the current T and/or H coordinates:
```
if (dtw->isRunning()) {
	int t = dtw->getT();
	int h = dtw->getH();
	// do something
}
```
When you're done, you can start() a new run.

## Testing

The [unit test](https://github.com/RVirmoors/RVdtw-/blob/master/oDTW_test/) folder shows a simple use case / test scenario. Compile and run it with the Boost Test library.