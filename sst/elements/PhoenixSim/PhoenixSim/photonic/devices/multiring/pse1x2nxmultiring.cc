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

#include "pse1x2nxmultiring.h"

Define_Module(PSE1x2NXMultiRing);


PSE1x2NXMultiRing::PSE1x2NXMultiRing()
{
}

PSE1x2NXMultiRing::~PSE1x2NXMultiRing()
{
}

void PSE1x2NXMultiRing::Setup()
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


double PSE1x2NXMultiRing::GetLatency(int indexIn, int indexOut)
{
	return latencyMatrix[indexIn][indexOut];
}

double PSE1x2NXMultiRing::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	int ringSet;
	IsInResonance(wavelength,ringSet);
	double ringDiameter = this->ringDiameter[0];

	if((indexIn==0 && indexOut == 3) ||(indexIn==3 && indexOut == 0))
	{
		IL += 2*(5 + ringDiameter/2 + ringDiameter*(ringSet));
	}
	else if((indexIn==0 && indexOut == 3) ||(indexIn==3 && indexOut == 0))
	{
		IL +=  2*(5 + ringDiameter/2 + ringDiameter*(numOfRingSets - 1 - ringSet));
	}
	else
	{
		IL +=  5 + (ringDiameter+5)*numOfRingSets;
	}

	return IL*PropagationLoss;


}


double PSE1x2NXMultiRing::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);

	if((indexIn == 0 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 1))
	{
		IL += PassThroughRingLoss;

		if(resonant && GetRingSetState(currState,ringSet) == 0)
		{
			IL += RingOn_ER_1x2NX;
		}
	}
	return IL;
}

double PSE1x2NXMultiRing::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);

	if((indexIn == 1 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 1)
			|| (indexIn == 0 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 0))
	{
		IL += PassByRingLoss*numOfRingSets;

		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += RingOff_ER_1x2NX;
		}
	}

	else if(((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)) && resonant)
	{
		IL += PassByRingLoss*2*ringSet;
	}

	else if(((indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut == 1)) && resonant)
	{
		IL += PassByRingLoss*2*(numOfRingSets-1-ringSet);
	}


	return IL;
}


double PSE1x2NXMultiRing::GetPowerLevel(int state)
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

double PSE1x2NXMultiRing::GetEnergyDissipation(int stateBefore, int stateAfter)
{
	double energyTotal = 0;

	if(stateBefore != stateAfter)
	{
		if(stateBefore&1 == 0 && stateAfter&1 == 1)
		{
			energyTotal += RingDynamicOffOn;
		}

		if(stateBefore&1 == 1 && stateAfter&1 == 0)
		{
			energyTotal += RingDynamicOnOff;
		}

		if((stateBefore>>1)&1 == 0 && (stateAfter>>1)&1 == 1)
		{
			energyTotal += RingDynamicOffOn;
		}

		if((stateBefore>>1)&1 == 1 && (stateAfter>>1)&1 == 0)
		{
			energyTotal += RingDynamicOnOff;
		}

	}
	return energyTotal;
}

void PSE1x2NXMultiRing::SetRoutingTable()
{
	// off resonance case
	routingTable[0] = 1;
	routingTable[1] = 0;
	routingTable[2] = 3;
	routingTable[3] = 2;

}


int PSE1x2NXMultiRing::GetMultiRingRoutingTable(int index, int ringSet)
{
	int ringState = GetRingSetState(currState,ringSet);

	if(ringState == 1)
	{
		switch(index)
		{
		case 0:
			return 3;
		case 1:
			return 2;
		case 2:
			return 1;
		case 3:
			return 0;
		}
	}
	else
	{
		return routingTable[index];
	}
}
