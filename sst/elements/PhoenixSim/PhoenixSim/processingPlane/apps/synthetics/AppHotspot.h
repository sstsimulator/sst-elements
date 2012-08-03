/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPHOTSPOT_H_
#define APPHOTSPOT_H_

#include <omnetpp.h>
#include <assert.h>
#include "Application.h"

class AppHotspot : public Application {   //does poisson arrival, exponential size
public:
	AppHotspot(int i, int n, double arr, int s);
	virtual ~AppHotspot();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* msgCreated(ApplicationData* pdata);

private:
	double arrival;
	int size;
	int numMessages;
	int progress;


	static int spot;

};

#endif /* APPRANDOM_H_ */
