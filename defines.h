// defines.h
//
// 2014-2019 Grigore Burloiu
//
// int defines



// ========= RVDTW main ===============

// input select:
#define IN_SCORE 1
#define IN_LIVE 2
#define OUT_IO 3

// audio features
#define MFCCS 1
#define CHROMA 2

// buffer destination
#define BUF_MAIN 1
#define BUF_ACCO 2

// ========= TEMPO models ===============

// tempo tracking params
#define SEN 1 // 0.9 // 0.98//1
#define ELA 1 // 1

// tempo modes:
#define OFF 0
#define DTW 1
#define BEAT 2

// acco beats:
#define A_BEATPOS 0
#define A_TEMPO 1

// reference beats:
#define Y_BEATPOS 0
#define Y_DIFF 1

// tempo models:
#define T_DTW 0
#define T_P 1
#define T_PID 2
#define Kp 0.017 //0.017 //0.5 // 0.1467
#define Ki 0.0003//0.0004 // 0.0007 // 0.01 // 0.005051
#define Kd 0.4 //2.5
#define T_DEQ 3
#define T_ARZT 4
