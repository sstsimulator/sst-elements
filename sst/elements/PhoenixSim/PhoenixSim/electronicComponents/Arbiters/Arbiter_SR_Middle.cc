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

#include "Arbiter_SR_Middle.h"

Arbiter_SR_Middle::Arbiter_SR_Middle() {

}

Arbiter_SR_Middle::~Arbiter_SR_Middle() {
	// TODO Auto-generated destructor stub
}

void Arbiter_SR_Middle::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	Arbiter_SR::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	int myId = myAddr->id[level];
	myUp = myAddr->id[level - 1];

	if (myId == 4) {
		routeMiddle[0] = (myUp == 2) ? SW_S : SW_E;
		routeMiddle[1] = (myUp == 3) ? SW_E : SW_N;
		routeMiddle[2] = (myUp == 0) ? SW_E : SW_S;
		routeMiddle[3] = (myUp == 2) ? SW_S : SW_W;

	} else if (myId == 5) {
		routeMiddle[0] = (myUp == 2) ? SW_W : SW_N;
		routeMiddle[1] = (myUp == 3) ? SW_S : SW_W;
		routeMiddle[2] = (myUp == 3) ? SW_S : SW_E;
		routeMiddle[3] = (myUp == 1) ? SW_W : SW_S;

	} else if (myId == 6) {
		routeMiddle[0] = (myUp == 1) ? SW_N : SW_W;
		routeMiddle[1] = (myUp == 0) ? SW_N : SW_E;
		routeMiddle[2] = (myUp == 3) ? SW_W : SW_N;
		routeMiddle[3] = (myUp == 1) ? SW_W : SW_S;

	} else if (myId == 7) {
		routeMiddle[0] = (myUp == 1) ? SW_N : SW_W;
		routeMiddle[1] = (myUp == 3) ? SW_E : SW_N;
		routeMiddle[2] = (myUp == 0) ? SW_E : SW_S;
		routeMiddle[3] = (myUp == 1) ? SW_N : SW_E;
	}

	bool exOff = (level == translator->convertLevel("NET2")) && (numX > 8);
	int myTop = -1;

	if(exOff){
		myTop = myAddr->id[translator->convertLevel("NET4")];
	}

	if(myUp == 0){
		upPort = (myId == 4 || myId == 7) ? (myTop == 1 ? 3 : 5) : (myTop == 2 ? 5 : 3);
	} else if(myUp == 1){
		upPort = (myId == 4 || myId == 7) ? (myTop == 3 ? 4 : 6) : (myTop == 0 ? 6 : 4);
	} else if(myUp == 2){
		upPort = (myId == 4 || myId == 7) ? (myTop == 0 ? 1 : 7) : (myTop == 3 ? 7 : 1);
	}else if(myUp == 3){
		upPort = (myId == 4 || myId == 7) ? (myTop == 2 ? 2 : 0) : (myTop == 1 ? 0 : 2);
	}


	std::cout << translator->untranslate_str(myAddr) << "- myId: " << myId << ", myUp: " << myUp << ", upPort: " << upPort << endl;

}

//returns the port num to go to
int Arbiter_SR_Middle::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	return routeMiddle[destId];

}

int Arbiter_SR_Middle::getUpPort(ArbiterRequestMsg* rmsg, int lev) {

	NetworkAddress* __attribute__ ((unused)) addr = (NetworkAddress*) rmsg->getDest();
//	int destId = addr->id[level];

	//if (adjacent(destId, myUp)) {

	//} else {
		return routeToPort[upPort];
	//}

}
