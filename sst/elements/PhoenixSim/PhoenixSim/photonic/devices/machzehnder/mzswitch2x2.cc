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

#include "mzswitch2x2.h"

Define_Module(MZSwitch2x2);

MZSwitch2x2::MZSwitch2x2()
{
}

MZSwitch2x2::~MZSwitch2x2()
{
}

void MZSwitch2x2::Setup()
{

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = 0;

	// this routing is slightly inaccurate


	gateIdIn[0] = gate("photonicInA$i")->getId();
	gateIdIn[1] = gate("photonicInB$i")->getId();
	gateIdIn[2] = gate("photonicOutA$i")->getId();
	gateIdIn[3] = gate("photonicOutB$i")->getId();

	gateIdOut[0] = gate("photonicInA$o")->getId();
	gateIdOut[1] = gate("photonicInB$o")->getId();
	gateIdOut[2] = gate("photonicOutA$o")->getId();
	gateIdOut[3] = gate("photonicOutB$o")->getId();

	portType[0] = inout;
	portType[1] = inout;
	portType[2] = inout;
	portType[3] = inout;

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

double MZSwitch2x2::GetLatency(int indexIn, int indexOut)
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


double MZSwitch2x2::GetPropagationLoss(int indexIn, int indexOut)
{
	if(currState == 0)
	{
		if((indexIn == 0 && indexOut == 3) || (indexIn == 1 && indexOut == 2) ||
				(indexIn == 3 && indexOut == 0) || (indexIn == 2 && indexOut == 1))
		{
			return InsertionLossCrossState;
		}
		else
		{
			return -1*CrosstalkCrossState;
		}
	}
	else //(currState == 1)
	{
		if((indexIn == 0 && indexOut == 3) || (indexIn == 1 && indexOut == 2) ||
				(indexIn == 3 && indexOut == 0) || (indexIn == 2 && indexOut == 1))
		{
			return -1*CrosstalkBarState;
		}
		else
		{
			return InsertionLossBarState;
		}
	}
}


double MZSwitch2x2::GetBendingLoss(int indexIn, int indexOut)
{
	return 0;
}



double MZSwitch2x2::GetPowerLevel(int state)
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
double MZSwitch2x2::GetEnergyDissipation(int stateBefore, int stateAfter)
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

void MZSwitch2x2::SetRoutingTable()
{
	if(currState == 0)
	{
		routingTable[0] = 3;
		routingTable[1] = 2;
		routingTable[2] = 1;
		routingTable[3] = 0;
	}
	else // if(currState == 1)
	{
		routingTable[0] = 2;
		routingTable[1] = 3;
		routingTable[2] = 0;
		routingTable[3] = 1;
	}
}
