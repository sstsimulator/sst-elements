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

#include "PhotonicArbiter.h"

PhotonicArbiter::PhotonicArbiter() {
	// TODO Auto-generated constructor stub

}

PhotonicArbiter::~PhotonicArbiter() {
	for (int i = 0; i < numPorts; i++) {

		if(InputGateState[i] != gateFree){
			std::cout << "router " << name << " in ";
			std::cout << ((InputGateState[i] == gateSetup) ? "gateSetup" : "gateActive");
			std::cout << " state when simulation finished." << endl;
			break;
		}

	}
}

void PhotonicArbiter::init(string id, int lev, int x, int y, int vc, int ports,
		int numpse, int buff_size, string n) {

	ElectronicArbiter::init(id, lev, x, y, vc, ports, numpse, buff_size, n);

	turnaround = false;

	for (int i = 0; i < numPorts; i++) {

		InputGateDir[i] = NA;
		OutputGateDir[i] = NA;
		InputGateState[i] = gateFree;
		OutputGateState[i] = gateFree;
	}

	for (int i = 0; i < numPSE; i++) {
		nextPSEState[i] = PSE_DEFAULT_STATE;
		currPSEState[i] = PSE_DEFAULT_STATE;
	}

}

//returns true if the request can be accommodated.
//also, as a side effect, if true, the gate states are updated
int PhotonicArbiter::pathStatus(ArbiterRequestMsg* rmsg, int outport) {
	int inport = rmsg->getPortIn();
	int reqType = rmsg->getReqType();


	int b = ElectronicArbiter::pathStatus(rmsg, outport);

	if (b == NO_GO)
		return NO_GO;
	else {

		if (reqType == dataPacket || reqType == requestDataTx || reqType
				== grantDataTx) {

			return GO_OK;

		} else if (reqType == pathSetup) {
			if (InputGateState[inport] == gateFree && OutputGateState[outport]
					== gateFree) {
				debug(name, "ok for path setup", UNIT_PATH_SETUP);
				return GO_OK;
			} else
				return PATH_BLOCKED;

		} else if (reqType == pathACK) { //in and outports are switched, cause the message is travelling backwards
			if (InputGateState[outport] == gateSetup && OutputGateState[inport]
					== gateSetup) {
				debug(name, "ok for path ack", UNIT_PATH_SETUP);
				return GO_OK;
			} else {
				opp_error("ERROR: pathACK received, gates not in gateSetup");
			}

		} else if (reqType == pathBlocked) { //in and outports are switched, cause the message is travelling backwards
			if (InputGateState[outport] == gateSetup && OutputGateState[inport]
					== gateSetup) {
				debug(name, "ok for path blocked", UNIT_PATH_SETUP);
				return GO_OK;

			} else {
				opp_error("ERROR: pathBlocked received, gates not in gateSetup");
			}

		} else if (reqType == pathBlockedTurnaround) {
			debug(name, "ok for path blocked turnaround", UNIT_PATH_SETUP);
			return GO_OK;
		} else if (reqType == pathTeardown) {

			if (InputGateState[inport] == gateActive
					&& OutputGateState[outport] == gateActive) {
				debug(name, "ok for path teardown", UNIT_PATH_SETUP);
				return GO_OK;

			} else {
				opp_error(
						"ERROR: pathTeardown received, gates not in gateActive");
			}
		} else {
			opp_error("Photonic arbiter: checkPath: unknown request type");
		}

	}
}

