#include "pse2x2.h"

Define_Module (PSE2x2)
;

PSE2x2::PSE2x2() {
}

PSE2x2::~PSE2x2() {
}

void PSE2x2::Setup() {
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = (int) PSE_DEFAULT_STATE;

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

	Crosstalk_Cross = par("Crosstalk_Cross");

	// might be incorrectly named? On = Drop port, Off = Through Port
	RingOn_ER_2x2 = par("RingOn_ER_2x2");
	RingOff_ER_2x2 = par("RingOff_ER_2x2");

	CrossDelay_2x2 = par("CrossDelay_2x2");
	BarDelay_2x2 = par("BarDelay_2x2");

	vector<double> tempLatencyMatrix(numOfPorts, 0);
	latencyMatrix.resize(numOfPorts, tempLatencyMatrix);
	// eventually read from file

	latencyMatrix[0][0] = BarDelay_2x2;
	latencyMatrix[0][1] = CrossDelay_2x2;
	latencyMatrix[0][2] = BarDelay_2x2;
	latencyMatrix[0][3] = CrossDelay_2x2;
	latencyMatrix[1][0] = CrossDelay_2x2;
	latencyMatrix[1][1] = BarDelay_2x2;
	latencyMatrix[1][2] = CrossDelay_2x2;
	latencyMatrix[1][3] = BarDelay_2x2;
	latencyMatrix[2][0] = BarDelay_2x2;
	latencyMatrix[2][1] = CrossDelay_2x2;
	latencyMatrix[2][2] = BarDelay_2x2;
	latencyMatrix[2][3] = CrossDelay_2x2;
	latencyMatrix[3][0] = CrossDelay_2x2;
	latencyMatrix[3][1] = BarDelay_2x2;
	latencyMatrix[3][2] = CrossDelay_2x2;
	latencyMatrix[3][3] = BarDelay_2x2;

	gateIdFromRouter = gate("fromRouter")->getId();

	RingStaticDissipation = par("RingStaticDissipation");
	RingDynamicOffOn = par("RingDynamicOffOn");
	RingDynamicOnOff = par("RingDynamicOnOff");

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double PSE2x2::GetLatency(int indexIn, int indexOut) {
	return latencyMatrix[indexIn][indexOut];
}

double PSE2x2::GetPropagationLoss(int indexIn, int indexOut, double wavelength) {
	// Does not count the ring loss since included with drop #s
	if ((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut
			== 1)) {
		return PropagationLoss * (5 + 5 + ringDiameter);
	} else {
		return PropagationLoss * (5 + 5 + ringDiameter + ringDiameter);
	}
}

double PSE2x2::GetCrossingLoss(int indexIn, int indexOut, double wavelength) {
	if (indexIn != indexOut) {
		if ((indexIn + 2) % 4 == indexOut) {
			return CrossingLoss;
		} else if ((indexIn == 0 && indexOut == 1) || (indexIn == 1 && indexOut
				== 0) || (indexIn == 2 && indexOut == 3) || (indexIn == 3
				&& indexOut == 2)) {
			return Crosstalk_Cross;
		} else {
			return 0;
		}
	} else {
		return MAX_INSERTION_LOSS;
	}
}

double PSE2x2::GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength) {
	double IL = 0;
	if ((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut
			== 1)) {
		IL += PassThroughRingLoss;

		if (currState == 0) {
			IL += RingOn_ER_2x2;
		}
	}
	return IL;
}

double PSE2x2::GetPassByRingLoss(int indexIn, int indexOut, double wavelength) {
	double IL = 0;
	if (!((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut
			== 1))) {
		IL += PassByRingLoss * 2;

		if (currState == 1) {
			IL += RingOff_ER_2x2;
		}
	}
	return IL;
}

double PSE2x2::GetPowerLevel(int state) {
	switch (state) {
	//case 0:
	//	return 0;
	//	break;
	case PSE_POWER_STATE:
		return RingStaticDissipation * 2;
		break;
	default:
		return 0;
	}

}

double PSE2x2::GetEnergyDissipation(int stateBefore, int stateAfter) {
	if (stateBefore != stateAfter) {
		switch (stateBefore) {
		case 0:
			return RingDynamicOffOn * 2;
			break;
		case 1:
			return RingDynamicOnOff * 2;
			break;
		default:
			return 0;
		};
	}
	return 0;
}

void PSE2x2::SetRoutingTable() {
	if (currState == PSE_OFF) {
		routingTable[0] = 2;
		routingTable[1] = 3;
		routingTable[2] = 0;
		routingTable[3] = 1;
	} else // if(currState == PSE_ON)
	{
		routingTable[0] = 3;
		routingTable[1] = 2;
		routingTable[2] = 1;
		routingTable[3] = 0;
	}
}

void PSE2x2::SetOffResonanceRoutingTable() {
	routingTableOffResonance[0] = 2;
	routingTableOffResonance[1] = 3;
	routingTableOffResonance[2] = 0;
	routingTableOffResonance[3] = 1;
}
