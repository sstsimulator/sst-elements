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

#include "Arbiter_SR16x16.h"

Arbiter_SR16x16::Arbiter_SR16x16() {

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

	routeLevelUp = (int***) malloc(4 * sizeof(int**));
	for (int i = 0; i < 4; i++) {
		routeLevelUp[i] = (int**) malloc(4 * sizeof(int*));
		for (int j = 0; j < 4; j++) {
			routeLevelUp[i][j] = (int*) malloc(4 * sizeof(int));
		}
	}

	routeInterGridExpress = (int***) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		routeInterGridExpress[i] = (int**) malloc(4 * sizeof(int*));
		for (int j = 0; j < 4; j++) {
			routeInterGridExpress[i][j] = (int*) malloc(4 * sizeof(int));
			for (int k = 0; k < 4; k++) {
				routeInterGridExpress[i][j][k] = -1;
			}
		}
	}

	routeLevelTwo = (int***) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		routeLevelTwo[i] = (int**) malloc(4 * sizeof(int*));
		for (int j = 0; j < 4; j++) {
			routeLevelTwo[i][j] = (int*) malloc(5 * sizeof(int));
		}
	}

	routeExpressDirection = (int***) malloc(4 * sizeof(int**));
	for (int i = 0; i < 4; i++) {
		routeExpressDirection[i] = (int**) malloc(4 * sizeof(int*));
		for (int j = 0; j < 4; j++) {
			routeExpressDirection[i][j] = (int*) malloc(4 * sizeof(int));
			for (int k = 0; k < 4; k++) {
				routeExpressDirection[i][j][k] = -1;
			}
		}
	}

	routeQuad = (int**) malloc(4 * sizeof(int*));
	for (int i = 0; i < 4; i++) {
		routeQuad[i] = (int*) malloc(3 * sizeof(int));
	}

}

Arbiter_SR16x16::~Arbiter_SR16x16() {

	/*for (int i = 0; i < 4; i++) {
		free(gatewayAccess[i]);
	}
	free(gatewayAccess);

	for (int i = 0; i < 16; i++) {
		free(routeGrid[i]);
	}
	free(routeGrid);

	for (int i = 0; i < 4; i++) {
		free(routeMiddle[i]);
	}
	free(routeMiddle);

	for (int i = 0; i < 4; i++) {
		free(routeLevelUp[i]);
	}
	free(routeLevelUp);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			free(routeInterGridExpress[i][j]);
		}
		free(routeInterGridExpress[i]);
	}
	free(routeInterGridExpress);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			free(routeLevelTwo[i][j]);
		}
		free(routeLevelTwo[i]);
	}
	free(routeLevelTwo);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			free(routeExpressDirection[i][j]);
		}
		free(routeExpressDirection[i]);
	}
	free(routeExpressDirection);

	for (int i = 0; i < 4; i++) {
		free(routeQuad[i]);
	}
	free(routeQuad);*/
}

