/*
 * Application.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <omnetpp.h>
#include "messages_m.h"


#include "NetworkAddress.h"

#include "SizeDistribution.h"

#include "statistics.h"

#include "sim_includes.h"

#include "DRAM_Cfg.h"

using namespace std;

#define MEM_READ_SIZE 64

class Application {
public:

	Application(int i, int n, DRAM_Cfg* cfg);
	virtual ~Application();


	virtual simtime_t process(ApplicationData* pdata) = 0;  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg() = 0;  //returns the first message to be processed

	virtual ApplicationData* dataArrive(ApplicationData* pdata){return NULL;};  //data has arrived at the destination
	virtual ApplicationData* sending(ApplicationData* pdata){return NULL;};  //data is done processing, sent to NIF
	virtual ApplicationData* msgSent(ApplicationData* pdata){return NULL;};  //data has been transmitted by NIF
	virtual ApplicationData* msgCreated(ApplicationData* pdata){return NULL;};  //data has created by App

	virtual void finish();

	static double param1;
	static double param2;
	static double param3;
	static double param4;

	static string param5;

	static double ePerOp;

	static double clockPeriod;


	static SizeDistribution* sizeDist;

	simtime_t currentTime;

	bool useioplane;


protected:
	int id;
	int numOfProcs;



	DRAM_Cfg* dramcfg;

	static long bytesTransferred;
	static int msgsRx;
	static double latencyTime;
	static bool statsReported;




};

#endif /* APPLICATION_H_ */
