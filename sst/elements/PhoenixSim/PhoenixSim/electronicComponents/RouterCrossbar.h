/*
 * RouterCrossbar.h
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#ifndef ROUTERCROSSBAR_H_
#define ROUTERCROSSBAR_H_

#include <omnetpp.h>
#include <map>
#include "messages_m.h"
#include "sim_includes.h"

#ifdef ENABLE_ORION
#include "orion/ORION_Crossbar.h"
#include "orion/ORION_Util.h"
#endif

using namespace std;

class RouterCrossbar: public cSimpleModule {
public:
	RouterCrossbar();
	virtual ~RouterCrossbar();

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();

	int numPorts;
	int flit_width;
	int buffer_size;
	double clockPeriod;
	map<int, int> xbar; //inport -> outport

	map<int, cGate*> ports;
	map<int, cGate*> inports;

	cGate* cntrlIn;
	cGate* cntrlOut;

	double lastEnergy;

	bool autounblock;

	StatObject* P_static;
	StatObject* E_dynamic;

#ifdef ENABLE_ORION
	ORION_Crossbar* power;

#endif
};

#endif /* ROUTERCROSSBAR_H_ */
