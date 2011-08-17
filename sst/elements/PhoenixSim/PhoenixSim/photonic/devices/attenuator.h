/*
 * attenuator.h
 *
 *  Created on: Jun 7, 2009
 *      Author: SolidSnake
 */

#ifndef ATTENUATOR_H_
#define ATTENUATOR_H_

#include <omnetpp.h>
#include "sim_includes.h"
#include "messages_m.h"

using namespace std;

class attenuator : public cSimpleModule {
public:
	attenuator();
	virtual ~attenuator();

protected:
	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);

	long level;

	cGate* phIn;
	cGate* cntrl;
	cGate* phOut;
};

#endif /* ATTENUATOR_H_ */
