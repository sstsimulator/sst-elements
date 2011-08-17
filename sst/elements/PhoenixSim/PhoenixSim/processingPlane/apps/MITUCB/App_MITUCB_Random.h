/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPMITUCBRANDOM_H_
#define APPMITUCBRANDOM_H_

#include <omnetpp.h>
#include <assert.h>
#include "Application.h"

class App_MITUCB_Random : public Application {   //does poisson arrival, exponential size
public:
	App_MITUCB_Random(int i, int n, double arr, int s);
	virtual ~App_MITUCB_Random();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* msgSent(ApplicationData* pdata);

private:
	double arrival;
	int size;
	int numMessages;
	int progress;
};

#endif /* APPMITUCBRANDOM_H_ */
