#define BOOST_TEST_MODULE online DTW test module
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <math.h>
#include <cstdint>
#include "..\oDTW\oDTW.h"

BOOST_AUTO_TEST_SUITE(tests)

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
	// BOOST_DATA_TEST_CASE causes compile errors in vs2010, so let's do it like this:
	for (int testno = 0; testno < 1; testno++) { // !! number of file pairs to be tested !!
		BOOST_TEST_MESSAGE (" ===== RUNNING FILE TEST # " << testno << " =====");

		string Xname = "fileX" + to_string((long long)testno) + ".txt";
		string Yname = "fileY" + to_string((long long)testno) + ".txt";

		// !! DTW params!! caution: "true" takes a lot more processing power due to back-DTW
		oDTW *dtw = new oDTW(128, 512, false, 12);//(256, 256, false, 12);
		ifstream fileX(Xname); 
		ifstream fileY(Yname); 
		double f_feat[50];
		unsigned int iter = 0;

		BOOST_REQUIRE(fileX.is_open());
	
		// count # of frames:
		long xsize = count(std::istreambuf_iterator<char>(fileX), 
						   std::istreambuf_iterator<char>(), '\n');
		long ysize = count(std::istreambuf_iterator<char>(fileY), 
						   std::istreambuf_iterator<char>(), '\n');

		BOOST_REQUIRE (xsize);
		BOOST_REQUIRE (ysize);
		BOOST_TEST_MESSAGE (" X (live)  size is " << xsize);
		BOOST_TEST_MESSAGE (" Y (score) size is " << ysize);
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
			if(strstr(line.c_str(), "marker")) {
				dtw->addMarkerToScore(iter);
				//BOOST_TEST_MESSAGE("added score marker " << iter);
			}

			iter = dtw->processScoreFV(f_feat);
		}
	
		BOOST_CHECK(iter == ysize);
		BOOST_CHECK(dtw->isScoreLoaded());

		dtw->start();
		iter = 0;
	
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

			if(strstr(line.c_str(), "marker")) {
				if (dtw->addMarkerToLive(iter) % 50 == 0)
					cout << endl;
//				BOOST_TEST_MESSAGE("added live marker " << iter);
				cout << ".";
			}

//			BOOST_TEST_MESSAGE ("now at iter: " << iter <<"; t = " << dtw->getT() << " h = "<< dtw->getH());
			BOOST_REQUIRE (iter == dtw->getT(), "iter " << iter << " vs t " << dtw->getT());
			iter ++;
		
			BOOST_TEST_CHECKPOINT ("now at t = " << dtw->getT() << " h = "<< dtw->getH());
			dtw->processLiveFV(f_feat);
		}
		
		cout << endl;

		BOOST_WARN_MESSAGE(dtw->getT() == xsize, "score end reached early! t is " << dtw->getT());
		BOOST_WARN_MESSAGE(!dtw->isRunning(), "score end not reached! h is " << dtw->getH());


		BOOST_TEST_MESSAGE(" X, Y markers: " << dtw->getMarkerCount());
		for(int i = 0; i < dtw->getMarkerCount(); i++) {
			BOOST_TEST_CHECKPOINT ("now at i = " << i);
//			BOOST_TEST_MESSAGE(to_string((long double)dtw->getMarker(i, M_IDEAL)) << ", " << 
//								to_string((long double)dtw->getMarker(i, M_SCORED)));
		}

		int thresh = 25;
		for (int i = 0; i < dtw->getMarkerCount(); i++) {
			int Xmarker = dtw->getMarker(i, M_LIVE);
			int Ymarker = dtw->getMarker(i, M_SCORED);
			BOOST_CHECK_MESSAGE(abs((int)dtw->getHistory(Xmarker) - Ymarker) <= thresh,
				"mk "<< i <<" fail! t = " << Xmarker << "; re = "<<dtw->getHistory(Xmarker) << "; id = " << Ymarker
				<< " | diff = " << (int)dtw->getHistory(Xmarker) - Ymarker);
		}

		fileX.close();
		fileY.close();
	}
}

// ===============================================

BOOST_AUTO_TEST_SUITE_END()