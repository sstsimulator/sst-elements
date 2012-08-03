/*
 * Arbiter_PM_Node.h
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#ifndef ARBITER_PM_NODE_H_
#define ARBITER_PM_NODE_H_

#include "PhotonicArbiter.h"

class Arbiter_PM_Node: public PhotonicArbiter {
public:
	Arbiter_PM_Node();
	virtual ~Arbiter_PM_Node();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int pathStatus(ArbiterRequestMsg* rmsg, int outport);
	virtual int getBcastOutport(ArbiterRequestMsg* bmsg);

	enum PORTS
	{
		Node_N = 0,
		Node_E,
		Node_S,
		Node_W,
		Node_Out
	};
};

#endif /* ARBITER_PM_NODE_H_ */
