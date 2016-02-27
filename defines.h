// rvdtw~.h
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

// marker matrix:
#define M_SCORED 0
#define M_HOOK 1
#define M_CERT 2
#define M_IDEAL 3
#define M_LIVE 4

// tempo models:
#define T_DTW 0
#define T_P 1
#define T_PID 2
#define T_PIVOTS 3