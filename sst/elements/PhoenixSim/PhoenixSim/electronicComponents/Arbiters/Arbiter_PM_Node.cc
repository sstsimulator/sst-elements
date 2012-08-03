/*
 * Arbiter_TNX_Gateway.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#include "Arbiter_PM_Node.h"

Arbiter_PM_Node::Arbiter_PM_Node() {

}

Arbiter_PM_Node::~Arbiter_PM_Node() {
	// TODO Auto-generated destructor stub
}

int Arbiter_PM_Node::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
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
	}

	return Node_Out;
}

int Arbiter_PM_Node::route(ArbiterRequestMsg* rmsg) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];
	int destx;
	int desty;
	int inport = rmsg->getPortIn();
	destx = destId % numX;
	desty = destId / numX;

	if (desty > myY) {
		return Node_S;
	} else if (desty < myY) {
		return Node_N;
	}

	if (destx > myX) {
		return Node_E;
	} else if (destx < myX) {
		return Node_W;
	}

	throw cRuntimeError("hey2");
}

int Arbiter_PM_Node::getBcastOutport(ArbiterRequestMsg* bmsg) {
	int ret = -1;
	NetworkAddress* addr = (NetworkAddress*) bmsg->getSrc();
	int destId = addr->id[level];
	int inport = bmsg->getPortIn();
	redo: switch (bmsg->getStage()) {
	case 0: //N
		if (myY == 0 || inport == Node_N) {
			bmsg->setStage(bmsg->getStage() + 1);
			goto redo;
		} else {
			ret = Node_N;
		}
		break;
	case 1: //E
		if (myX == numX - 1 || inport == Node_E || inport == Node_N || inport
				== Node_S) {
			bmsg->setStage(bmsg->getStage() + 1);
			goto redo;
		} else {
			ret = Node_E;
		}
		break;
	case 2: //S
		if (myY == numY - 1 || inport == Node_S) {
			bmsg->setStage(bmsg->getStage() + 1);
			goto redo;
		} else {
			ret = Node_S;
		}
		break;
	case 3: //W
		if (myX == 0 || inport == Node_W || inport == Node_N || inport
				== Node_S) {
			bmsg->setStage(bmsg->getStage() + 1);
			goto redo;
		} else {
			ret = Node_W;
		}
		break;
	case 4: //local
		if (destId != myAddr->id[level])
			ret = Node_Out;
		break;
	default:
		opp_error("Arbiter_PM_Node: invalid stage in getBcastOutport");
	}

	return ret;
}

void Arbiter_PM_Node::PSEsetup(int inport, int outport, PSE_STATE st) {
	/*
	 std::cout<<"id:"<<this->myX<<" "<<this->myY<<endl;
	 map<int,int>::iterator it;
	 map<int,gateState>::iterator it2;

	 std::cout<<"input"<<endl;
	 for(it = InputGateDir.begin() ; it != InputGateDir.end() ; it++)
	 {
	 std::cout<<(*it).first<<" "<<(*it).second<<endl;
	 }
	 std::cout<<"output"<<endl;
	 for(it = OutputGateDir.begin() ; it != OutputGateDir.end() ; it++)
	 {
	 std::cout<<(*it).first<<" "<<(*it).second<<endl;
	 }
	 std::cout<<"input states"<<endl;
	 for(it2 = InputGateState.begin() ; it2 != InputGateState.end() ; it2++)
	 {
	 std::cout<<(*it2).first<<" "<<(*it2).second<<endl;
	 }
	 std::cout<<"output states"<<endl;
	 for(it2 = OutputGateState.begin() ; it2 != OutputGateState.end() ; it2++)
	 {
	 std::cout<<(*it2).first<<" "<<(*it2).second<<endl;
	 }

	 std::cout<<st<<":"<<inport<<" "<<outport<<endl;
	 */

	if (variant == 0) { //NonBlockingSwitch4x4_P (original)
		if (inport == Node_Out && outport != Node_S) {
			nextPSEState[6] = st;
		} else if (inport == Node_E && outport == Node_Out) {
			nextPSEState[8] = st;
		} else if (inport == Node_Out && outport == Node_S) {
			nextPSEState[7] = st;
		} else if (inport != Node_E && outport == Node_Out) {
			nextPSEState[9] = st;
		}

		if (inport == Node_N) {
			if (outport == Node_E || outport == Node_Out) {
				nextPSEState[4] = st;
			} else if (outport == Node_W) {

			} else if (outport == Node_S) {
				nextPSEState[1] = (InputGateState[Node_E] == gateActive
						&& InputGateDir[Node_E] == Node_W
						&& OutputGateState[Node_W] == gateActive
						&& OutputGateDir[Node_W] == Node_E) ? PSE_ON : st;
			}
		} else if (inport == Node_S || inport == Node_Out) {
			if (outport == Node_W) {
				nextPSEState[5] = st;
			} else if (outport == Node_E || outport == Node_Out) {

			} else if (outport == Node_N) {
				nextPSEState[0] = ((InputGateState[Node_W] == gateActive
						&& InputGateDir[Node_W] == Node_E
						&& OutputGateState[Node_E] == gateActive
						&& OutputGateDir[Node_E] == Node_W)
						|| (InputGateState[Node_W] == gateActive
								&& InputGateDir[Node_W] == Node_Out
								&& OutputGateState[Node_Out] == gateActive
								&& OutputGateDir[Node_Out] == Node_W)) ? PSE_ON
						: st;
			}
		} else if (inport == Node_E) {
			if (outport == Node_N) {
				nextPSEState[2] = st;
			} else if (outport == Node_S) {

			} else if (outport == Node_W) {
				nextPSEState[1] = (InputGateState[Node_N] == gateActive
						&& InputGateDir[Node_N] == Node_S
						&& OutputGateState[Node_S] == gateActive
						&& OutputGateDir[Node_S] == Node_N) ? PSE_ON : st;
			}
		} else if (inport == Node_W) {
			if (outport == Node_S) {
				nextPSEState[3] = st;
			} else if (outport == Node_N) {

			} else if (outport == Node_E || outport == Node_Out) {
				//std::cout<<"check"<<endl;
				nextPSEState[0] = ((InputGateState[Node_S] == gateActive
						&& InputGateDir[Node_S] == Node_N
						&& OutputGateState[Node_N] == gateActive
						&& OutputGateDir[Node_N] == Node_S)
						|| (InputGateState[Node_Out] == gateActive
								&& InputGateDir[Node_Out] == Node_N
								&& OutputGateState[Node_N] == gateActive
								&& OutputGateDir[Node_N] == Node_Out)) ? PSE_ON
						: st;
			}
		} else {
			opp_error("unknown input direction specified");
		}
	}

	else if (variant == 1) { //NonBlockingSwitch4x4Regular (straight path)
		if (inport == Node_Out && outport != Node_S) {
			nextPSEState[6] = st;
		} else if (inport == Node_E && outport == Node_Out) {
			nextPSEState[8] = st;
		} else if (inport == Node_Out && outport == Node_S) {
			nextPSEState[7] = st;
		} else if (inport != Node_E && outport == Node_Out) {
			nextPSEState[9] = st;
		}

		if (inport == Node_N) {
			if (outport == Node_E || outport == Node_Out) {
				nextPSEState[4] = st;
			} else if (outport == Node_S) {

			} else if (outport == Node_W) {
				nextPSEState[1] = (InputGateState[Node_E] == gateActive
						&& InputGateDir[Node_E] == Node_S
						&& OutputGateState[Node_S] == gateActive
						&& OutputGateDir[Node_S] == Node_E) ? PSE_ON : st;
			}
		} else if (inport == Node_S || inport == Node_Out) {
			if (outport == Node_W) {
				nextPSEState[5] = st;
			} else if (outport == Node_N) {

			} else if (outport == Node_E || outport == Node_Out) {
				nextPSEState[0] = (InputGateState[Node_W] == gateActive
						&& InputGateDir[Node_W] == Node_N
						&& OutputGateState[Node_N] == gateActive
						&& OutputGateDir[Node_N] == Node_W) ? PSE_ON : st;
			}
		} else if (inport == Node_E) {
			if (outport == Node_N) {
				nextPSEState[2] = st;
			} else if (outport == Node_W) {

			} else if (outport == Node_S) {
				nextPSEState[1] = (InputGateState[Node_N] == gateActive
						&& InputGateDir[Node_N] == Node_W
						&& OutputGateState[Node_W] == gateActive
						&& OutputGateDir[Node_W] == Node_N) ? PSE_ON : st;
			}
		} else if (inport == Node_W) {
			if (outport == Node_S) {
				nextPSEState[3] = st;
			} else if (outport == Node_E || outport == Node_Out) {

			} else if (outport == Node_N) {
				//std::cout<<"check"<<endl;
				nextPSEState[0] = ((InputGateState[Node_S] == gateActive
						&& InputGateDir[Node_S] == Node_E
						&& OutputGateState[Node_E] == gateActive
						&& OutputGateDir[Node_E] == Node_S)
						|| (InputGateState[Node_S] == gateActive
								&& InputGateDir[Node_S] == Node_Out
								&& OutputGateState[Node_Out] == gateActive
								&& OutputGateDir[Node_Out] == Node_S)
						|| (InputGateState[Node_Out] == gateActive
								&& InputGateDir[Node_Out] == Node_E
								&& OutputGateState[Node_E] == gateActive
								&& OutputGateDir[Node_E] == Node_Out)) ? PSE_ON
						: st;
			}
		} else {
			opp_error("unknown input direction specified");
		}
	}
	else if (variant == 2) { //NonBlockingSwitch4x4New (other one)
		if (inport == Node_Out && outport != Node_S) {
			nextPSEState[8] = st;
		} else if (inport == Node_E && outport == Node_Out) {
			nextPSEState[10] = st;
		} else if (inport == Node_Out && outport == Node_S) {
			nextPSEState[9] = st;
		} else if (inport != Node_E && outport == Node_Out) {
			nextPSEState[11] = st;
		}

		if (inport == Node_N) {
			if (outport == Node_E || outport == Node_Out) {

			} else if (outport == Node_W) {
				nextPSEState[7] = st;
			} else if (outport == Node_S) {
				nextPSEState[2] = st;
			}
		} else if (inport == Node_S || inport == Node_Out) {
			if (outport == Node_W) {

			} else if (outport == Node_E || outport == Node_Out) {
				nextPSEState[6] = st;
			} else if (outport == Node_N) {
				nextPSEState[1] = st;
			}
		} else if (inport == Node_E) {
			if (outport == Node_N) {
				nextPSEState[4] = st;
			} else if (outport == Node_S) {

			} else if (outport == Node_W) {
				nextPSEState[0] = st;
			}
		} else if (inport == Node_W) {
			if (outport == Node_S) {
				nextPSEState[5] = st;
			} else if (outport == Node_N) {

			} else if (outport == Node_E || outport == Node_Out) {

				nextPSEState[3] = st;
			}
		} else {
			opp_error("unknown input direction specified");
		}
	} else if (variant == 200) { //matrix crossbar with 18 rings

		if (inport == Node_Out) {

			if (outport == Node_N) {
				nextPSEState[15] = st;
			} else if (outport == Node_S) {
			} else if (outport == Node_E) {
				nextPSEState[17] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[16] = st;
			}
		} else if (inport == Node_N) {
			if (outport == Node_Out) {
				nextPSEState[3] = st;
			} else if (outport == Node_S) {
				nextPSEState[2] = st;
			} else if (outport == Node_E) {
				nextPSEState[1] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[0] = st;
			}
		} else if (inport == Node_S) {
			if (outport == Node_Out) {
				nextPSEState[11] = st;
			} else if (outport == Node_N) {
				nextPSEState[8] = st;
			} else if (outport == Node_E) {
				nextPSEState[10] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[9] = st;
			}
		} else if (inport == Node_E) {
			if (outport == Node_Out) {
			} else if (outport == Node_N) {
				nextPSEState[12] = st;
			} else if (outport == Node_S) {
				nextPSEState[14] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[13] = st;
			}
		} else if (inport == Node_W) {
			if (outport == Node_Out) {
				nextPSEState[7] = st;
			} else if (outport == Node_N) {
				nextPSEState[4] = st;
			} else if (outport == Node_S) {
				nextPSEState[6] = st;
			} else //if(outport == Node_E)
			{
				nextPSEState[5] = st;
			}
		}
	} else if (variant == 201) { //matrix crossbar with 15 rings
		if (inport == Node_Out) {

			if (outport == Node_N) {
				nextPSEState[12] = st;
			} else if (outport == Node_S) {
			} else if (outport == Node_E) {
				nextPSEState[14] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[13] = st;
			}
		} else if (inport == Node_N) {
			if (outport == Node_Out) {
				nextPSEState[3] = st;
			} else if (outport == Node_S) {
				nextPSEState[2] = st;
			} else if (outport == Node_E) {
				nextPSEState[1] = st;
			} else //if(outport == Node_W)
			{
			}
		} else if (inport == Node_S) {
			if (outport == Node_Out) {
				nextPSEState[8] = st;
			} else if (outport == Node_N) {
			} else if (outport == Node_E) {
				nextPSEState[7] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[6] = st;
			}
		} else if (inport == Node_E) {
			if (outport == Node_Out) {
			} else if (outport == Node_N) {
				nextPSEState[9] = st;
			} else if (outport == Node_S) {
				nextPSEState[11] = st;
			} else //if(outport == Node_W)
			{
				nextPSEState[10] = st;
			}
		} else if (inport == Node_W) {
			if (outport == Node_Out) {
				nextPSEState[5] = st;
			} else if (outport == Node_N) {
				nextPSEState[0] = st;
			} else if (outport == Node_S) {
				nextPSEState[4] = st;
			} else //if(outport == Node_E)
			{
			}
		}
	} /*else if (variant == 3) {
	 if (inport == Node_Out) {

	 if (outport == Node_N) {
	 nextPSEState[0] = st;
	 } else if (outport == Node_S) {
	 } else if (outport == Node_E) {
	 nextPSEState[2] = st;
	 } else //if(outport == Node_W)
	 {
	 nextPSEState[1] = st;
	 }
	 } else if (inport == Node_N) {
	 if (outport == Node_Out) {
	 nextPSEState[10] = st;
	 } else if (outport == Node_S) {
	 nextPSEState[12] = st;
	 } else if (outport == Node_E) {
	 nextPSEState[13] = st;
	 } else //if(outport == Node_W)
	 {
	 }
	 } else if (inport == Node_S) {
	 if (outport == Node_Out) {
	 nextPSEState[8] = st;
	 } else if (outport == Node_N) {
	 } else if (outport == Node_E) {
	 nextPSEState[7] = st;
	 } else //if(outport == Node_W)
	 {
	 nextPSEState[5] = st;
	 }
	 } else if (inport == Node_E) {
	 if (outport == Node_Out) {
	 } else if (outport == Node_N) {
	 nextPSEState[3] = st;
	 } else if (outport == Node_S) {
	 nextPSEState[6] = st;
	 } else //if(outport == Node_W)
	 {
	 nextPSEState[4] = st;
	 }
	 } else if (inport == Node_W) {
	 if (outport == Node_Out) {
	 nextPSEState[11] = st;
	 } else if (outport == Node_N) {
	 nextPSEState[14] = st;
	 } else if (outport == Node_S) {
	 nextPSEState[9] = st;
	 } else //if(outport == Node_E)
	 {
	 }
	 }
	 } */else {
		cRuntimeError("variant not implemented");
	}

}

//returns true if the request can be accommodated.
//also, as a side effect, if true, the gate states are updated

// contains special case for photonic Mesh
int Arbiter_PM_Node::pathStatus(ArbiterRequestMsg* rmsg, int outport) {

	if (variant >= 200) {
		return PhotonicArbiter::pathStatus(rmsg, outport);
	}

	// else (variant == 0)

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
					== gateFree && !(outport == Node_E
					&& OutputGateState[Node_Out] != gateFree) && !(outport
					== Node_Out && OutputGateState[Node_E] != gateFree)
					&& !(inport == Node_Out && InputGateState[Node_S]
							!= gateFree) && !(inport == Node_S
					&& InputGateState[Node_Out] != gateFree)) {
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
