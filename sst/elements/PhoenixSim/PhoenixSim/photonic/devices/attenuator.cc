/*
 * attenuator.cc
 *
 *  Created on: Jun 7, 2009
 *      Author: SolidSnake
 */

#include "attenuator.h"

Define_Module(attenuator);

attenuator::attenuator() {
	// TODO Auto-generated constructor stub

}

attenuator::~attenuator() {
	// TODO Auto-generated destructor stub
}



void attenuator::initialize(){

	phIn = gate("phIn$i");
	phOut = gate("phOut$o");
	cntrl = gate("cntrl");
}

void attenuator::finish(){


}


void attenuator::handleMessage(cMessage* msg){

	cGate* arr = msg->getArrivalGate();

	if(arr == phIn){ //attenuate the signal here
		send(msg, phOut);
	}else if(arr == cntrl){
		RouterCntrlMsg* rmsg = check_and_cast<RouterCntrlMsg*>(msg);
		level = rmsg->getData();

		delete msg;
	}

}
