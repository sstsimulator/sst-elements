/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPRANDOMDRAM_H_
#define APPRANDOMDRAM_H_

#include <omnetpp.h>

#include "Application.h"

class AppRandomDRAM : public Application {   //does poisson arrival, exponential size
public:
	AppRandomDRAM(int i, int n, DRAM_Cfg* d);
	virtual ~AppRandomDRAM();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* sending(ApplicationData* pdata);

private:
	double arrival;

	int numMessages;

	int progress;

	bool locked;

};

#endif /* APPRANDOMDRAM_H_ */
