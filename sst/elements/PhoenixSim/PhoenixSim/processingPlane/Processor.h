/*
 * Processor.h
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <omnetpp.h>
#include <string.h>

#include "Application.h"

#include "NetworkAddress.h"
#include "AddressTranslator.h"

#include "Arbiter.h"

#include "sim_includes.h"

#include "DRAM_Cfg.h"

#include "thermalmodel.h"
#include <queue>

using namespace std;


class Processor : public cSimpleModule {
public:
	Processor();
	virtual ~Processor();

	static AddressTranslator* translator;

	static int numOfProcs;
	int myNum;

protected:
		virtual void initialize (int stages);
		virtual void finish();
		virtual void handleMessage (cMessage *msg);
		virtual int numInitStages() const;
		Application* app;

		void sendRequest(ProcessorData* pdata);

		bool isStalled;   //waiting for communication or cache

		NetworkAddress* myAddr;

		simtime_t clockPeriod_cntrl;

		simtime_t procOverhead;   //in seconds

		static DRAM_Cfg* dramcfg;
		static bool dram_init;

		static int numDone;

		bool outstandingRequest;

		StatObject* SO_latency;
		StatObject* SO_msg;
		StatObject* SO_bw;

		cGate* fromNic;
		cGate* toNic;
		cGate* reqOut;

		static int msgsarrived;

		queue<ProcessorData*> theQ;

		int flit_width;

		bool isNetworkSideConcentration;

		map<int, int> msgRx;

};

#endif /* PROCESSOR_H_ */
