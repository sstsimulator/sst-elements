/*
 * AppFFT.h
 *
 *  Created on: Feb 16, 2009
 *      Author: SolidSnake
 */

#ifndef APPFFT_H_
#define APPFFT_H_
#include <omnetpp.h>

#include "Application.h"


class AppFFT : public Application {
public:
	AppFFT(int i, int n, DRAM_Cfg* cfg);
	virtual ~AppFFT();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
private:

	int stage;
	int kernelId;
	int numKernels;
	int datasize;

	bool sentDone;

	bool doRead;
	bool doFFT;
	bool doWrite;



	map<int, bool> rcvd;


	void init(int n, int start);

};

#endif /* APPFFT_H_ */