list<RouterCntrlMsg*>* PhotonicArbiter::setupPath(ArbiterRequestMsg* rmsg,
		int outport) {

	list<RouterCntrlMsg*>* m = ElectronicArbiter::setupPath(rmsg, outport);

	int reqType = rmsg->getReqType();
	int inport = rmsg->getPortIn();

	if (reqType == pathBlockedTurnaround) {
		debug(name, "path blocked, turning around ", inport, UNIT_PATH_SETUP );

		m->front()->setType(router_change_blocked);
		m->front()->setData((long)myAddr);

		return m;

	} else if (reqType == pathSetup) {

		InputGateState[inport] = gateSetup;
		OutputGateState[outport] = gateSetup;
		InputGateDir[inport] = outport;
		OutputGateDir[outport] = inport;

		debug(name, "pathSetup successful from ", inport, UNIT_PATH_SETUP );
		debug(name, "... to port ", outport, UNIT_PATH_SETUP );

	} else if (reqType == pathACK) { //in and outports are switched, cause the message is travelling backwards

		InputGateState[outport] = gateActive;
		OutputGateState[inport] = gateActive;

		PSEsetup(outport, inport, PSE_ON);

		debug(name, "pathACK from ", inport, UNIT_PATH_SETUP );
		debug(name, "... to port ", outport, UNIT_PATH_SETUP );

	} else if (reqType == pathBlocked) { //in and outports are switched, cause the message is travelling backwards

		InputGateState[outport] = gateFree;
		OutputGateState[inport] = gateFree;
		InputGateDir[outport] = NA;
		OutputGateDir[inport] = NA;

		debug(name, "pathBlocked from ", inport, UNIT_PATH_SETUP );
		debug(name, "... to port ", outport, UNIT_PATH_SETUP );

	} else if (reqType == pathTeardown) {

		InputGateState[inport] = gateFree;
		OutputGateState[outport] = gateFree;
		InputGateDir[inport] = NA;
		OutputGateDir[outport] = NA;

		PSEsetup(inport, outport, PSE_OFF);

		debug(name, "pathTeardown  from ", inport, UNIT_PATH_SETUP );
		debug(name, "... to port ", outport, UNIT_PATH_SETUP );
	}

	getPSEmessages(m);

	return m;

}

void PhotonicArbiter::getPSEmessages(list<RouterCntrlMsg*>* msgs) {

	for (int i = 0; i < numPSE; i++) {

		if (nextPSEState[i] != currPSEState[i]) {
			ElementControlMessage* msg = new ElementControlMessage();
			msg->setState(nextPSEState[i]);
			msg->setPSEid(i);
			msg->setType(router_pse_setup);
			msgs->push_back(msg);
			currPSEState[i] = nextPSEState[i];
		}
	}

}

void PhotonicArbiter::pathNotSetup(ArbiterRequestMsg* rmsg, int p) {
	if (p == PATH_BLOCKED && rmsg->getReqType() == pathSetup) {
		turnaround = true;
		rmsg->setReqType(pathBlockedTurnaround);
		rmsg->setNewVC(VirtualChannelControl::control->getVC(pathBlocked, (ProcessorData*)rmsg->getData()));
	}
}

void PhotonicArbiter::cleanup(int numG){
	//if(numG == 0){
	//	if(!turnaround)
	//		stalled = true;
	//}


	//turnaround = false;
}


int PhotonicArbiter::getOutport(ArbiterRequestMsg* rmsg) {
	int reqType = rmsg->getReqType();
	int inport = rmsg->getPortIn();

	if (reqType == pathBlockedTurnaround || reqType == pathSetupCancelled) {
		debug(name, "getOutport: returning inport", UNIT_PATH_SETUP);

		return inport;
	} else if (reqType == pathACK || reqType == pathBlocked) {
		debug(name, "getOutport: OutputGateDir, inport = ", inport,
				UNIT_PATH_SETUP);
		debug(name, "getOutport: reqType = ", reqType, UNIT_PATH_SETUP);
		return OutputGateDir[inport];
	} else if (reqType == pathTeardown) {
		debug(name, "getOutport: returning InputGateDir", UNIT_PATH_SETUP);
		return InputGateDir[inport];
	} else {
		debug(name, "getOutport: defaulting to Arbiter::getOutport",
				UNIT_PATH_SETUP);

		return ElectronicArbiter::getOutport(rmsg);
	}
}

