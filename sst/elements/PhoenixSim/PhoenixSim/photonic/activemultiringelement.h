/*
 * activeringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef ACTIVEMULTIRINGELEMENT_H_
#define ACTIVEMULTIRINGELEMENT_H_

#include "activeelement.h"
#include "multiringelement.h"

class ActiveMultiRingElement : public ActiveElement, public MultiRingElement
{
	public:
		ActiveMultiRingElement();
		virtual ~ActiveMultiRingElement();
		virtual void initialize();

		virtual void SetRoutingTable() = 0;
		virtual int GetMultiRingRoutingTable(int index, int ringSet) = 0;
		virtual int AccessRoutingTable(int index, PacketStat *ps);
		virtual bool StateChangeDestroysSignal(int newState, PacketStat *ps);

		int GetRingSetState(int deviceState, int ringSet);
};

#endif /* ACTIVEMULTIRINGELEMENT_H_ */
