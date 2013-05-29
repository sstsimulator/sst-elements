/*
 * Arbiter_EM.cc
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#include "Arbiter_PC.h"

Arbiter_PC::Arbiter_PC() {
	//isConc = isC;
}

Arbiter_PC::~Arbiter_PC() {
	// TODO Auto-generated destructor stub
}

//returns the port num to go to
int Arbiter_PC::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	debug(name, "destId ", destId, UNIT_ROUTER);
	debug(name, "...myId ", myAddr->id[level], UNIT_ROUTER);
	debug(name, "...myX ", myX, UNIT_ROUTER);
	debug(name, "...myY ", myY, UNIT_ROUTER);

	/*
	if (destX > myX) {
		return EM_E;
	} else if (destX < myX) {
		return EM_W;
	} else //(destX == myX)
	{
		if (destY > myY) {
			return EM_S;
		} else if (destY < myY) {
			return EM_N;
		} else {

			return EM_Node;
		}
	}*/



	if(destId == myAddr->id[level]) // going to the node
	{
		return numX*numY;
	}
	else
	{
		return destId - ((destId > myAddr->id[level])?1:0);
	}

}


int Arbiter_PC::getDownPort(ArbiterRequestMsg* rmsg, int lev) {

	NetworkAddress* __attribute__ ((unused)) addr = (NetworkAddress*) rmsg->getDest();
//	int destId = addr->id[lev];

	int portToNode = numX*numY-1;

	if(this->variant == 0)
	{
		return portToNode;
	}
	else if(this->variant == 1)
	{
		int p = addr->id[translator->convertLevel("PROC")];
		return portToNode + p;
	}
	return 0; // Added to avoid Compile Warning
}
