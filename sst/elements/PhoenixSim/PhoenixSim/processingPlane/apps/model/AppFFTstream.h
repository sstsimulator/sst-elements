/*
 * AppFFT.h
 *
 *  Created on: Feb 16, 2009
 *      Author: SolidSnake
 */

#ifndef APPFFTSTREAM_H_
#define APPFFTSTREAM_H_
#include <omnetpp.h>

#include <queue>

#include "Application.h"

using namespace std;


class AppFFTstream : public Application {
public:

	AppFFTstream(int i, int n,  DRAM_Cfg* cfg);
	virtual ~AppFFTstream();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	ApplicationData* getNextMsg();
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application

	virtual ApplicationData* sending(ApplicationData* p);
private:


	int kernelId;
	int numKernels;
	long long datasize;

	int writeAddr;

	ApplicationData* firstFFT;

	bool sentDone;

	queue<ApplicationData*> messages;
	bool busy;
	bool firstReadSent;


	void init(int n, int start);

};

#endif /* APPFFT_H_ */
