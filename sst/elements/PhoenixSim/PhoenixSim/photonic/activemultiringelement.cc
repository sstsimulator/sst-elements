/*
 * activeringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "activemultiringelement.h"

ActiveMultiRingElement::ActiveMultiRingElement()
{
}

ActiveMultiRingElement::~ActiveMultiRingElement()
{
}

void ActiveMultiRingElement::initialize()
{
	// Required to be initialized to correctly set routing table.
	this->currState = 0;

	InitializeBasicElement();
	InitializeMultiRingElement();
	InitializeActiveElement();
	Setup();
}

int ActiveMultiRingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	//RingElement *myself = dynamic_cast<RingElement*>(this);

	return MultiRingElement::AccessRoutingTable(index, ps);
}

bool ActiveMultiRingElement::StateChangeDestroysSignal(int newState, PacketStat *ps)
{
	int currRingSetState, newRingSetState;
	int resonantSet;

	if(IsInResonance(ps->wavelength, resonantSet))
	{
		currRingSetState = GetRingSetState(currState,resonantSet);
		newRingSetState = GetRingSetState(newState,resonantSet);
		if(currRingSetState == newRingSetState)
		{
			return false;
		}
	}

	return true;
}

int ActiveMultiRingElement::GetRingSetState(int deviceState, int ringSet)
{
	return (deviceState>>ringSet) & 1;
}
