/*
 * activeringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "activeringelement.h"

ActiveRingElement::ActiveRingElement()
{
}

ActiveRingElement::~ActiveRingElement()
{
}

void ActiveRingElement::initialize()
{
	// Required to be initialized to correctly set routing table.
	this->currState = 0;

	InitializeBasicElement();
	InitializeRingElement();
	InitializeActiveElement();
	Setup();
}

int ActiveRingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	//RingElement *myself = dynamic_cast<RingElement*>(this);

	return RingElement::AccessRoutingTable(index, ps);
}

bool ActiveRingElement::StateChangeDestroysSignal(int newState, PacketStat *ps)
{
	std::cout<<"ActiveRingElement.cc\\StateChangeDestroysSignal() - this should probably never be called"<<endl;
	return IsInResonance(ps->wavelength);
}
