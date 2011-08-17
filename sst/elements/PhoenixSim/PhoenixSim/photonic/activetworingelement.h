/*
 * activeringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef ACTIVETWORINGELEMENT_H_
#define ACTIVETWORINGELEMENT_H_

#include "activeelement.h"
#include "tworingelement.h"

class ActiveTwoRingElement : public ActiveElement, public TwoRingElement
{
	public:
		ActiveTwoRingElement();
		virtual ~ActiveTwoRingElement();
		virtual void initialize();


		virtual void SetState(int newState);
		virtual void SetRoutingTable() = 0;
		virtual void SetRoutingTableAlt() = 0;
		virtual void SetOffResonanceRoutingTable() = 0;

		virtual bool StateChangeDestroysSignal(int newState, PacketStat *ps);

		virtual int AccessRoutingTable(int index, PacketStat *ps);
};

#endif /* ACTIVETWORINGELEMENT_H_ */
