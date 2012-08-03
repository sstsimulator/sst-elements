/*
 * GatewaySignalControl.cc
 *
 *  Created on: Jun 7, 2009
 *      Author: SolidSnake
 */

#include "GatewaySignalControl.h"

Define_Module(GatewaySignalControl);

GatewaySignalControl::GatewaySignalControl() {
	// TODO Auto-generated constructor stub

}

GatewaySignalControl::~GatewaySignalControl() {
	// TODO Auto-generated destructor stub
}


void GatewaySignalControl::initialize(){

	wavelengths = par("numOfWavelengthChannels");
	doVWT = par("doVWT");

	for(int i = 0; i < wavelengths; i++){
		toSel[i] = gate("toSelectors", i);
	}

	toLBC = gate("toLBC");
	toAtt = gate("toAttenuator");

	elInProc = gate("elInProc");
	elOutProc = gate("elOutProc");

	elInNet = gate("elInNet");
	elOutNet = gate("elOutNet");




}

void GatewaySignalControl::finish(){


}


void GatewaySignalControl::handleMessage(cMessage* msg){

	cGate* arr = msg->getArrivalGate();
	ElectronicMessage* emsg = check_and_cast<ElectronicMessage*>(msg);

	if(!doVWT){
		if (arr == elInProc) {
			send(emsg, elOutNet);
		} else if (arr == elInNet) {
			send(emsg, elOutProc);
		}
	}else{

		FSM_Switch  (state) {

			case FSM_Enter(SLEEP):
				used = 0;

				break;


			case FSM_Exit(SLEEP):
				if(arr == elInProc){
					if(emsg->getMsgType() == pathSetup){
						VWTcntrlMsg *vwt = check_and_cast<VWTcntrlMsg*>(emsg);

						used = vwt->getWavelengths();
						FSM_Goto(state, SendLBC);
					}

					send(emsg, elOutNet);
				}else if(arr == elInNet){


					send(emsg, elOutProc);
				}

				break;



			case FSM_Enter(SendLBC):

				break;


			case FSM_Exit(SendLBC):
				FSM_Goto(state, WaitACK);
				break;


			case FSM_Enter(WaitACK):

				break;

			case FSM_Exit(WaitACK):
				if(arr == elInProc){

					send(emsg, elOutNet);
				}else if(arr == elInNet){
					if(emsg->getMsgType() == pathACK){
						FSM_Goto(state, SendLBC);
					}else if(emsg->getMsgType() == pathBlocked){
						send(emsg, elOutProc);
						FSM_Goto(state, SLEEP);
					}else{
						opp_error("ERROR: unexpected msg type in WaitACK.");
					}

				}
				break;


			case FSM_Enter(PrepareSignal): {

				for(int i = 0; i < wavelengths; i++){

					DeviceCntrlMsg* rmsg = new DeviceCntrlMsg();

					rmsg->setData(i < used);

					send(rmsg, toSel[i]);
				}

				DeviceCntrlMsg* amsg = new DeviceCntrlMsg();
				amsg->setData((double)used / (double)wavelengths);
				send(amsg, toAtt);

				break;
			}

			case FSM_Exit(PrepareSignal):

				//have to send the pathACK to the proc once we have prepared the signal
				// possibly with a delay to make sure the rings and attenuator are set
				send(emsg, elOutProc);

				FSM_Goto(state, WaitTeardown);
				break;



			case FSM_Enter(WaitTeardown):

				break;

			case FSM_Exit(WaitTeardown):
				if(arr == elInProc){

					if(emsg->getMsgType() == pathTeardown){
						FSM_Goto(state, TurnOff);
					}

					send(emsg, elOutNet);
				}else if(arr == elInNet){
					send(emsg, elOutProc);
				}
				break;



			case FSM_Enter(TurnOff):

				break;


			case FSM_Exit(TurnOff):

				FSM_Goto(state, SLEEP);
				break;

		}

	}


}
