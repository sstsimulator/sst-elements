/*
 * AppAll2All.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPONE2ONEDRAM_H_
#define APPONE2ONEDRAM_H_

#include <omnetpp.h>

#include "Application.h"

class AppOne2OneDRAM : public Application {
public:
	AppOne2OneDRAM(int i, int n,  DRAM_Cfg* cfg);
	virtual ~AppOne2OneDRAM();

	void init();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* sending(ApplicationData* adata);

private:
	int numSent;
	bool doneSent;

};

#endif /* APPALL2ALL_H_ */