void Arbiter_SR16x16::init(string id, int level, int numX, int numY, int vc,
		int ports, int numpse, int buff, string n) {
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff, n);

	isMiddle = (name.find("middle") != string::npos);

	myGrid = myAddr->id[translator->convertLevel("NET3")];

	oldIDsQuad[0] = 0;
	oldIDsQuad[1] = 0;
	oldIDsQuad[2] = 1;
	oldIDsQuad[3] = 1;
	oldIDsQuad[4] = 0;
	oldIDsQuad[5] = 0;
	oldIDsQuad[6] = 1;
	oldIDsQuad[7] = 1;
	oldIDsQuad[8] = 3;
	oldIDsQuad[9] = 3;
	oldIDsQuad[10] = 2;
	oldIDsQuad[11] = 2;
	oldIDsQuad[12] = 3;
	oldIDsQuad[13] = 3;
	oldIDsQuad[14] = 2;
	oldIDsQuad[15] = 2;

	oldIDsUnit[0] = 0;
	oldIDsUnit[1] = 1;
	oldIDsUnit[2] = 0;
	oldIDsUnit[3] = 1;
	oldIDsUnit[4] = 3;
	oldIDsUnit[5] = 2;
	oldIDsUnit[6] = 3;
	oldIDsUnit[7] = 2;
	oldIDsUnit[8] = 0;
	oldIDsUnit[9] = 1;
	oldIDsUnit[10] = 0;
	oldIDsUnit[11] = 1;
	oldIDsUnit[12] = 3;
	oldIDsUnit[13] = 2;
	oldIDsUnit[14] = 3;
	oldIDsUnit[15] = 2;

	//myQuad, myUnit
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

	//myQuad, myUnit, destQuad
	routeGrid[0][0][1] = SW_N;
	routeGrid[0][0][2] = (myGrid == 2) ? SW_N : SW_E;
	routeGrid[0][0][3] = SW_S;
	routeGrid[0][1][1] = SW_W;
	routeGrid[0][1][2] = (myGrid == 2) ? SW_S : SW_E;
	routeGrid[0][1][3] = (myGrid == 2) ? SW_S : SW_E;
	routeGrid[0][2][1] = (myGrid == 1) ? SW_N : SW_S;
	routeGrid[0][2][2] = (myGrid == 1) ? SW_N : SW_S;
	routeGrid[0][2][3] = SW_W;
	routeGrid[0][3][1] = SW_N;
	routeGrid[0][3][2] = (myGrid == 0 || myGrid == 1) ? SW_W : SW_E;
	routeGrid[0][3][3] = SW_W;

	routeGrid[1][0][0] = SW_E;
	routeGrid[1][0][2] = (myGrid == 3) ? SW_S : SW_W;
	routeGrid[1][0][3] = (myGrid == 3) ? SW_S : SW_W;
	routeGrid[1][1][0] = SW_N;
	routeGrid[1][1][2] = SW_S;
	routeGrid[1][1][3] = (myGrid == 3) ? SW_N : SW_W;
	routeGrid[1][2][0] = SW_N;
	routeGrid[1][2][2] = SW_E;
	routeGrid[1][2][3] = (myGrid == 0 || myGrid == 1) ? SW_E : SW_W;
	routeGrid[1][3][0] = (myGrid == 0) ? SW_E : SW_S;
	routeGrid[1][3][2] = SW_E;
	routeGrid[1][3][3] = (myGrid == 0) ? SW_N : SW_S;

	routeGrid[2][0][0] = (myGrid == 3) ? SW_S : SW_N;
	routeGrid[2][0][1] = SW_E;
	routeGrid[2][0][3] = (myGrid == 3) ? SW_S : SW_N;
	routeGrid[2][1][0] = (myGrid == 3 || myGrid == 2) ? SW_E : SW_W;
	routeGrid[2][1][1] = SW_E;
	routeGrid[2][1][3] = SW_S;
	routeGrid[2][2][0] = (myGrid == 0) ? SW_N : SW_W;
	routeGrid[2][2][1] = SW_N;
	routeGrid[2][2][3] = SW_S;
	routeGrid[2][3][0] = (myGrid == 0) ? SW_N : SW_W;
	routeGrid[2][3][1] = (myGrid == 0) ? SW_E : SW_W;
	routeGrid[2][3][3] = SW_E;

	routeGrid[3][0][0] = SW_W;
	routeGrid[3][0][1] = (myGrid == 3 || myGrid == 2) ? SW_W : SW_E;
	routeGrid[3][0][2] = SW_S;
	routeGrid[3][1][0] = SW_W;
	routeGrid[3][1][1] = (myGrid == 2) ? SW_S : SW_N;
	routeGrid[3][1][2] = (myGrid == 2) ? SW_W : SW_N;

	routeGrid[3][2][0] = (myGrid == 1) ? SW_W : SW_E;
	routeGrid[3][2][1] = (myGrid == 1) ? SW_N : SW_E;
	routeGrid[3][2][2] = SW_W;
	routeGrid[3][3][0] = SW_N;
	routeGrid[3][3][1] = (myGrid == 1) ? SW_S : SW_E;
	routeGrid[3][3][2] = SW_S;

	//middleId, destGrid or quad
	routeMiddle[0][0] = SW_E;
	routeMiddle[0][1] = SW_N;
	routeMiddle[0][3] = SW_W;
	routeMiddle[0][2] = SW_S;
	routeMiddle[1][0] = SW_N;
	routeMiddle[1][1] = SW_W;
	routeMiddle[1][3] = SW_S;
	routeMiddle[1][2] = SW_E;
	routeMiddle[3][0] = SW_W;
	routeMiddle[3][1] = SW_N;
	routeMiddle[3][3] = SW_E;
	routeMiddle[3][2] = SW_S;
	routeMiddle[2][0] = SW_W;
	routeMiddle[2][1] = SW_E;
	routeMiddle[2][3] = SW_S;
	routeMiddle[2][2] = SW_N;

	//gridId, routerId, destQuad
	routeLevelTwo[0][0][0] = SW_E;
	routeLevelTwo[0][0][1] = SW_N;
	routeLevelTwo[0][0][3] = SW_W;
	routeLevelTwo[0][0][2] = SW_S;
	routeLevelTwo[0][0][4] = SW_S;
	routeLevelTwo[0][1][0] = SW_N;
	routeLevelTwo[0][1][1] = SW_W;
	routeLevelTwo[0][1][3] = SW_W;
	routeLevelTwo[0][1][2] = SW_E;
	routeLevelTwo[0][1][4] = SW_S;
	routeLevelTwo[0][3][0] = SW_W;
	routeLevelTwo[0][3][1] = SW_N;
	routeLevelTwo[0][3][3] = SW_E;
	routeLevelTwo[0][3][2] = SW_E;
	routeLevelTwo[0][3][4] = SW_S;
	routeLevelTwo[0][2][0] = SW_N;
	routeLevelTwo[0][2][1] = SW_N;
	routeLevelTwo[0][2][3] = SW_S;
	routeLevelTwo[0][2][2] = SW_N;
	routeLevelTwo[0][2][4] = SW_E;

	routeLevelTwo[1][0][0] = SW_E;
	routeLevelTwo[1][0][1] = SW_N;
	routeLevelTwo[1][0][3] = SW_W;
	routeLevelTwo[1][0][2] = SW_S;
	routeLevelTwo[1][0][4] = SW_S;
	routeLevelTwo[1][1][0] = SW_N;
	routeLevelTwo[1][1][1] = SW_W;
	routeLevelTwo[1][1][3] = SW_W;
	routeLevelTwo[1][1][2] = SW_E;
	routeLevelTwo[1][1][4] = SW_S;
	routeLevelTwo[1][3][0] = SW_E;
	routeLevelTwo[1][3][1] = SW_N;
	routeLevelTwo[1][3][3] = SW_N;
	routeLevelTwo[1][3][2] = SW_S;
	routeLevelTwo[1][3][4] = SW_W;
	routeLevelTwo[1][2][0] = SW_N;
	routeLevelTwo[1][2][1] = SW_E;
	routeLevelTwo[1][2][3] = SW_W;
	routeLevelTwo[1][2][2] = SW_N;
	routeLevelTwo[1][2][4] = SW_S;

	routeLevelTwo[3][0][0] = SW_E;
	routeLevelTwo[3][0][1] = SW_E;
	routeLevelTwo[3][0][3] = SW_W;
	routeLevelTwo[3][0][2] = SW_S;
	routeLevelTwo[3][0][4] = SW_N;
	routeLevelTwo[3][1][0] = SW_N;
	routeLevelTwo[3][1][1] = SW_S;
	routeLevelTwo[3][1][3] = SW_W;
	routeLevelTwo[3][1][2] = SW_S;
	routeLevelTwo[3][1][4] = SW_E;
	routeLevelTwo[3][3][0] = SW_W;
	routeLevelTwo[3][3][1] = SW_E;
	routeLevelTwo[3][3][3] = SW_N;
	routeLevelTwo[3][3][2] = SW_S;
	routeLevelTwo[3][3][4] = SW_N;
	routeLevelTwo[3][2][0] = SW_N;
	routeLevelTwo[3][2][1] = SW_E;
	routeLevelTwo[3][2][3] = SW_S;
	routeLevelTwo[3][2][2] = SW_W;
	routeLevelTwo[3][2][4] = SW_N;

	routeLevelTwo[2][0][0] = SW_S;
	routeLevelTwo[2][0][1] = SW_N;
	routeLevelTwo[2][0][3] = SW_S;
	routeLevelTwo[2][0][2] = SW_E;
	routeLevelTwo[2][0][4] = SW_W;
	routeLevelTwo[2][1][0] = SW_W;
	routeLevelTwo[2][1][1] = SW_S;
	routeLevelTwo[2][1][3] = SW_S;
	routeLevelTwo[2][1][2] = SW_E;
	routeLevelTwo[2][1][4] = SW_N;
	routeLevelTwo[2][3][0] = SW_W;
	routeLevelTwo[2][3][1] = SW_E;
	routeLevelTwo[2][3][3] = SW_E;
	routeLevelTwo[2][3][2] = SW_S;
	routeLevelTwo[2][3][4] = SW_N;
	routeLevelTwo[2][2][0] = SW_W;
	routeLevelTwo[2][2][1] = SW_E;
	routeLevelTwo[2][2][3] = SW_S;
	routeLevelTwo[2][2][2] = SW_N;
	routeLevelTwo[2][2][4] = SW_N;

	//gridId, myGridid
	routeLevelUp[0][0][0] = SW_E;
	routeLevelUp[0][0][1] = SW_E;
	routeLevelUp[0][1][0] = SW_W;
	routeLevelUp[0][1][1] = SW_W;
	routeLevelUp[0][0][3] = SW_E;
	routeLevelUp[0][0][2] = SW_S;
	routeLevelUp[0][1][3] = SW_N;
	routeLevelUp[0][1][2] = SW_W;
	routeLevelUp[0][3][0] = SW_E;
	routeLevelUp[0][3][1] = SW_N;
	routeLevelUp[0][2][0] = SW_N;
	routeLevelUp[0][2][1] = SW_W;
	routeLevelUp[0][3][3] = SW_E;
	routeLevelUp[0][3][2] = SW_E;
	routeLevelUp[0][2][3] = SW_N;
	routeLevelUp[0][2][2] = SW_W;

	routeLevelUp[1][0][0] = SW_E;
	routeLevelUp[1][0][1] = SW_E;
	routeLevelUp[1][1][0] = SW_W;
	routeLevelUp[1][1][1] = SW_W;
	routeLevelUp[1][0][3] = SW_E;
	routeLevelUp[1][0][2] = SW_N;
	routeLevelUp[1][1][3] = SW_S;
	routeLevelUp[1][1][2] = SW_W;
	routeLevelUp[1][3][0] = SW_E;
	routeLevelUp[1][3][1] = SW_N;
	routeLevelUp[1][2][0] = SW_N;
	routeLevelUp[1][2][1] = SW_W;
	routeLevelUp[1][3][3] = SW_E;
	routeLevelUp[1][3][2] = SW_N;
	routeLevelUp[1][2][3] = SW_W;
	routeLevelUp[1][2][2] = SW_W;

	routeLevelUp[2][0][0] = SW_E;
	routeLevelUp[2][0][1] = SW_E;
	routeLevelUp[2][1][0] = SW_S;
	routeLevelUp[2][1][1] = SW_W;
	routeLevelUp[2][0][3] = SW_E;
	routeLevelUp[2][0][2] = SW_S;
	routeLevelUp[2][1][3] = SW_S;
	routeLevelUp[2][1][2] = SW_W;
	routeLevelUp[2][3][0] = SW_E;
	routeLevelUp[2][3][1] = SW_N;
	routeLevelUp[2][2][0] = SW_S;
	routeLevelUp[2][2][1] = SW_W;
	routeLevelUp[2][3][3] = SW_E;
	routeLevelUp[2][3][2] = SW_E;
	routeLevelUp[2][2][3] = SW_W;
	routeLevelUp[2][2][2] = SW_W;

	routeLevelUp[3][0][0] = SW_E;
	routeLevelUp[3][0][1] = SW_S;
	routeLevelUp[3][1][0] = SW_W;
	routeLevelUp[3][1][1] = SW_W;
	routeLevelUp[3][0][3] = SW_E;
	routeLevelUp[3][0][2] = SW_S;
	routeLevelUp[3][1][3] = SW_S;
	routeLevelUp[3][1][2] = SW_W;
	routeLevelUp[3][3][0] = SW_E;
	routeLevelUp[3][3][1] = SW_S;
	routeLevelUp[3][2][0] = SW_N;
	routeLevelUp[3][2][1] = SW_W;
	routeLevelUp[3][3][3] = SW_E;
	routeLevelUp[3][3][2] = SW_E;
	routeLevelUp[3][2][3] = SW_W;
	routeLevelUp[3][2][2] = SW_W;

	//myGrid, myQuad, oldDestId
	routeInterGridExpress[0][0][1] = 6;
	routeInterGridExpress[0][1][1] = 6;
	routeInterGridExpress[0][2][1] = 6;
	routeInterGridExpress[0][1][3] = 14;
	routeInterGridExpress[0][2][3] = 14;
	routeInterGridExpress[0][3][3] = 14;

	routeInterGridExpress[1][0][0] = 5;
	routeInterGridExpress[1][1][0] = 5;
	routeInterGridExpress[1][3][0] = 5;
	routeInterGridExpress[1][0][2] = 13;
	routeInterGridExpress[1][2][2] = 13;
	routeInterGridExpress[1][3][2] = 13;

	routeInterGridExpress[3][0][0] = 2;
	routeInterGridExpress[3][1][0] = 2;
	routeInterGridExpress[3][2][0] = 2;
	routeInterGridExpress[3][1][2] = 10;
	routeInterGridExpress[3][2][2] = 10;
	routeInterGridExpress[3][3][2] = 10;

	routeInterGridExpress[2][0][1] = 1;
	routeInterGridExpress[2][1][1] = 1;
	routeInterGridExpress[2][3][1] = 1;
	routeInterGridExpress[2][0][3] = 9;
	routeInterGridExpress[2][2][3] = 9;
	routeInterGridExpress[2][3][3] = 9;

	//gridId, myGridId
	routeExpressDirection[0][1][3] = SW_S;
	routeExpressDirection[0][2][3] = SW_W;
	routeExpressDirection[1][0][2] = SW_S;
	routeExpressDirection[1][3][2] = SW_E;

	routeExpressDirection[2][0][1] = SW_E;
	routeExpressDirection[2][3][1] = SW_N;
	routeExpressDirection[3][1][0] = SW_W;
	routeExpressDirection[3][2][0] = SW_N;

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
int Arbiter_SR16x16::route(ArbiterRequestMsg* rmsg) {



	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destGrid = addr->id[translator->convertLevel("NET3")];
	int destQuad = addr->id[translator->convertLevel("NET2")];
	int destId = addr->id[level];

	debug(name, "start route", UNIT_ROUTER);
	if (isMiddle) { //i'm a middle router somewhere
		debug(name, "...middle router", UNIT_ROUTER);
		debug(name, "...address: ", translator->untranslate_str(myAddr), UNIT_ROUTER);

		int myGrid = myAddr->id[translator->convertLevel("NET3")];
		int myQuad = myAddr->id[translator->convertLevel("NET2")];

		if(myQuad < 0){
			debug(name, "...top level", UNIT_ROUTER);
			return routeMiddle[myGrid - 4][destGrid]; //the top of the heirarchy
		}else{

			return routeLevelTwo[myGrid][(myQuad - 4)][destQuad];
		}

	} else { //i'm in a quadrant that contains the destination (do XY routing)
		debug(name, "...same quad", UNIT_ROUTER);
		int myUnit = myAddr->id[translator->convertLevel("NET1")];

		return routeQuad[myUnit][destId];
	}

}

