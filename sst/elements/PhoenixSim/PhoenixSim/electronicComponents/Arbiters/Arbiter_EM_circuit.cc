/*
 * Arbiter_TNX_Gateway.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#include "Arbiter_EM_circuit.h"

Arbiter_EM_Circuit::Arbiter_EM_Circuit() {
	// TODO Auto-generated constructor stub

}

Arbiter_EM_Circuit::~Arbiter_EM_Circuit() {
	// TODO Auto-generated destructor stub
}

void Arbiter_EM_Circuit::init(string id, int lev, int x, int y, int vc, int ports,
		int numpse, int buff_size, string n) {

	PhotonicNoUturnArbiter::init(id, lev, x, y, vc, ports, numpse, buff_size, n);


	for (int i = 0; i < numPSE; i++) {
		nextPSEState[i] = -1;
		currPSEState[i] = -1;
	}

}

int Arbiter_EM_Circuit::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];



	if (lev == translator->convertLevel("DRAM") && destId == 1) {
		if (myY == 0) {

			return Node_N;
		} else if (myY == numY - 1) {

			return Node_S;
		} else if (myX == 0) {

			return Node_W;
		} else if (myX == numX - 1) {

			return Node_E;
		} else {
			return route(rmsg);
		}
	}else if(this->variant == 0){
		return Node_Out;
	}else if(this->variant == 1){
		int p = addr->id[translator->convertLevel("PROC")];
		return Node_Out + p;

	}
	return 0; // Added to avoid Compile Warning
}

int Arbiter_EM_Circuit::route(ArbiterRequestMsg* rmsg) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];
	int destx;
	int desty;

	destx = destId % numX;
	desty = destId / numY;


	if (destx > myX) {
		return Node_E;
	} else if (destx < myX) {
		return Node_W;
	}

	if (desty > myY) {
		return Node_S;
	} else if (desty < myY) {
		return Node_N;
	}

	throw cRuntimeError("hey2");
}

void Arbiter_EM_Circuit::PSEsetup(int inport, int outport, PSE_STATE st) {
	if (st == PSE_OFF)
		nextPSEState[inport] = -1;
	else {
		nextPSEState[inport] = outport;
	}

}
