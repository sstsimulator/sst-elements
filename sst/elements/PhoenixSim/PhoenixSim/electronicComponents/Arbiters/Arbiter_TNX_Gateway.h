/*
 * Arbiter_TNX_Gateway.h
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#ifndef ARBITER_TNX_GATEWAY_H_
#define ARBITER_TNX_GATEWAY_H_

#include "PhotonicArbiter.h"

class Arbiter_TNX_Gateway: public PhotonicArbiter {
public:
	Arbiter_TNX_Gateway();
	virtual ~Arbiter_TNX_Gateway();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	enum PORTS
	{
		GW_N = 0,
		GW_E,
		GW_S,
		GW_W,
		GW_Out
	};
};

#endif /* ARBITER_TNX_GATEWAY_H_ */
