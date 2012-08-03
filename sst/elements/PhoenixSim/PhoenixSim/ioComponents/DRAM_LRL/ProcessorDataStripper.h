
#ifndef PROC_DATA_STRIP_H_
#define PROC_DATA_STRIP_H_

#include <omnetpp.h>
#include "sim_includes.h"
#include "messages_m.h"

#include "NetworkAddress.h"
#include "statistics.h"

using namespace std;

class PhotonicToProcessorMsg: public cSimpleModule {

protected:
	virtual void handleMessage(cMessage* msg);
	virtual void initialize();

	cGate* procOut;
	cGate* procIn;
	cGate* photonicIn;
	cGate* photonicOut;

	int wavelengths;
	double photonicBitPeriod;

};

class ElectronicToProcessorMsg: public cSimpleModule {

protected:
	virtual void handleMessage(cMessage* msg);
	virtual void initialize();
	cGate* procOut;
	cGate* procIn;
	cGate* electronicIn;
	cGate* electronicOut;
};

#endif
