/*
 * AppAll2All.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef AppBcastTest_H_
#define AppBcastTest_H_

#include <omnetpp.h>

#include "Application.h"

class AppBcastTest : public Application {
public:

	AppBcastTest(int i, int n,  DRAM_Cfg* cfg);
	virtual ~AppBcastTest();

	void init();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application


private:

	static int bcastRcvd;
	static int responseRcvd;



};

#endif /* AppBcastTest_H_ */
