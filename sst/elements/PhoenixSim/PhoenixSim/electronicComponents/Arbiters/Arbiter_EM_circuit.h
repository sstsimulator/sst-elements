/*
 * Arbiter_PM_Node.h
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#ifndef ARBITER_EM_CIRCUIT_H_
#define ARBITER_EM_CIRCUIT_H_

#include "PhotonicNoUturnArbiter.h"

class Arbiter_EM_Circuit: public PhotonicNoUturnArbiter  {
public:
	Arbiter_EM_Circuit();
	virtual ~Arbiter_EM_Circuit();

	virtual void init(string id, int lev, int x, int y, int vc, int ports,
			int numpse, int buff_size, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);
	enum PORTS
	{
		Node_N = 0,
		Node_E,
		Node_S,
		Node_W,
		Node_Out
	};
};

#endif
