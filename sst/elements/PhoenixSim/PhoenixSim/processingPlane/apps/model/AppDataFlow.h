/*
 * AppRandom.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPDATAFLOW_H_
#define APPDATAFLOW_H_

#include <omnetpp.h>

#include "Application.h"

class AppDataFlow : public Application {   //does poisson arrival, exponential size
public:
	AppDataFlow(int i, int n, DRAM_Cfg* d);
	virtual ~AppDataFlow();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* sending(ApplicationData* pdata);

	ApplicationData* writeMemory();
	ApplicationData* readMemory();

private:

	int size;
	int numMessages;

	int progress;



	bool peripheral;



};

#endif /* APPRANDOMDRAM_H_ */