int Arbiter_SR16x16::getUpPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling upPort.. ", UNIT_ROUTER);

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	int myGrid = myAddr->id[translator->convertLevel("NET3")];
	int myQuad = myAddr->id[translator->convertLevel("NET2")];
	int myUnit = myAddr->id[translator->convertLevel("NET1")];


	int destGrid = addr->id[translator->convertLevel("NET3")];
	int destQuad = addr->id[translator->convertLevel("NET2")];

	if (isMiddle) {

		if (myGrid != destGrid)
			destQuad = 4;

		return routeLevelTwo[myGrid][(myQuad - 4)][destQuad];

	} else if (destGrid == myGrid) {
		return routeGrid[myQuad][myUnit][destId];
	} else {
		int exId = routeInterGridExpress[myGrid][myQuad][destGrid];

		if (exId >= 0) { //we should use an inter-grid express lane
			debug(name, "...use express lane, at express point ", exId,
					UNIT_ROUTER);

			int exQuad = oldIDsQuad[exId];
			int exUnit = oldIDsUnit[exId];

			if (exQuad == myQuad) {
				if (exUnit == myUnit) { //i'm here, take the express lane

					debug(name, "...at express point", UNIT_ROUTER);
					int dir = routeExpressDirection[myGrid][myQuad][myUnit];
					if (dir == SW_NA) {
						opp_error(
								"something wrong with the inter grid express lane routing");
					} else
						return dir;
				} else { //i'm in a quadrant that contains the destination (do XY routing)
					debug(name, "...in same quad as express point", UNIT_ROUTER);
					return routeQuad[myUnit][exUnit];
				}
			} else {
				debug(name, "...getting to express point's quad", UNIT_ROUTER);
				return routeGrid[myQuad][myUnit][exQuad];
			}

		} else {
			debug(name, "...use middle routers", UNIT_ROUTER);
			return routeLevelUp[myGrid][myQuad][myUnit];
		}
	}

}

int Arbiter_SR16x16::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	debug(name, "calling downPort.. ", UNIT_ROUTER);

	int myQuad = myAddr->id[translator->convertLevel("NET2")];
	int myUnit = myAddr->id[translator->convertLevel("NET1")];

	return gatewayAccess[myQuad][myUnit];
}
