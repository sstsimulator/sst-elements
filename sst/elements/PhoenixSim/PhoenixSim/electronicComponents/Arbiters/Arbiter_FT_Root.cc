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

#include "Arbiter_FT_Root.h"


Arbiter_FT_Root::Arbiter_FT_Root() {

}

Arbiter_FT_Root::~Arbiter_FT_Root() {

}


bool Arbiter_FT_Root::isDimensionChange(int in, int out) {
	return false;
}

//returns the port num to go to
int Arbiter_FT_Root::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();

	return addr->id[level];

}

void Arbiter_FT_Root::PSEsetup(int inport, int outport, PSE_STATE st) {


	//N-up0, E-up1, S-dn0, W-dn1
	if (inport == FT_UP0) {
		if (outport == FT_UP1) {
			nextPSEState[4] = st;
		} else if (outport == FT_DN0) {
			//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = (InputGateState[FT_UP1] == gateActive
					&& InputGateDir[FT_UP1] == FT_DN1
					&& OutputGateState[FT_DN1] == gateActive
					&& OutputGateDir[FT_DN1] == FT_UP1) ? PSE_ON : st;
		} else if (outport == FT_DN1) {
			// do nothing
		}
	} else if (inport == FT_DN0) {
		if (outport == FT_DN1) {
			nextPSEState[5] = st;
		} else if (outport == FT_UP0) {
			nextPSEState[0] = (InputGateState[FT_DN1] == gateActive
					&& InputGateDir[FT_DN1] == FT_UP1
					&& OutputGateState[FT_UP1] == gateActive
					&& OutputGateDir[FT_UP1] == FT_DN1) ? PSE_ON : st;
		} else if (outport == FT_UP1) {
			// do nothing
		}
	} else if (inport == FT_UP1) {
		if (outport == FT_UP0) {
			nextPSEState[2] = st;
		} else if (outport == FT_DN1) {
			nextPSEState[1] = (InputGateState[FT_UP0] == gateActive
					&& InputGateDir[FT_UP0] == FT_DN0
					&& OutputGateState[FT_DN0] == gateActive
					&& OutputGateDir[FT_DN0] == FT_UP0) ? PSE_ON : st;
		} else if (outport == FT_DN0) {
			// do nothing
		}
	} else if (inport == FT_DN1) {
		if (outport == FT_DN0) {
			nextPSEState[3] = st;
		} else if (outport == FT_UP1) {
			nextPSEState[0] = (InputGateState[FT_DN0] == gateActive
					&& InputGateDir[FT_DN0] == FT_UP0
					&& OutputGateState[FT_UP0] == gateActive
					&& OutputGateDir[FT_UP0] == FT_DN0) ? PSE_ON : st;
		} else if (outport == FT_UP0) {
			// do nothing
		}
	} else {
		opp_error("unknown input direction specified");
	}
}
