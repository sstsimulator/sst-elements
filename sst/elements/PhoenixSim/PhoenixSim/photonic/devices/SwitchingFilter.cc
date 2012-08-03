/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*                       (c) Assaf Shacham 2006-7			*
*			(c) Johnnie Chan  2008				*
*                                                                       *
* file: pse1x2.cc                                                       *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/


#include "SwitchingFilter.h"

Define_Module (SwitchingFilter);



SwitchingFilter::SwitchingFilter()
{
}

SwitchingFilter::~SwitchingFilter()
{
}

void SwitchingFilter::Setup()
{

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = (int)PSE_OFF;

	gateIdIn[0] = gate("thruIn$i")->getId();
	gateIdIn[1] = gate("In$i")->getId();
	gateIdIn[2] = gate("Out$i")->getId();
	gateIdIn[3] = gate("Drop$i")->getId();

	gateIdOut[0] = gate("thruIn$o")->getId();
	gateIdOut[1] = gate("In$o")->getId();
	gateIdOut[2] = gate("Out$o")->getId();
	gateIdOut[3] = gate("Drop$o")->getId();

	portType[0] = in;
	portType[1] = in;
	portType[2] = out;
	portType[3] = out;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");

	Crosstalk_Cross = par("Crosstalk_Cross");

	// might be incorrectly named? On = Drop port, Off = Through Port
	RingOn_ER_1x2 = par("RingOn_ER_1x2");
	RingOff_ER_1x2 = par("RingOff_ER_1x2");

	ThroughDelay_1x2 = par("ThroughDelay_1x2");
	DropDelay_1x2 = par("DropDelay_1x2");

	vector<double> tempLatencyMatrix(numOfPorts,0);
	latencyMatrix.resize(numOfPorts,tempLatencyMatrix);
	// eventually read from file

	latencyMatrix[0][0] = 5.0e-12;
	latencyMatrix[0][1] = 4.1e-12;
	latencyMatrix[0][2] = 1.0e-12;
	latencyMatrix[0][3] = 4.1e-12;
	latencyMatrix[1][0] = 4.1e-12;
	latencyMatrix[1][1] = 0.9e-12;
	latencyMatrix[1][2] = 0.9e-12;
	latencyMatrix[1][3] = 1.0e-12;
	latencyMatrix[2][0] = 1.0e-12;
	latencyMatrix[2][1] = 0.9e-12;
	latencyMatrix[2][2] = 0.9e-12;
	latencyMatrix[2][3] = 4.1e-12;
	latencyMatrix[3][0] = 4.1e-12;
	latencyMatrix[3][1] = 1.0e-12;
	latencyMatrix[3][2] = 4.1e-12;
	latencyMatrix[3][3] = 5.0e-12;

	vector<double> tempLossMatrix(numOfPorts,0);
	vector< vector<double> > tempLossMatrix2(numOfPorts,tempLossMatrix);
	lossMatrix.resize(numOfStates,tempLossMatrix2);


	// off state
	lossMatrix[0][0][0] = MAX_INSERTION_LOSS;
	lossMatrix[0][0][1] = MAX_INSERTION_LOSS;
	lossMatrix[0][0][2] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss;
	lossMatrix[0][0][3] = PropagationLoss * (5+5+.5*25*3.141592) + PassThroughRingLoss + RingOn_ER_1x2;
	lossMatrix[0][1][0] = MAX_INSERTION_LOSS;
	lossMatrix[0][1][1] = MAX_INSERTION_LOSS;
	lossMatrix[0][1][2] = MAX_INSERTION_LOSS;
	lossMatrix[0][1][3] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss;
	lossMatrix[0][2][0] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss;
	lossMatrix[0][2][1] = MAX_INSERTION_LOSS;
	lossMatrix[0][2][2] = MAX_INSERTION_LOSS;
	lossMatrix[0][2][3] = MAX_INSERTION_LOSS;
	lossMatrix[0][3][0] = PropagationLoss * (5+5+.5*25*3.141592) + PassThroughRingLoss + RingOn_ER_1x2;
	lossMatrix[0][3][1] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss;
	lossMatrix[0][3][2] = MAX_INSERTION_LOSS;
	lossMatrix[0][3][3] = MAX_INSERTION_LOSS;

	// on state
	lossMatrix[1][0][0] = MAX_INSERTION_LOSS;
	lossMatrix[1][0][1] = MAX_INSERTION_LOSS;
	lossMatrix[1][0][2] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss + RingOff_ER_1x2;
	lossMatrix[1][0][3] = PropagationLoss * (5+5+.5*25*3.141592) + PassThroughRingLoss;
	lossMatrix[1][1][0] = MAX_INSERTION_LOSS;
	lossMatrix[1][1][1] = MAX_INSERTION_LOSS;
	lossMatrix[1][1][2] = MAX_INSERTION_LOSS;
	lossMatrix[1][1][3] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss + RingOff_ER_1x2;
	lossMatrix[1][2][0] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss + RingOff_ER_1x2;
	lossMatrix[1][2][1] = MAX_INSERTION_LOSS;
	lossMatrix[1][2][2] = MAX_INSERTION_LOSS;
	lossMatrix[1][2][3] = MAX_INSERTION_LOSS;
	lossMatrix[1][3][0] = PropagationLoss * (5+5+.5*25*3.141592) + PassThroughRingLoss;
	lossMatrix[1][3][1] = PropagationLoss * 85 + CrossingLoss + PassByRingLoss + RingOff_ER_1x2;
	lossMatrix[1][3][2] = MAX_INSERTION_LOSS;
	lossMatrix[1][3][3] = MAX_INSERTION_LOSS;

	gateIdFromRouter = gate("cntrl")->getId();

	RingStaticDissipation = par("RingStaticDissipation");
	RingDynamicOffOn = par("RingDynamicOffOn");
	RingDynamicOnOff = par("RingDynamicOnOff");

	Power_Switch_Static = par("PSE1x2_pwr_switch_static");
	Power_Off_to_On = par("PSE1x2_pwr_switch_off_on");
	Power_On_to_Off = par("PSE1x2_pwr_switch_on_off");


	debug(getFullPath(), "INIT: done", UNIT_INIT);

}


