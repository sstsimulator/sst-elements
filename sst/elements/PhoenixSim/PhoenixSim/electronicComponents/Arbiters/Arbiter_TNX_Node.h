/*
 * Arbiter_TNX_Node.h
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#ifndef ARBITER_TNX_NODE_H_
#define ARBITER_TNX_NODE_H_

#include "Photonic4x4Arbiter.h"

class Arbiter_TNX_Node: public Photonic4x4Arbiter {
public:
	Arbiter_TNX_Node();
	virtual ~Arbiter_TNX_Node();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);



};

#endif /* ARBITER_TNX_NODE_H_ */
