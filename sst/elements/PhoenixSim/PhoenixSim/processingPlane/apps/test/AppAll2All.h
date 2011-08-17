/*
 * AppAll2All.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPALL2ALL_H_
#define APPALL2ALL_H_

#include <omnetpp.h>

#include "Application.h"

class AppAll2All : public Application {
public:

	AppAll2All(int i, int n,  DRAM_Cfg* cfg);
	virtual ~AppAll2All();

	void init();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* sending(ApplicationData* pdata);  //data is done processing, sent to NIF


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
