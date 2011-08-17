/*
 * Arbiter_EM.h
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#ifndef ARBITER_EM_H_
#define ARBITER_EM_H_

#include "ElectronicArbiter.h"


class Arbiter_EM : public ElectronicArbiter{
public:
	Arbiter_EM();
	virtual ~Arbiter_EM();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);


	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getBcastOutport(ArbiterRequestMsg* bmsg);
	//bool isConc;

	enum PORTS {
		EM_N = 0,
		EM_E=1,
		EM_S=2,
		EM_W=3,
		EM_Node=4
	};

};

#endif /* ARBITER_EM_H_ */
