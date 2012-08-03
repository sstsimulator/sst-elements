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

#include "pse1x2nx.h"

Define_Module(PSE1x2NX)
;

PSE1x2NX::PSE1x2NX() {
}

PSE1x2NX::~PSE1x2NX() {
}

void PSE1x2NX::Setup() {
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	currState = (int) PSE_DEFAULT_STATE;

	// this routing is slightly inaccurate


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

	vector<double> tempLatencyMatrix(numOfPorts, 0);
	latencyMatrix.resize(numOfPorts, tempLatencyMatrix);
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

double PSE1x2NX::GetLatency(int indexIn, int indexOut) {
	return latencyMatrix[indexIn][indexOut];
}

double PSE1x2NX::GetPropagationLoss(int indexIn, int indexOut,
		double wavelength) {
	return PropagationLoss * (5 + 5 + ringDiameter);
}

double PSE1x2NX::GetDropIntoRingLoss(int indexIn, int indexOut,
		double wavelength) {
	double IL = 0;

	if ((indexIn == 0 && indexOut == 3) || (indexIn == 3 && indexOut == 0)
			|| (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut
			== 1)) {
		IL += PassThroughRingLoss;

		if (currState == 0) {
			IL += RingOn_ER_1x2NX;
		}
	}
	return IL;
}

double PSE1x2NX::GetPassByRingLoss(int indexIn, int indexOut, double wavelength) {
	double IL = 0;
	if ((indexIn == 0 && indexOut == 1) || (indexIn == 1 && indexOut == 0)
			|| (indexIn == 2 && indexOut == 3) || (indexIn == 3 && indexOut
			== 2)) {
		IL += PassByRingLoss;

		if (currState == 1) {
			IL += RingOff_ER_1x2NX;
		}
	}
	return IL;
}

double PSE1x2NX::GetPowerLevel(int state) {
	switch (state) {
	//case 0:
	//	return 0;
	//	break;
	case PSE_POWER_STATE:
		return RingStaticDissipation;
		break;
	default:
		return 0;
	}

}

double PSE1x2NX::GetEnergyDissipation(int stateBefore, int stateAfter) {
	if (stateBefore != stateAfter) {
		switch (stateBefore) {
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

void PSE1x2NX::SetRoutingTable() {
	if (currState == PSE_OFF) {
		routingTable[0] = 1;
		routingTable[1] = 0;
		routingTable[2] = 3;
		routingTable[3] = 2;
	} else // if(currState == PSE_ON)
	{
		routingTable[0] = 3;
		routingTable[1] = 2;
		routingTable[2] = 1;
		routingTable[3] = 0;
	}
}

void PSE1x2NX::SetOffResonanceRoutingTable() {
	routingTableOffResonance[0] = 1;
	routingTableOffResonance[1] = 0;
	routingTableOffResonance[2] = 3;
	routingTableOffResonance[3] = 2;
}

