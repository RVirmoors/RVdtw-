// defines.h
//
// 2014-2016 Grigore Burloiu
//
// int defines


// previous dtw step:
#define NEW_ROW 1
#define NEW_COL 2
#define NEW_BOTH 3

// input select:
#define IN_SCORE 1
#define IN_LIVE 2
#define OUT_IO 3

// marker matrix:
#define M_SCORED 0
#define M_HOOK 1
#define M_ACC 2
#define M_IDEAL 3
#define M_LIVE 4

// tempo models:
#define T_DTW 0
#define T_P 1
#define T_PID 2
#define Kp 0.017 //0.017 //0.5 // 0.1467
#define Ki 0.0003//0.0004 // 0.0007 // 0.01 // 0.005051
#define Kd 0.4 //2.5
#define T_DEQ 3
#define T_ARZT 4

// audio features
#define MFCCS 0
#define GIST_MFCCS 1
#define CHROMA 2
