/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPBITREVERSE_H_
#define APPBITREVERSE_H_

#include <omnetpp.h>
#include <assert.h>
#include "Application.h"

class AppBitreverse : public Application {   //does poisson arrival, exponential size
public:
	AppBitreverse(int i, int n, double arr, int s);
	virtual ~AppBitreverse();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* msgCreated(ApplicationData* pdata);

private:
	double arrival;
	int size;
	int numMessages;
	int progress;

	int myDest;


};

#endif /* APPRANDOM_H_ */
