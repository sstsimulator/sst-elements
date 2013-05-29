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

#include "Arbiter_SR_Quad.h"

Arbiter_SR_Quad::Arbiter_SR_Quad() {

}

Arbiter_SR_Quad::~Arbiter_SR_Quad() {
	// TODO Auto-generated destructor stub
}

void Arbiter_SR_Quad::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	Arbiter_SR::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	myEight = myAddr->id[translator->convertLevel("NET4")];
	myGrid = myAddr->id[translator->convertLevel("NET3")];
	myQuad = myAddr->id[translator->convertLevel("NET2")];
	myUnit = myAddr->id[translator->convertLevel("NET1")];

	map<int, int> gw;

	if (myQuad % 2 == 0) {
		gw[0] = 7;
		gw[1] = 1;
		gw[2] = 3;
		gw[3] = 5;
	} else {
		gw[0] = 0;
		gw[1] = 2;
		gw[2] = 4;
		gw[3] = 6;
	}

	int middlePort = 0;

	//myQuad, myUnit
	if (myQuad == 0) {

		quadExpressPort[1] = 0;
		quadExpressPort[3] = 3;

		middlePort = (myUnit < 2 && myGrid != 2) ? 1 : ((myGrid != 1) ? 2 : 1);
	} else if (myQuad == 1) {

		quadExpressPort[0] = 1;
		quadExpressPort[2] = 2;

		middlePort = (myUnit < 2 && myGrid != 3) ? 0 : ((myGrid != 0 && numX
				> 4) ? 3 : 0);
	} else if (myQuad == 2) {

		quadExpressPort[1] = 1;
		quadExpressPort[3] = 2;

		middlePort = (myUnit < 2 && myGrid != 3) ? 0 : ((myGrid != 0 && numX
				> 4) ? 3 : 0);
	} else if (myQuad == 3) {

		quadExpressPort[0] = 0;
		quadExpressPort[2] = 3;

		middlePort = (myUnit < 2 && myGrid != 2) ? 1 : ((myGrid != 1) ? 2 : 1);
	}

	if (myGrid == 0) {
		gridExpressQuad[1] = 1;
		gridExpressQuad[3] = 2;

		gridExpressPort = 3;
	} else if (myGrid == 1) {
		gridExpressQuad[0] = 0;
		gridExpressQuad[2] = 3;

		gridExpressPort = 2;
	} else if (myGrid == 2) {
		gridExpressQuad[1] = 0;
		gridExpressQuad[3] = 3;

		gridExpressPort = 1;
	} else if (myGrid == 3) {
		gridExpressQuad[0] = 1;
		gridExpressQuad[2] = 2;

		gridExpressPort = 0;
	}

	if (myUnit == 0) {
		routeQuad[0] = (myQuad % 2 == 0) ? SW_N : SW_W; //used to get to the up port
		routeQuad[1] = SW_E;
		routeQuad[2] = SW_E;
		routeQuad[3] = SW_S;
	} else if (myUnit == 1) {
		routeQuad[0] = SW_W;
		routeQuad[1] = (myQuad % 2 == 0) ? SW_E : SW_N;
		routeQuad[2] = SW_S;
		routeQuad[3] = SW_S;
	} else if (myUnit == 2) {
		routeQuad[0] = SW_W;
		routeQuad[1] = SW_N;
		routeQuad[2] = (myQuad % 2 == 0) ? SW_S : SW_E;
		routeQuad[3] = SW_W;
	} else if (myUnit == 3) {
		routeQuad[0] = SW_N;
		routeQuad[1] = SW_E;
		routeQuad[2] = SW_E;
		routeQuad[3] = (myQuad % 2 == 0) ? SW_W : SW_S;
	}

	gatewayAccess = routeToPort[gw[myUnit]];
	toMiddle = routeQuad[middlePort];

}

//returns the port num to go to
int Arbiter_SR_Quad::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	debug(name, "start route", UNIT_ROUTER);

	return routeQuad[destId];

}

int Arbiter_SR_Quad::getUpPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling upPort.. ", UNIT_ROUTER);

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
//	int destId = addr->id[lev];

	int destEight = addr->id[translator->convertLevel("NET4")];
	int destGrid = addr->id[translator->convertLevel("NET3")];
	int destQuad = addr->id[translator->convertLevel("NET2")];

	if (myEight == destEight) { //check for use of inter-grid express lanes
		if (destGrid == myGrid) {
			if (adjacent(myQuad, destQuad)) {
				return routeQuad[quadExpressPort[destQuad]];
			} else {
				return toMiddle;
			}

		} else {
			if (adjacent(myGrid, destGrid)) {
				if (myQuad == gridExpressQuad[destGrid]) {
					return routeQuad[gridExpressPort];
				} else if (adjacent(myQuad, gridExpressQuad[destGrid])) {
					return routeQuad[quadExpressPort[gridExpressQuad[destGrid]]];
				} else {
					return toMiddle;
				}
			} else {
				return toMiddle;
			}

		}
	} else {
		return toMiddle;
	}

}

int Arbiter_SR_Quad::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling downPort.. ", UNIT_ROUTER);

	return gatewayAccess;
}
