/*
 * ringelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "tworingelement.h"

TwoRingElement::TwoRingElement()
{
}

TwoRingElement::~TwoRingElement()
{
}

void TwoRingElement::initialize()
{
	InitializeBasicElement();
	InitializeRingElement();
	InitializeTwoRingElement();
	Setup();
	if(isBroadband)
	{
		throw cRuntimeError("Cannot operate two ring element as broadband device.");
	}
}

void TwoRingElement::InitializeTwoRingElement()
{
	if(hasPar("filterBaseWavelengthAlt"))
	{
		filterBaseWavelengthAlt = par("filterBaseWavelengthAlt");
	}

	if(hasPar("numOfResonantWavelengthsAlt"))
	{
		for(int i = 0; i < par("numOfResonantWavelengthsAlt").longValue() ; i++)
		{
			resonantWavelengthsAlt.insert(filterBaseWavelengthAlt + i*(par("filterWavelengthSpacingAlt").doubleValue()));
		}
	}
	else
	{
		resonantWavelengthsAlt.insert(filterBaseWavelengthAlt);
	}

	if(useRingModel)
	{
		ringLengthAlt = PhyLayer::pi*(par("ringDiameterAlt").doubleValue());
		ringFinesseAlt = 10000;
	}

	routingTableAlt.resize(numOfPorts,-1);

	SetRoutingTableAlt();

}


int TwoRingElement::AccessRoutingTable(int index, PacketStat *ps)
{
	if(IsInResonance(ps->wavelength))
	{
		return Route(index);
	}
	else if(IsInResonanceAlt(ps->wavelength))
	{
		return RouteAlt(index);
	}
	else
	{
		return RouteOffResonance(index);
	}
}

int TwoRingElement::RouteAlt(int index)
{
	return routingTableAlt[index];
}


bool TwoRingElement::IsInResonanceAlt(double wavelength)
{
	double n = PhyLayer::RefractiveIndex("Si");
	double K;
	int near1, near2;
	if(useRingModel)
	{
		K = 2*n*ringLengthAlt/wavelength;
		near1 = (int)ceil(K);
		near2 = (int)floor(K);


		//if(fabs(diff1 - wavelength) < (diff1-diff2)/ringFinesse || fabs(diff2 - wavelength) < (diff1-diff2)/ringFinesse)
		if(fabs(K - near1) < .001 || fabs(K - near2) < .001)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return resonantWavelengthsAlt.find(wavelength) != resonantWavelengthsAlt.end();
	}
}
