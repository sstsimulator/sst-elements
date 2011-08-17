/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef AppRandom_FiniteQueue_H_
#define AppRandom_FiniteQueue_H_

#include <omnetpp.h>

#include "Application.h"

class AppRandom_FiniteQueue : public Application {   //does poisson arrival, exponential size
public:
	AppRandom_FiniteQueue(int i, int n, double arr, int s);
	virtual ~AppRandom_FiniteQueue();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* msgSent(ApplicationData* pdata);

private:
	double arrival;
	int size;
	int numMessages;

	int progress;
	bool first;



};

#endif /* AppRandom_FiniteQueue_H_ */