void  SwitchingFilter::finish()
{
	// remaining cumulative state needs to be saved

	debug(getFullPath(), "FINISH: start", UNIT_FINISH);

	cumulativeTime[currState] += simTime() - lastSwitchTimestamp;


	opp_error("switching filter needs to update to new statistics model");

	double stat = SIMTIME_DBL(cumulativeTime[PSE_POWER_STATE]) * Power_Switch_Static;
	double swit =  switchCount[0][1] * Power_Off_to_On + switchCount[1][0] * Power_On_to_Off;



	debug(getFullPath(), "FINISH: done", UNIT_FINISH);
}


void SwitchingFilter::HandleControlMessage(ElementControlMessage *msg)
{

	opp_error("switching filter needs to update to new statistics model");
	double StaticEnergy = SIMTIME_DBL(simTime() - lastSwitchTimestamp) * Power_Switch_Static;
	double SwitchingEnergy;

	switch(msg->getState())
	{
		case PSE_OFF: 	SetState(0);
						SwitchingEnergy = Power_On_to_Off;
						break;
		case PSE_ON:	SetState(1);
						SwitchingEnergy = Power_Off_to_On;
						break;
		default:		opp_error("Error 012: Unknown State in SwitchingFilter");
	}
	delete msg;


	debug(getFullPath(), "energy ", StaticEnergy + SwitchingEnergy, UNIT_POWER);
}

double SwitchingFilter::GetLatency(int indexIn, int indexOut)
{
	return latencyMatrix[indexIn][indexOut];
}

double SwitchingFilter::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	// Does not count the ring loss since included with drop #s
	double IL = 0;
	if((indexIn == 0 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 0))
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

double SwitchingFilter::GetCrossingLoss(int indexIn, int indexOut, double wavelength)
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

double SwitchingFilter::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if((indexIn == 0 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 0))
	{
		IL += PassThroughRingLoss;

		if(currState == 0)
		{
			IL += RingOn_ER_1x2;
		}
	}
	return IL;
}

double SwitchingFilter::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if((indexIn == 1 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 1)
			|| (indexIn == 0 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 0))
	{
		IL += PassByRingLoss;

		if(currState == 1)
		{
			IL += RingOff_ER_1x2;
		}
	}
	return IL;
}

double SwitchingFilter::GetCrossTalk(int indexIn, int indexOut, double wavelength)
{
	return lossMatrix[currState][indexIn][indexOut];
}

double SwitchingFilter::GetPowerLevel(int state)
{
	switch(state)
	{
	case 0:
		return 0;
		break;
	case 1:
		return RingStaticDissipation;
		break;
	default:
		return 0;
	};


}

double SwitchingFilter::GetEnergyDissipation(int stateBefore, int stateAfter)
{
	if(stateBefore != stateAfter)
	{
		switch(stateBefore)
		{
		case 0:
			return RingDynamicOffOn;
			break;
		case 1:
			return RingDynamicOnOff;
			break;
		default:
			return 0;
		};
	}
	return 0;
}

void SwitchingFilter::SetRoutingTable()
{
	if(currState == PSE_OFF)
	{
		routingTable[0] = 2;
		routingTable[1] = 3;
		routingTable[2] = 0;
		routingTable[3] = 1;
	}
	else // if(currState == PSE_ON)
	{
		routingTable[0] = 3;
		routingTable[1] = -1;
		routingTable[2] = -1;
		routingTable[3] = 0;
	}
}

void SwitchingFilter::SetOffResonanceRoutingTable()
{
	routingTableOffResonance[0] = 2;
	routingTableOffResonance[1] = 3;
	routingTableOffResonance[2] = 0;
	routingTableOffResonance[3] = 1;
}
