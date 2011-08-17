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

#include "activedirectionalcoupler.h"

Define_Module(ActiveDirectionalCoupler);

ActiveDirectionalCoupler::ActiveDirectionalCoupler()
{
}

ActiveDirectionalCoupler::~ActiveDirectionalCoupler()
{
}

void ActiveDirectionalCoupler::Setup()
{

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = 0;

	// this routing is slightly inaccurate


	gateIdIn[0] = gate("photonicInA$i")->getId();
	gateIdIn[1] = gate("photonicInB$i")->getId();
	gateIdIn[2] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicInA$o")->getId();
	gateIdOut[1] = gate("photonicInB$o")->getId();
	gateIdOut[2] = gate("photonicOut$o")->getId();

	portType[0] = inout;
	portType[1] = inout;
	portType[2] = inout;

	PropagationLoss = par("PropagationLoss");
	PropagationDelay = par("PropagationDelay");

	CouplingLength = 50;

	// Latency, IL, and CT numbers based on Vlasov paper, 2009
	LatencyBarPath = CouplingLength * PropagationDelay;
	LatencyCrossPath = CouplingLength * PropagationDelay;

	InsertionLossBarState = CouplingLength * PropagationLoss;
	InsertionLossCrossState = CouplingLength * PropagationLoss;
	CrosstalkBarState = MAX_INSERTION_LOSS;
	CrosstalkCrossState = MAX_INSERTION_LOSS;

	gateIdFromRouter = gate("fromRouter")->getId();


	std::cout<<"MZ Power Parameters not for external use, source from Ben Lee"<<endl;
	// Energy parameters are currently unpublished results from Ben Lee, not for public release
    StaticPowerDissipationOffState = 0;
    StaticPowerDissipationOnState = 0;

    DynamicPowerDissipationOffToOn = 0;
    DynamicPowerDissipationOnToOff = 0;

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double ActiveDirectionalCoupler::GetLatency(int indexIn, int indexOut)
{
	if(indexIn == 0)
	{
		return LatencyBarPath;
	}
	else
	{
		return LatencyCrossPath;
	}
}


double ActiveDirectionalCoupler::GetPropagationLoss(int indexIn, int indexOut)
{
	if(currState == 0)
	{
		if(indexIn == 0 && indexOut == 2)
		{
			return InsertionLossBarState;
		}
		else
		{
			return CrosstalkBarState;
		}
	}
	else //(currState == 1)
	{
		if(indexIn == 0 && indexOut == 2)
		{
			return CrosstalkCrossState;
		}
		else
		{
			return InsertionLossCrossState;
		}
	}
}


double ActiveDirectionalCoupler::GetBendingLoss(int indexIn, int indexOut)
{
	return 0;
}



double ActiveDirectionalCoupler::GetPowerLevel(int state)
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
double ActiveDirectionalCoupler::GetEnergyDissipation(int stateBefore, int stateAfter)
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

void ActiveDirectionalCoupler::SetRoutingTable()
{
	if(currState == 0)
	{
		routingTable[0] = 2;
		routingTable[1] = -1;
		routingTable[2] = 0;
	}
	else // if(currState == 1)
	{
		routingTable[0] = -1;
		routingTable[1] = 2;
		routingTable[2] = 1;
	}
}


