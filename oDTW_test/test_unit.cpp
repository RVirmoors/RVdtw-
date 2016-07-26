#define BOOST_TEST_MODULE DTW test
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "..\oDTW\oDTW.h"


 
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