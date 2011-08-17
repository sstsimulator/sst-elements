/*
 * Arbiter_CM.cc
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#include "Arbiter_CM.h"

Arbiter_CM::Arbiter_CM() {
	//isConc = isC;
}

Arbiter_CM::~Arbiter_CM() {
	// TODO Auto-generated destructor stub
}

//returns the port num to go to
int Arbiter_CM::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];
	int destX;
	int destY;

	//if(isConc){
	//	destX = (destId % (numX * 2)) / 2;
	//	destY = (destId / (numY * 2)) / 2;
	//}else{
	destX = destId % numX;
	destY = destId / numY;
	//}

	debug(name, "destId ", destId, UNIT_ROUTER);
	debug(name, "...myId ", myAddr->id[level], UNIT_ROUTER);
	debug(name, "...myX ", myX, UNIT_ROUTER);
	debug(name, "...myY ", myY, UNIT_ROUTER);

	if (destX > myX) {
		if (myY == 0 || myY == numY - 1) {
			if (destX - myX >= 2) {
				if (myX % 4 == 1 || myX % 4 == 0) {
					return (myY == 0) ? CM_N : CM_S;
				}
			}
		}

		return CM_E;
	} else if (destX < myX) {
		if (myY == 0 || myY == numY - 1) {
			if (myX - destX >= 2) {
				if (myX % 4 == 2 || myX % 4 == 3) {
					return (myY == 0) ? CM_N : CM_S;
				}
			}
		}

		return CM_W;
	} else //(destX == myX)
	{
		if (destY > myY) {
			if (myX == 0 || myX == numX - 1) {
				if (destY - myY >= 2) {
					if (myY % 4 == 1 || myY % 4 == 0) {
						return (myX == 0) ? CM_W : CM_E;
					}
				}
			}

			return CM_S;
		} else if (destY < myY) {
			if (myX == 0 || myX == numX - 1) {
				if (myY - destY >= 2) {
					if (myY % 4 == 2 || myY % 4 == 3) {
						return (myX == 0) ? CM_W : CM_E;
					}
				}
			}
			return CM_N;
		} else {
			//if(destType == node_type_memory){

			//	return numPorts-1;
			//}else
			//	return CM_Node + isConc*(((destId % (numX*2)) % 2) + 2*((destId / (numY*2)) % 2));

			return CM_Node;
		}
	}

}

int Arbiter_CM::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	if (lev == translator->convertLevel("DRAM") && destId == 1) {
		return CM_Mem;
	}

	return CM_Node;
}
