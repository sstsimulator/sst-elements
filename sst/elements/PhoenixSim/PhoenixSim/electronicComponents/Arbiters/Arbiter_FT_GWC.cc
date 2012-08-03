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

#include "Arbiter_FT_GWC.h"

Arbiter_FT_GWC::Arbiter_FT_GWC() {

}

Arbiter_FT_GWC::~Arbiter_FT_GWC() {

}

//returns the port num to go to
int Arbiter_FT_GWC::route(ArbiterRequestMsg* rmsg) {

	getUpPort(rmsg, level);

}

int Arbiter_FT_GWC::getUpPort(ArbiterRequestMsg* rmsg, int lev) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();

	if (addr->id[0] == myAddr->id[0]) {
		return 1; //it's in my grid
	} else {
		int diff = addr->id[0] - myAddr->id[0];

		if (abs(diff) == 1) {
			if (diff > 0) {
				return 2;
			} else {
				return 3;
			}
		} else if (diff == 3) {
			return 3;
		} else if (diff == -3) {
			return 2;
		} else {
			return (intrand(2) >= 1 ? 3 : 2);
		}

	}

}

int Arbiter_FT_GWC::getDownPort(ArbiterRequestMsg* rmsg, int lev) {


	return 0;
}

void Arbiter_FT_GWC::PSEsetup(int inport, int outport, PSE_STATE st) {

	debug(name, "PSE setup, outport ", outport, UNIT_PATH_SETUP);
	debug(name, " ... inport ", inport, UNIT_PATH_SETUP);
	debug(name, " ... st ", st, UNIT_PATH_SETUP);

	if (inport == SW_N) {
		if (outport == SW_E) {
			nextPSEState[4] = st;
		} else if (outport == SW_S) {
			//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = (InputGateState[SW_E] == gateActive
					&& InputGateDir[SW_E] == SW_W && OutputGateState[SW_W]
					== gateActive && OutputGateDir[SW_W] == SW_E) ? PSE_ON : st;
		} else if (outport == SW_W) {
			// do nothing
		}
	} else if (inport == SW_S) {
		if (outport == SW_W) {
			nextPSEState[5] = st;
		} else if (outport == SW_N) {
			nextPSEState[0] = (InputGateState[SW_W] == gateActive
					&& InputGateDir[SW_W] == SW_E && OutputGateState[SW_E]
					== gateActive && OutputGateDir[SW_E] == SW_W) ? PSE_ON : st;
		} else if (outport == SW_E) {
			// do nothing
		}
	} else if (inport == SW_E) {
		if (outport == SW_N) {
			nextPSEState[2] = st;
		} else if (outport == SW_W) {
			nextPSEState[1] = (InputGateState[SW_N] == gateActive
					&& InputGateDir[SW_N] == SW_S && OutputGateState[SW_S]
					== gateActive && OutputGateDir[SW_S] == SW_N) ? PSE_ON : st;
		} else if (outport == SW_S) {
			// do nothing
		}
	} else if (inport == SW_W) {
		if (outport == SW_S) {
			nextPSEState[3] = st;
		} else if (outport == SW_E) {
			nextPSEState[0] = (InputGateState[SW_S] == gateActive
					&& InputGateDir[SW_S] == SW_N && OutputGateState[SW_N]
					== gateActive && OutputGateDir[SW_N] == SW_S) ? PSE_ON : st;
		} else if (outport == SW_N) {
			// do nothing
		}
	} else {
		opp_error("unknown input direction specified");
	}

}

