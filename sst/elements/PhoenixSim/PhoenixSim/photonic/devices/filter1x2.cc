//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "filter1x2.h"

Define_Module(Filter1x2);

Filter1x2::Filter1x2()
{
}

Filter1x2::~Filter1x2()
{
}


void Filter1x2::Setup()
{

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	gateIdIn[0] = gate("photonicHorizIn$i")->getId();
	gateIdIn[1] = gate("photonicVertIn$i")->getId();
	gateIdIn[2] = gate("photonicHorizOut$i")->getId();
	gateIdIn[3] = gate("photonicVertOut$i")->getId();

	gateIdOut[0] = gate("photonicHorizIn$o")->getId();
	gateIdOut[1] = gate("photonicVertIn$o")->getId();
	gateIdOut[2] = gate("photonicHorizOut$o")->getId();
	gateIdOut[3] = gate("photonicVertOut$o")->getId();

	portType[0] = inout;
	portType[1] = inout;
	portType[2] = inout;
	portType[3] = inout;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");
	Crosstalk_Cross = par("Crosstalk_Cross");

    RingThrough_ER_Filter1x2 = par("RingThrough_ER_Filter1x2");
    RingDrop_ER_Filter1x2 = par("RingDrop_ER_Filter1x2");
    ThroughDelay_Filter1x2 = par("ThroughDelay_Filter1x2");
    DropDelay_Filter1x2 = par("DropDelay_Filter1x2");


	debug(getFullPath(), "INIT: done", UNIT_INIT);

}

double Filter1x2::GetLatency(int indexIn, int indexOut)
{
	//assumes 3 um ring
	if((indexIn == 0 && indexOut == 2) || (indexIn == 2 && indexOut == 0))
	{
		// 12um path
		return 0.13714e-012;
	}
	else if((indexIn == 1 && indexOut ==3) || (indexIn == 3 && indexOut ==1))
	{
		// 12um path
		return 0.13714e-012;
	}
	else if((indexIn == 0 && indexOut ==3) || (indexIn == 3 && indexOut ==0))
	{
		// Through ring ~13um path
		return 0.15264e-012;
	}
	else
	{
		// 12um path
		return 0.13714e-012;
	}
}

double Filter1x2::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0))
	{
		IL += PropagationLoss * (30+30);
	}
	else
	{
		if(indexIn == 0 || indexIn == 3)
		{
			IL += PropagationLoss * 55;
		}
		else
		{
			IL += PropagationLoss * 25;
		}
		if(indexOut == 0 || indexOut == 3)
		{
			IL += PropagationLoss * 55;
		}
		else
		{
			IL += PropagationLoss * 25;
		}
	}
	return IL;
}

double Filter1x2::GetCrossingLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		if((indexIn+2)%4 == indexOut)
		{
			return CrossingLoss;
		}
		else if((indexIn == 0 && indexOut == 1)
			|| (indexIn == 1 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 1)
			|| (indexIn == 2 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 2))
		{
			return Crosstalk_Cross;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return MAX_INSERTION_LOSS;
	}
}

double Filter1x2::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0))
	{
		IL += PassThroughRingLoss;
		if(!IsInResonance(wavelength))
		{
			IL += RingDrop_ER_Filter1x2;
		}
	}
	return IL;
}

double Filter1x2::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if((indexIn == 1 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 1)
			|| (indexIn == 0 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 0))
	{
		IL += PassByRingLoss;

		if(IsInResonance(wavelength))
		{
			IL += RingThrough_ER_Filter1x2;
		}
	}
	return IL;
}

void Filter1x2::SetRoutingTable()
{
	routingTable[0] = 3;
	routingTable[1] = -1;
	routingTable[2] = -1;
	routingTable[3] = 0;
}

void Filter1x2::SetOffResonanceRoutingTable()
{
	routingTableOffResonance[0] = 2;
	routingTableOffResonance[1] = 3;
	routingTableOffResonance[2] = 0;
	routingTableOffResonance[3] = 1;
}
