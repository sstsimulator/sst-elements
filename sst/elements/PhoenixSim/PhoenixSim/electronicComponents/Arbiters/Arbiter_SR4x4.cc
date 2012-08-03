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

#include "Arbiter_SR4x4.h"

Arbiter_SR4x4::Arbiter_SR4x4() {
	gatewayAccess = (int**) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		gatewayAccess[i] = (int*) malloc(4 * sizeof(int));
	}

	routeGrid = (int***) malloc(4 * sizeof(int**));
	for (int i = 0; i < 4; i++) {
		routeGrid[i] = (int**) malloc(4 * sizeof(int*));
		for (int j = 0; j < 4; j++) {
			routeGrid[i][j] = (int*) malloc(3 * sizeof(int));
		}
	}

	routeMiddle = (int**) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		routeMiddle[i] = (int*) malloc(4 * sizeof(int));
	}

	routeQuad = (int**) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		routeQuad[i] = (int*) malloc(3 * sizeof(int));
	}
}

Arbiter_SR4x4::~Arbiter_SR4x4() {

	for (int i = 0; i < 4; i++) {
		free(gatewayAccess[i]);
	}
	free(gatewayAccess);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			free(routeGrid[i][j]);
		}
	}
	free(routeGrid);

	for (int i = 0; i < 4; i++) {
		free(routeMiddle[i]);
	}
	free(routeMiddle);

	for (int i = 0; i < 4; i++) {
		free(routeQuad[i]);
	}
	free(routeQuad);
}

void Arbiter_SR4x4::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	isMiddle = (name.find("middle") != string::npos);

	gatewayAccess[0][0] = SW_W;
	gatewayAccess[0][1] = SW_N;
	gatewayAccess[0][2] = SW_E;
	gatewayAccess[0][3] = SW_S;

	gatewayAccess[1][0] = SW_N;
	gatewayAccess[1][1] = SW_E;
	gatewayAccess[1][2] = SW_S;
	gatewayAccess[1][3] = SW_W;

	gatewayAccess[2][0] = SW_W;
	gatewayAccess[2][1] = SW_N;
	gatewayAccess[2][2] = SW_E;
	gatewayAccess[2][3] = SW_S;

	gatewayAccess[3][0] = SW_N;
	gatewayAccess[3][1] = SW_E;
	gatewayAccess[3][2] = SW_S;
	gatewayAccess[3][3] = SW_W;

	routeGrid[0][0][1] = SW_N;
	routeGrid[0][0][2] = SW_E;
	routeGrid[0][0][3] = SW_S;
	routeGrid[0][1][1] = SW_W;
	routeGrid[0][1][2] = SW_E;
	routeGrid[0][1][3] = SW_E;
	routeGrid[0][2][1] = SW_S;
	routeGrid[0][2][2] = SW_S;
	routeGrid[0][2][3] = SW_W;
	routeGrid[0][3][1] = SW_N;
	routeGrid[0][3][2] = SW_E;
	routeGrid[0][3][3] = SW_W;

	routeGrid[1][0][0] = SW_E;
	routeGrid[1][0][2] = SW_W;
	routeGrid[1][0][3] = SW_W;
	routeGrid[1][1][0] = SW_N;
	routeGrid[1][1][2] = SW_S;
	routeGrid[1][1][3] = SW_W;
	routeGrid[1][2][0] = SW_N;
	routeGrid[1][2][2] = SW_E;
	routeGrid[1][2][3] = SW_W;
	routeGrid[1][3][0] = SW_S;
	routeGrid[1][3][2] = SW_E;
	routeGrid[1][3][3] = SW_S;

	routeGrid[2][0][0] = SW_N;
	routeGrid[2][0][1] = SW_E;
	routeGrid[2][0][3] = SW_N;
	routeGrid[2][1][0] = SW_W;
	routeGrid[2][1][1] = SW_E;
	routeGrid[2][1][3] = SW_S;
	routeGrid[2][2][0] = SW_W;
	routeGrid[2][2][1] = SW_N;
	routeGrid[2][2][3] = SW_S;
	routeGrid[2][3][0] = SW_W;
	routeGrid[2][3][1] = SW_W;
	routeGrid[2][3][3] = SW_E;

	routeGrid[3][0][0] = SW_W;
	routeGrid[3][0][1] = SW_E;
	routeGrid[3][0][2] = SW_S;
	routeGrid[3][1][0] = SW_W;
	routeGrid[3][1][1] = SW_N;
	routeGrid[3][1][2] = SW_N;
	routeGrid[3][2][0] = SW_E;
	routeGrid[3][2][1] = SW_E;
	routeGrid[3][2][2] = SW_W;
	routeGrid[3][3][0] = SW_N;
	routeGrid[3][3][1] = SW_E;
	routeGrid[3][3][2] = SW_S;

	routeMiddle[0][0] = SW_E;
	routeMiddle[0][1] = SW_N;
	routeMiddle[0][2] = SW_S;
	routeMiddle[0][3] = SW_W;

	routeMiddle[1][0] = SW_N;
	routeMiddle[1][1] = SW_W;
	routeMiddle[1][2] = SW_E;
	routeMiddle[1][3] = SW_S;

	routeMiddle[2][0] = SW_W;
	routeMiddle[2][1] = SW_E;
	routeMiddle[2][2] = SW_N;
	routeMiddle[2][3] = SW_S;

	routeMiddle[3][0] = SW_W;
	routeMiddle[3][1] = SW_N;
	routeMiddle[3][2] = SW_S;
	routeMiddle[3][3] = SW_E;

	routeQuad[0][1] = SW_E;
	routeQuad[0][2] = SW_E;
	routeQuad[0][3] = SW_S;

	routeQuad[1][0] = SW_W;
	routeQuad[1][2] = SW_S;
	routeQuad[1][3] = SW_S;

	routeQuad[2][0] = SW_W;
	routeQuad[2][1] = SW_N;
	routeQuad[2][3] = SW_W;

	routeQuad[3][0] = SW_N;
	routeQuad[3][1] = SW_E;
	routeQuad[3][2] = SW_E;

}

//returns the port num to go to
int Arbiter_SR4x4::route(ArbiterRequestMsg* rmsg) {

	debug(name, "calling route.. ", UNIT_ROUTER);

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	if (isMiddle) { //i'm a middle router somewhere
		return routeMiddle[myAddr->id[level] - 4][destId];
	} else {
		int myUnit = myAddr->id[translator->convertLevel("NET1")];

		return routeQuad[myUnit][destId];
	}

}

int Arbiter_SR4x4::getUpPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling upPort.. ", UNIT_ROUTER);

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	int myQuad = myAddr->id[translator->convertLevel("NET2")];
	int myUnit = myAddr->id[translator->convertLevel("NET1")];

	return routeGrid[myQuad][myUnit][destId];
}

int Arbiter_SR4x4::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling downPort.. ", UNIT_ROUTER);

	int myQuad = myAddr->id[translator->convertLevel("NET2")];
	int myUnit = myAddr->id[translator->convertLevel("NET1")];

	return gatewayAccess[myQuad][myUnit];
}



