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

#include "Arbiter_SR_Top.h"

Arbiter_SR_Top::Arbiter_SR_Top() {

}

Arbiter_SR_Top::~Arbiter_SR_Top() {
	// TODO Auto-generated destructor stub
}

void Arbiter_SR_Top::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	Arbiter_SR::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	int myId = myAddr->id[level];

	if (myId == 4) {
		routeMiddle[0] = 1;
		routeMiddle[1] = 0;
		routeMiddle[2] = 2;
		routeMiddle[3] = 7;
	} else if (myId == 5) {
		routeMiddle[0] = 1;
		routeMiddle[1] = 0;
		routeMiddle[2] = 2;
		routeMiddle[3] = 4;
	} else if (myId == 6) {
		routeMiddle[0] = 6;
		routeMiddle[1] = 3;
		routeMiddle[2] =5;
		routeMiddle[3] = 4;
	} else if (myId == 7) {
		routeMiddle[0] = 6;
		routeMiddle[1] = 3;
		routeMiddle[2] = 5;
		routeMiddle[3] = 7;
	}

}

//returns the port num to go to
int Arbiter_SR_Top::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	return routeToPort[routeMiddle[destId]];

}

