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

#include "ElectronicArbiter.h"

ElectronicArbiter::ElectronicArbiter() {
	// TODO Auto-generated constructor stub

}

ElectronicArbiter::~ElectronicArbiter() {
	// TODO Auto-generated destructor stub
}

int ElectronicArbiter::pathStatus(ArbiterRequestMsg* rmsg, int outport) {
	bool dim = isDimensionChange(rmsg->getPortIn(), outport);
	int threshold = rmsg->getSize() * (1 + dim);
	int vc = rmsg->getNewVC();

	//for (int i = 0; i < numVC; i++) {
		if (!blockedOut[outport] && !blockedIn[rmsg->getPortIn()]
				&& (*credits[outport])[vc] >= threshold) {
		//	rmsg->setNewVC(i);
			return GO_OK;
		}
	//}

	//debug(name, "No_GO: blockedOut[..]", blockedOut[outport], UNIT_ROUTER);
	//debug(name, ".. blockedIn[..]", blockedIn[rmsg->getPortIn()], UNIT_ROUTER);
	//debug(name, ".. credits[..]", (*credits[outport])[vc], UNIT_ROUTER);

	return NO_GO;
}

bool ElectronicArbiter::isDimensionChange(int inport, int outport) {
	//this is for a mesh, or torus
	if (inport < 4) {
		if (outport < 4) {
			return inport % 2 != outport % 2;
		} else
			return false;
	} else if (outport < 4) {
		return true;
	} else
		//two NICs, which is fine
		return false;
}

