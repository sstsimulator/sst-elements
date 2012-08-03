/*
 * Arbiter_EM.h
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#ifndef ARBITER_CM_H_
#define ARBITER_CM_H_

#include "ElectronicArbiter.h"


class Arbiter_CM : public ElectronicArbiter{
public:
	Arbiter_CM();
	virtual ~Arbiter_CM();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);


	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);

	//bool isConc;

	enum PORTS {
		CM_N = 0,
		CM_E=1,
		CM_S=2,
		CM_W=3,
		CM_Node=4,
		CM_Mem
	};

};

#endif /* ARBITER_EM_H_ */
