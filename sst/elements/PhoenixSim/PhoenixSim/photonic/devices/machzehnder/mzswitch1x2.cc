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

#include "mzswitch1x2.h"

Define_Module(MZSwitch1x2);


MZSwitch1x2::MZSwitch1x2()
{
}

MZSwitch1x2::~MZSwitch1x2()
{
}

void MZSwitch1x2::Setup()
{

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = 0;

	// this routing is slightly inaccurate


	gateIdIn[0] = gate("photonicIn$i")->getId();
	gateIdIn[1] = gate("photonicOutA$i")->getId();
	gateIdIn[2] = gate("photonicOutB$i")->getId();

	gateIdOut[0] = gate("photonicIn$o")->getId();
	gateIdOut[1] = gate("photonicOutA$o")->getId();
	gateIdOut[2] = gate("photonicOutB$o")->getId();

	portType[0] = inout;
	portType[1] = inout;
	portType[2] = inout;

	PropagationLoss = par("PropagationLoss");
	PropagationDelay = par("PropagationDelay");

	// Latency, IL, and CT numbers based on Vlasov paper, 2009
	LatencyBarPath = 500 * PropagationDelay;
	LatencyCrossPath = 500 * PropagationDelay;

	InsertionLossBarState = .7 + 500 * PropagationLoss;
	InsertionLossCrossState = 500 * PropagationLoss;
	CrosstalkBarState = -21.3 - 500 * PropagationLoss;
	CrosstalkCrossState = 0;

	gateIdFromRouter = gate("fromRouter")->getId();


	std::cout<<"MZ Power Parameters not for external use, source from Ben Lee"<<endl;
	// Energy parameters are currently unpublished results from Ben Lee, not for public release
    StaticPowerDissipationOffState = 0;
    StaticPowerDissipationOnState = 11.2e-3;

    DynamicPowerDissipationOffToOn = 9e-12;
    DynamicPowerDissipationOnToOff = 9e-12;

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double MZSwitch1x2::GetLatency(int indexIn, int indexOut)
{
	if(indexOut == 0)
	{
		return LatencyBarPath;
	}
	else
	{
		return LatencyCrossPath;
	}
}


double MZSwitch1x2::GetPropagationLoss(int indexIn, int indexOut)
{
	if(currState == 0)
	{
		if(indexIn == 0 && indexOut == 1)
		{
			return InsertionLossBarState;
		}
		else
		{
			return -1*CrosstalkBarState;
		}
	}
	else //(currState == 1)
	{
		if(indexIn == 0 && indexOut == 1)
		{
			return -1*CrosstalkCrossState;
		}
		else
		{
			return InsertionLossCrossState;
		}
	}
}


double MZSwitch1x2::GetBendingLoss(int indexIn, int indexOut)
{
	return 0;
}



double MZSwitch1x2::GetPowerLevel(int state)
{
	if(state == 0)
	{
		return StaticPowerDissipationOffState;
	}
	else
	{
		return StaticPowerDissipationOnState;
	}
}
double MZSwitch1x2::GetEnergyDissipation(int stateBefore, int stateAfter)
{
	if(stateBefore == 0 && stateAfter == 1)
	{
		return DynamicPowerDissipationOffToOn;
	}
	else if(stateBefore == 1 && stateAfter == 0)
	{
		return DynamicPowerDissipationOnToOff;
	}
	return 0;
}

void MZSwitch1x2::SetRoutingTable()
{
	if(currState == 0)
	{
		routingTable[0] = 1;
		routingTable[1] = 0;
		routingTable[2] = -1;
	}
	else // if(currState == 1)
	{
		routingTable[0] = 2;
		routingTable[1] = -1;
		routingTable[2] = 0;
	}
}
