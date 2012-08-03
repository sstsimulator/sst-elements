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

#include "Arbiter_SR.h"

Arbiter_SR::Arbiter_SR() {

}

Arbiter_SR::~Arbiter_SR() {
	// TODO Auto-generated destructor stub
}

void Arbiter_SR::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	int myId = myAddr->id[level];

	if (myId % 4 == 0) {

		routeToPort[0] = SW_N;
		routeToPort[1] = SW_E;
		routeToPort[2] = SW_E;
		routeToPort[3] = SW_E;
		routeToPort[4] = SW_S;
		routeToPort[5] = SW_S;
		routeToPort[6] = SW_S;
		routeToPort[7] = SW_W;

	} else if (myId % 4 == 1) {

		routeToPort[0] = SW_W;
		routeToPort[1] = SW_N;
		routeToPort[2] = SW_E;
		routeToPort[3] = SW_S;
		routeToPort[4] = SW_S;
		routeToPort[5] = SW_S;
		routeToPort[6] = SW_W;
		routeToPort[7] = SW_W;

	} else if (myId % 4 == 2) {

		routeToPort[0] = SW_N;
		routeToPort[1] = SW_N;
		routeToPort[2] = SW_N;
		routeToPort[3] = SW_E;
		routeToPort[4] = SW_S;
		routeToPort[5] = SW_W;
		routeToPort[6] = SW_W;
		routeToPort[7] = SW_W;

	} else if (myId % 4 == 3) {

		routeToPort[0] = SW_N;
		routeToPort[1] = SW_N;
		routeToPort[2] = SW_E;
		routeToPort[3] = SW_E;
		routeToPort[4] = SW_E;
		routeToPort[5] = SW_S;
		routeToPort[6] = SW_W;
		routeToPort[7] = SW_N;
	}
}

bool Arbiter_SR::adjacent(int mine, int his) {
	if (abs(mine - his) != 2) {
		return true;
	} else {
		return false;
	}
}
