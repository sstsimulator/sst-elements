/*
 * Arbiter_EM.h
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#ifndef ARBITER_PR_H_
#define ARBITER_PR_H_

#include "ElectronicArbiter.h"


class Arbiter_PR : public ElectronicArbiter{
public:
	Arbiter_PR();
	virtual ~Arbiter_PR();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);

};

#endif /* ARBITER_EM_H_ */
