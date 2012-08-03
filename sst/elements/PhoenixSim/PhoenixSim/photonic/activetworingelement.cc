/*
 * activeringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "activetworingelement.h"

ActiveTwoRingElement::ActiveTwoRingElement()
{
}

ActiveTwoRingElement::~ActiveTwoRingElement()
{
}

void ActiveTwoRingElement::initialize()
{
	// Required to be initialized to correctly set routing table.
	this->currState = 0;

	InitializeBasicElement();
	InitializeRingElement();
	InitializeTwoRingElement();
	InitializeActiveElement();
	Setup();
}

int ActiveTwoRingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	//RingElement *myself = dynamic_cast<RingElement*>(this);

	return TwoRingElement::AccessRoutingTable(index, ps);
}


void ActiveTwoRingElement::SetState(int newState)
{
	currState = newState;
	SetRoutingTable();
	SetRoutingTableAlt();
}

bool ActiveTwoRingElement::StateChangeDestroysSignal(int newState, PacketStat *ps)
{

	if(((currState == 0 || currState == 2)&&(newState == 1 || newState == 3)) ||
			((currState == 1 || currState == 3)&&(newState == 0 || newState == 2)) )

	{
		return IsInResonance(ps->wavelength);
	}
	else if(((currState == 0 || currState == 1)&&(newState == 2 || newState == 3)) ||
			((currState == 2 || currState == 3)&&(newState == 0 || newState == 1)) )
	{
		this->IsInResonanceAlt(ps->wavelength);
	}

	return false;
}
