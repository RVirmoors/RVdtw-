// defines.h
//
// 2014-2016 Grigore Burloiu
//
// int defines



// input select:
#define IN_SCORE 1
#define IN_LIVE 2
#define OUT_IO 3



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
#define MFCCS 1
#define CHROMA 2

// buffer destination
#define BUF_MAIN 1
#define BUF_ACCO 2
