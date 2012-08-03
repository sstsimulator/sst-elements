/*
 * activeringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef ACTIVERINGELEMENT_H_
#define ACTIVERINGELEMENT_H_

#include "activeelement.h"
#include "ringelement.h"

class ActiveRingElement : public ActiveElement, public RingElement
{
	public:
		ActiveRingElement();
		virtual ~ActiveRingElement();
		virtual void initialize();

		virtual void SetRoutingTable() = 0;
		virtual void SetOffResonanceRoutingTable() = 0;
		virtual int AccessRoutingTable(int index, PacketStat *ps);

		virtual bool StateChangeDestroysSignal(int newState, PacketStat *ps);
};

#endif /* ACTIVERINGELEMENT_H_ */
