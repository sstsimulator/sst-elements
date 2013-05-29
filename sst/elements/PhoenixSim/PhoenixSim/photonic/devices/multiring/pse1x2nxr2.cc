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

#include "pse1x2nxr2.h"

Define_Module(PSE1x2NXR2);


PSE1x2NXR2::PSE1x2NXR2()
{
}

PSE1x2NXR2::~PSE1x2NXR2()
{
}

void PSE1x2NXR2::Setup()
{
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = 0;


	gateIdIn[0] = gate("photonicInA$i")->getId();
	gateIdIn[1] = gate("photonicOutA$i")->getId();
	gateIdIn[2] = gate("photonicInB$i")->getId();
	gateIdIn[3] = gate("photonicOutB$i")->getId();

	gateIdOut[0] = gate("photonicInA$o")->getId();
	gateIdOut[1] = gate("photonicOutA$o")->getId();
	gateIdOut[2] = gate("photonicInB$o")->getId();
	gateIdOut[3] = gate("photonicOutB$o")->getId();

	portType[0] = inout;
	portType[1] = inout;
	portType[2] = inout;
	portType[3] = inout;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");

	Crosstalk_Cross = par("Crosstalk_Cross");

	RingOn_ER_1x2NX = par("RingOn_ER_1x2NX");
	RingOff_ER_1x2NX = par("RingOff_ER_1x2NX");

	DropDelay_1x2NX = par("DropDelay_1x2NX");
	ThroughDelay_1x2NX = par("ThroughDelay_1x2NX");


	vector<double> tempLatencyMatrix(numOfPorts,0);
	latencyMatrix.resize(numOfPorts,tempLatencyMatrix);
	// eventually read from file

	latencyMatrix[0][0] = DropDelay_1x2NX;
	latencyMatrix[0][1] = ThroughDelay_1x2NX;
	latencyMatrix[0][2] = DropDelay_1x2NX;
	latencyMatrix[0][3] = ThroughDelay_1x2NX;
	latencyMatrix[1][0] = ThroughDelay_1x2NX;
	latencyMatrix[1][1] = DropDelay_1x2NX;
	latencyMatrix[1][2] = ThroughDelay_1x2NX;
	latencyMatrix[1][3] = DropDelay_1x2NX;
	latencyMatrix[2][0] = DropDelay_1x2NX;
	latencyMatrix[2][1] = ThroughDelay_1x2NX;
	latencyMatrix[2][2] = DropDelay_1x2NX;
	latencyMatrix[2][3] = ThroughDelay_1x2NX;
	latencyMatrix[3][0] = ThroughDelay_1x2NX;
	latencyMatrix[3][1] = DropDelay_1x2NX;
	latencyMatrix[3][2] = ThroughDelay_1x2NX;
	latencyMatrix[3][3] = DropDelay_1x2NX;

	gateIdFromRouter = gate("fromRouter")->getId();

	RingStaticDissipation = par("RingStaticDissipation");
	RingDynamicOffOn = par("RingDynamicOffOn");
	RingDynamicOnOff = par("RingDynamicOnOff");

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}


double PSE1x2NXR2::GetLatency(int indexIn, int indexOut)
{
	return latencyMatrix[indexIn][indexOut];
}

double PSE1x2NXR2::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	return PropagationLoss * (5+5+5+ringDiameter+ringDiameter);

	double IL = 0;
	int ringSet = 0;

	if(IsInResonanceAlt(wavelength))
	{
		ringSet = 1;
	}

	if((indexIn==0 && indexOut == 3) ||(indexIn==3 && indexOut == 0))
	{
		IL += 2*(5 + ringDiameter/2 + ringDiameter*(ringSet));
	}
	else if((indexIn==0 && indexOut == 3) ||(indexIn==3 && indexOut == 0))
	{
		IL +=  2*(5 + ringDiameter/2 + ringDiameter*(1 - ringSet));
	}
	else
	{
		IL +=  5 + (ringDiameter+5)*2;
	}

	return IL*PropagationLoss;
}


double PSE1x2NXR2::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;

	if((indexIn == 0 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 1))
	{
		IL += PassThroughRingLoss;

		if((currState == 0 || currState == 2) && IsInResonance(wavelength))
		{
			IL += RingOn_ER_1x2NX;
		}
		if((currState == 0 || currState == 1) && IsInResonanceAlt(wavelength))
		{
			IL += RingOn_ER_1x2NX;
		}
	}
	return IL;
}

double PSE1x2NXR2::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;

	if((indexIn == 0 && indexOut == 1)
			|| (indexIn == 1 && indexOut == 0)
			|| (indexIn == 2 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 2))
	{
		IL += PassByRingLoss*2;

		if((currState == 1 || currState == 3) && IsInResonance(wavelength))
		{
			IL += RingOff_ER_1x2NX;
		}
		if((currState == 2 || currState == 3) && IsInResonanceAlt(wavelength))
		{
			IL += RingOff_ER_1x2NX;
		}
	}
	else if(((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)) && IsInResonanceAlt(wavelength))
	{
		IL += PassByRingLoss*2;
	}
	else if(((indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut == 1)) && IsInResonance(wavelength))
	{
		IL += PassByRingLoss*2;
	}

	return IL;
}


double PSE1x2NXR2::GetPowerLevel(int state)
{
	switch(state)
	{
	case 0:
		return 0;
		break;
	case 1:
		return RingStaticDissipation * 1;
		break;
	case 2:
		return RingStaticDissipation * 1;
			break;
	case 3:
		return RingStaticDissipation * 2;
			break;

	default:
		return 0;
	};
}

double PSE1x2NXR2::GetEnergyDissipation(int stateBefore, int stateAfter)
{
	double energyTotal = 0;

	if(stateBefore != stateAfter)
	{
		if((stateBefore&1) == 0 && (stateAfter&1) == 1)
		{
			energyTotal += RingDynamicOffOn;
		}

		if((stateBefore&1) == 1 && (stateAfter&1) == 0)
		{
			energyTotal += RingDynamicOnOff;
		}

		if(((stateBefore>>1)&1) == 0 && ((stateAfter>>1)&1) == 1)
		{
			energyTotal += RingDynamicOffOn;
		}

		if(((stateBefore>>1)&1) == 1 && ((stateAfter>>1)&1) == 0)
		{
			energyTotal += RingDynamicOnOff;
		}

	}
	return energyTotal;
}

void PSE1x2NXR2::SetRoutingTable()
{
	if(currState == 1 || currState == 3)
	{
		routingTable[0] = 3;
		routingTable[1] = 2;
		routingTable[2] = 1;
		routingTable[3] = 0;
	}
	else
	{
		routingTable[0] = 1;
		routingTable[1] = 0;
		routingTable[2] = 3;
		routingTable[3] = 2;
	}
}

void PSE1x2NXR2::SetRoutingTableAlt()
{
	if(currState == 2 || currState == 3)
	{
		routingTableAlt[0] = 3;
		routingTableAlt[1] = 2;
		routingTableAlt[2] = 1;
		routingTableAlt[3] = 0;
	}
	else
	{
		routingTableAlt[0] = 1;
		routingTableAlt[1] = 0;
		routingTableAlt[2] = 3;
		routingTableAlt[3] = 2;
	}
}

void PSE1x2NXR2::SetOffResonanceRoutingTable()
{
	routingTableOffResonance[0] = 1;
	routingTableOffResonance[1] = 0;
	routingTableOffResonance[2] = 3;
	routingTableOffResonance[3] = 2;
}

