#define BOOST_TEST_MODULE online DTW test module
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//#include <iostream>
#include <fstream>
#include <math.h>
#include "..\oDTW\oDTW.h"

BOOST_AUTO_TEST_SUITE(oDTW_test_suite)

// ===============================================
 
BOOST_AUTO_TEST_CASE(identical)
{
	oDTW dtw(2, 4, true, 1);
	double in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	dtw.start();
	dtw.setScoreSize(10);
	for (int i = 0; i < 10; i++)
		dtw.processScoreFV(&in[i]);
	// all 10 frames should now be loaded	
	BOOST_CHECK(dtw.getY(3,0) == 4);
	BOOST_CHECK(dtw.isScoreLoaded());

	for (int i = 0; i < 10; i++)
		dtw.processLiveFV(&in[i]);
	BOOST_CHECK(dtw.getH() == 10);
	BOOST_CHECK(!dtw.isRunning());

	BOOST_CHECK(dtw.getHistory(6) == 6);
}

// ===============================================

BOOST_AUTO_TEST_CASE(file_inputs)
{
	
	oDTW *dtw = new oDTW(32, 128, false, 12);
	ifstream fileX("fileX.txt"); 
	ifstream fileY("fileY.txt"); 
	double f_feat[50];
	unsigned int iter = 0;

	BOOST_CHECK(fileX.is_open());
	
	// count # of frames:
	long xsize = count(std::istreambuf_iterator<char>(fileX), 
					   std::istreambuf_iterator<char>(), '\n');
	long ysize = count(std::istreambuf_iterator<char>(fileY), 
					   std::istreambuf_iterator<char>(), '\n');

	BOOST_CHECK (xsize);
	BOOST_CHECK (ysize);
	fileX.seekg(0);
	fileY.seekg(0);

	dtw->setScoreSize(ysize);
	
	string line;
	short params = 12;
	while (getline(fileY, line)) {
		int length = sscanf(line.c_str(), "%*lf, %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
			&f_feat[0], &f_feat[1], &f_feat[2], &f_feat[3], &f_feat[4], 
			&f_feat[5], &f_feat[6], &f_feat[7], &f_feat[8], &f_feat[9],  
			&f_feat[10], &f_feat[11], &f_feat[12], &f_feat[13], &f_feat[14], 
			&f_feat[15], &f_feat[16], &f_feat[17], &f_feat[18], &f_feat[19], 
			&f_feat[20], &f_feat[21], &f_feat[22], &f_feat[23], &f_feat[24], 
			&f_feat[25], &f_feat[26], &f_feat[27], &f_feat[28], &f_feat[29],
			&f_feat[30], &f_feat[31], &f_feat[32], &f_feat[33], &f_feat[34], 
			&f_feat[35], &f_feat[36], &f_feat[37], &f_feat[38], &f_feat[39]);
		BOOST_CHECK (length == params);
		if(length > params) {
			params = length;
			dtw->setParams(params);
		}
		iter = dtw->processScoreFV(f_feat);
	}
	
	BOOST_CHECK(iter == ysize);
	BOOST_CHECK(dtw->isScoreLoaded());

	dtw->start();
	dtw->setH(160);
	
	while (getline(fileX, line)) {
		int length = sscanf(line.c_str(), "%*lf, %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
			&f_feat[0], &f_feat[1], &f_feat[2], &f_feat[3], &f_feat[4], 
			&f_feat[5], &f_feat[6], &f_feat[7], &f_feat[8], &f_feat[9],  
			&f_feat[10], &f_feat[11], &f_feat[12], &f_feat[13], &f_feat[14], 
			&f_feat[15], &f_feat[16], &f_feat[17], &f_feat[18], &f_feat[19], 
			&f_feat[20], &f_feat[21], &f_feat[22], &f_feat[23], &f_feat[24], 
			&f_feat[25], &f_feat[26], &f_feat[27], &f_feat[28], &f_feat[29],
			&f_feat[30], &f_feat[31], &f_feat[32], &f_feat[33], &f_feat[34], 
			&f_feat[35], &f_feat[36], &f_feat[37], &f_feat[38], &f_feat[39]);
		BOOST_CHECK (length == params);
		dtw->processLiveFV(f_feat);
		BOOST_TEST_MESSAGE ("now at t = " << dtw->getT() << " h = "<< dtw->getH());
	}

	BOOST_CHECK(dtw->getT() == xsize);
	BOOST_CHECK_MESSAGE(!dtw->isRunning(), "still running! h is " << dtw->getH());

	int thresh = 20;
	int Xmarker[8] = {9, 249, 580, 786, 1092, 1358, 1713, 1979};
	int Ymarker[8] = {170, 384, 605, 825, 1048, 1264, 1545, 1872};


	for (int i = 0; i < 8; i++) {
		BOOST_CHECK_MESSAGE(abs((int)dtw->getHistory(Xmarker[i]) - Ymarker[i]) <= thresh,
			"marker "<< i <<" failed! real = "<<dtw->getHistory(Xmarker[i]) << "; ideal = " << Ymarker[i]);
	}

}

// ===============================================

BOOST_AUTO_TEST_SUITE_END()