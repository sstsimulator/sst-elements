/*
 * GatewaySignalControl.h
 *
 *  Created on: Jun 7, 2009
 *      Author: SolidSnake
 */

#ifndef GATEWAYSIGNALCONTROL_H_
#define GATEWAYSIGNALCONTROL_H_

#include <omnetpp.h>
#include "sim_includes.h"
#include "messages_m.h"

using namespace std;

class GatewaySignalControl : public cSimpleModule {
public:
	GatewaySignalControl();
	virtual ~GatewaySignalControl();

protected:
	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);

	map<int, cGate*> toSel;

	bool doVWT;

	cGate* toLBC;
	cGate* toAtt;

	cGate* elInProc;
	cGate* elOutProc;

	cGate* elInNet;
	cGate *elOutNet;

	int wavelengths;
	int used;

	cFSM state;

	enum{
	SLEEP = FSM_Steady(0),
	SendLBC = FSM_Transient(1),
	WaitACK = FSM_Steady(2),
	PrepareSignal = FSM_Transient(3),
	WaitTeardown = FSM_Steady(4),
	TurnOff = FSM_Transient(5)
	};


};

#endif /* GATEWAYSIGNALCONTROL_H_ */
