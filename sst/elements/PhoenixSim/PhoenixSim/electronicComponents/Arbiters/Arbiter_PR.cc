/*
 * Arbiter_EM.cc
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#include "Arbiter_PR.h"

Arbiter_PR::Arbiter_PR(){
	//isConc = isC;
}

Arbiter_PR::~Arbiter_PR() {
	// TODO Auto-generated destructor stub
}




//returns the port num to go to
int Arbiter_PR::route(ArbiterRequestMsg* rmsg){
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	return destId;

}




int Arbiter_PR::getUpPort(ArbiterRequestMsg* rmsg, int lev){
	return numPorts - 1;
}


