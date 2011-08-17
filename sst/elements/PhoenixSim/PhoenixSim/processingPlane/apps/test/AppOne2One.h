/*
 * AppAll2All.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPONE2ONE_H_
#define APPONE2ONE_H_

#include <omnetpp.h>

#include "Application.h"

class AppOne2One : public Application {
public:

	AppOne2One(int i, int n,  DRAM_Cfg* cfg);
	virtual ~AppOne2One();

	void init();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application


private:
	int next;
	int stage;

	bool goMemory;
	bool doneSent;



	static int notifySent;
	static int notifyRcvd;

	static int accessRcvd;
	static int accessSent;

	int packetSize;
};

#endif /* APPALL2ALL_H_ */
