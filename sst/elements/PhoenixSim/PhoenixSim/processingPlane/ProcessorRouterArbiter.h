/*
 * RouterArbiter.h
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#ifndef PROCROUTERARBITER_H_
#define PROCROUTERARBITER_H_

#include "RouterArbiter.h"


using namespace std;

class ProcessorRouterArbiter: public RouterArbiter {
public:
	ProcessorRouterArbiter();


	virtual void handleMessage(cMessage *msg);
	int served;

};

#endif /* PROCROUTERARBITER_H_ */
