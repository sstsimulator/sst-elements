#include "pse2x2r3.h"

Define_Module (PSE2x2R3);



PSE2x2R3::PSE2x2R3()
{
}

PSE2x2R3::~PSE2x2R3()
{
}

void PSE2x2R3::Setup()
{
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = (int)PSE_OFF;

	gateIdIn[0] = gate("photonicHorizIn$i")->getId();
	gateIdIn[1] = gate("photonicVertIn$i")->getId();
	gateIdIn[2] = gate("photonicHorizOut$i")->getId();
	gateIdIn[3] = gate("photonicVertOut$i")->getId();

	gateIdOut[0] = gate("photonicHorizIn$o")->getId();
	gateIdOut[1] = gate("photonicVertIn$o")->getId();
	gateIdOut[2] = gate("photonicHorizOut$o")->getId();
	gateIdOut[3] = gate("photonicVertOut$o")->getId();

	portType[0] = in;
	portType[1] = in;
	portType[2] = out;
	portType[3] = out;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");
	BendingLoss = par("BendingLoss");

	Crosstalk_Cross = par("Crosstalk_Cross");

	// might be incorrectly named? On = Drop port, Off = Through Port
	RingOn_ER_2x2 = par("RingOn_ER_2x2");
	RingOff_ER_2x2 = par("RingOff_ER_2x2");

	CrossDelay_2x2 = par("CrossDelay_2x2");
	BarDelay_2x2 = par("BarDelay_2x2");


	vector<double> tempLatencyMatrix(numOfPorts,0);
	latencyMatrix.resize(numOfPorts,tempLatencyMatrix);
	// eventually read from file

	latencyMatrix[0][0] = CrossDelay_2x2;
	latencyMatrix[0][1] = BarDelay_2x2;
	latencyMatrix[0][2] = CrossDelay_2x2;
	latencyMatrix[0][3] = BarDelay_2x2;
	latencyMatrix[1][0] = BarDelay_2x2;
	latencyMatrix[1][1] = CrossDelay_2x2;
	latencyMatrix[1][2] = BarDelay_2x2;
	latencyMatrix[1][3] = CrossDelay_2x2;
	latencyMatrix[2][0] = CrossDelay_2x2;
	latencyMatrix[2][1] = BarDelay_2x2;
	latencyMatrix[2][2] = CrossDelay_2x2;
	latencyMatrix[2][3] = BarDelay_2x2;
	latencyMatrix[3][0] = BarDelay_2x2;
	latencyMatrix[3][1] = CrossDelay_2x2;
	latencyMatrix[3][2] = BarDelay_2x2;
	latencyMatrix[3][3] = CrossDelay_2x2;


	gateIdFromRouter = gate("fromRouter")->getId();

	RingStaticDissipation = par("RingStaticDissipation");
	RingDynamicOffOn = par("RingDynamicOffOn");
	RingDynamicOnOff = par("RingDynamicOnOff");


	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double PSE2x2R3::GetLatency(int indexIn, int indexOut)
{
	return latencyMatrix[indexIn][indexOut];
}

double PSE2x2R3::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	// Does not count the ring loss since included with drop #s
	double IL = 0;
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);

	double ringDiameter = this->ringDiameter[0];

	if(indexIn == 0 || indexOut == 0)
	{
		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += 5 + ringDiameter/2 + ringDiameter*(ringSet);
		}
		else
		{
			IL += 5 + (ringDiameter+5)*numOfRingSets;
		}
	}

	if(indexIn == 1 || indexOut == 1)
	{
		IL += 5 + 5 + 5 + ringDiameter + ringDiameter;

		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += 5 + ringDiameter/2 + ringDiameter*(ringSet);
		}
		else
		{
			IL += 5 + (ringDiameter+5)*numOfRingSets + ringDiameter;
		}
	}

	if(indexIn == 2 || indexOut == 2)
	{
		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += 5 + ringDiameter/2 + ringDiameter*(ringSet);
		}
		else
		{
			IL += 5 + (ringDiameter+5)*numOfRingSets;
		}
	}

	if(indexIn == 3 || indexOut == 3)
	{
		IL += 5 + 5 + 5 + ringDiameter + ringDiameter;

		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += 5 + ringDiameter/2 + ringDiameter*(ringSet);
		}
		else
		{
			IL += 5 + (ringDiameter+5)*numOfRingSets + ringDiameter;
		}
	}
	return PropagationLoss*IL;
}

double PSE2x2R3::GetCrossingLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		if((indexIn+2)%4 == indexOut)
		{
			return CrossingLoss;
		}
		else if((indexIn == 0 && indexOut == 1)
			|| (indexIn == 1 && indexOut == 0)
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

double PSE2x2R3::GetBendingLoss(int indexIn, int indexOut, double wavelength)
{
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);

	int bends = 0;
	if(indexIn == 3 || indexOut == 3)
	{
		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			bends += 3;
		}
		else
		{
			bends += 4;
		}
	}
	if(indexIn == 1 || indexOut == 1)
	{
		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			bends += 3;
		}
		else
		{
			bends += 4;
		}
	}
	return BendingLoss * bends;
}

double PSE2x2R3::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);
	if((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0) ||
			(indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut == 1) )
	{
		IL += PassThroughRingLoss;

		if(resonant && GetRingSetState(currState,ringSet) == 0)
		{
			IL += RingOn_ER_2x2;
		}
	}
	return IL;
}

double PSE2x2R3::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	int ringSet;
	bool resonant = IsInResonance(wavelength,ringSet);

	if(!((indexIn == 0 && indexOut == 3)
			|| (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2)
			|| (indexIn == 2 && indexOut == 1)))
	{
		IL += PassByRingLoss*numOfRingSets*2;

		if(resonant && GetRingSetState(currState,ringSet) == 1)
		{
			IL += RingOff_ER_2x2;
		}
	}

	else if(((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)
				|| (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut == 1)) && resonant)
	{
		IL += PassByRingLoss*2*ringSet;
	}

	return IL;
}


double PSE2x2R3::GetPowerLevel(int state)
{
	switch(state)
	{
	case 0:
		return 0;
		break;
	case 1:
		return RingStaticDissipation * 2;
		break;
	case 2:
		return RingStaticDissipation * 2;
			break;
	case 3:
		return RingStaticDissipation * 4;
			break;

	default:
		return 0;
	};
}

double PSE2x2R3::GetEnergyDissipation(int stateBefore, int stateAfter)
{
	double energyTotal = 0;

	if(stateBefore != stateAfter)
	{
		if(stateBefore&1 == 0 && stateAfter&1 == 1)
		{
			energyTotal += RingDynamicOffOn*2;
		}

		if(stateBefore&1 == 1 && stateAfter&1 == 0)
		{
			energyTotal += RingDynamicOffOn*2;
		}

		if((stateBefore>>1)&1 == 0 && (stateAfter>>1)&1 == 1)
		{
			energyTotal += RingDynamicOffOn*2;
		}

		if((stateBefore>>1)&1 == 1 && (stateAfter>>1)&1 == 0)
		{
			energyTotal += RingDynamicOffOn*2;
		}

	}
	return energyTotal;
}

void PSE2x2R3::SetRoutingTable()
{
	// off resonance case
	routingTable[0] = 2;
	routingTable[1] = 3;
	routingTable[2] = 0;
	routingTable[3] = 1;

}


int PSE2x2R3::GetMultiRingRoutingTable(int index, int ringSet)
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
