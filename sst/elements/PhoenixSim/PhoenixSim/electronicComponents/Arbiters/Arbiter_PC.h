/*
 * Arbiter_EM.h
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#ifndef ARBITER_PC_H_
#define ARBITER_PC_H_

#include "ElectronicArbiter.h"


class Arbiter_PC : public ElectronicArbiter{
public:
	Arbiter_PC();
	virtual ~Arbiter_PC();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);


	//bool isConc;


};

#endif /* ARBITER_PC_H_ */
