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

#include "Arbiter_GWC.h"

Arbiter_GWC::Arbiter_GWC() {

}

Arbiter_GWC::~Arbiter_GWC() {

}

//returns the port num to go to
int Arbiter_GWC::route(ArbiterRequestMsg* rmsg) {
	// + isConc*(((destId % (numX*2)) % 2) + 2*((destId / (numY*2)) % 2));

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	if (destId == myAddr->id[level]) {
		return 0;
	} else if (destId != myAddr->id[level]) {
		return 1;
	} else {
		opp_error("ERROR: something screwy in GatewayControl routing logic");
	}

}

int Arbiter_GWC::getUpPort(ArbiterRequestMsg* rmsg, int lev) {

	return 1;
}

int Arbiter_GWC::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	if (lev == translator->convertLevel("DRAM") && destId == 1) {
		return 2;
	}

	return 0;
}

void Arbiter_GWC::PSEsetup(int inport, int outport, PSE_STATE st) {

	debug(name, "PSE setup, outport ", outport, UNIT_PATH_SETUP);
	debug(name, " ... inport ", inport, UNIT_PATH_SETUP);
	debug(name, " ... st ", st, UNIT_PATH_SETUP);

	//debug(name, " ... nextPSE[0] ", nextPSEState[0], UNIT_PATH_SETUP);
	//debug(name, " ... nextPSE[1] ", nextPSEState[1], UNIT_PATH_SETUP);

	//debug(name, " ... numPorts ", numPorts, UNIT_PATH_SETUP);

	/*if (outport == numPorts - 1) {
	 if (inport >= GWC_Node && inport <= numPorts - 3) {
	 nextPSEState[PSE_output] = st;
	 } else if (inport == numPorts - 2) {
	 nextPSEState[PSE_output] = PSE_OFF;
	 nextPSEState[PSE_input] = st;
	 }
	 } else if (outport >= GWC_Node && outport <= numPorts - 3) {

	 if (inport == numPorts - 1) {
	 nextPSEState[PSE_input] = st;
	 } else if (inport == numPorts - 2) {
	 nextPSEState[PSE_input] = PSE_OFF;
	 }
	 } else if (outport == numPorts - 2) {
	 if (inport == numPorts - 1) {
	 nextPSEState[PSE_output] = st;
	 nextPSEState[PSE_input] = PSE_OFF;
	 } else if (inport >= GWC_Node && inport <= numPorts - 3) {
	 nextPSEState[PSE_output] = PSE_OFF;
	 }
	 }*/

	if (inport == 0) {
		if (outport == 1) {
			nextPSEState[4] = st;
		} else if (outport == 2) {
			//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = st;
		} else {
			opp_error("GWC: outport requested doesn't go to another 4x4 output");
		}
	} else if (inport == 2) {
		if (outport == 0) {
			nextPSEState[0] = st;

		}
	} else if (inport == 1) {

		if (outport == 0) {
			nextPSEState[2] = st;
		}
	} else {
		opp_error("unknown input direction specified");
	}

}

