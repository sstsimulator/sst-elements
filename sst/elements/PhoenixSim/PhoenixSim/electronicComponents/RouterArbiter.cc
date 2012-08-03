/*
 * RouterArbiter.cc
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#include "RouterArbiter.h"
#include "statistics.h"

#include "Arbiter_PR.h"

#include "Arbiter_EM.h"
#include "Arbiter_CM.h"
#include "Arbiter_FB.h"
#include "Arbiter_ET.h"
#include "Arbiter_EHS.h"

#include "Arbiter_GWC.h"

//#include "Arbiter_SR4x4.h"
//#include "Arbiter_SR8x8.h"
//#include "Arbiter_SR16x16.h"
#include "Arbiter_SR_Quad.h"
#include "Arbiter_SR_Top.h"
#include "Arbiter_SR_Middle.h"

#include "Arbiter_BT_GWS.h"
#include "Arbiter_BT_INJ.h"
#include "Arbiter_BT_EJ.h"
#include "Arbiter_BT_4x4.h"
#include "Arbiter_FT.h"
#include "Arbiter_FT_Root.h"
#include "Arbiter_FT_GWC.h"
#include "Arbiter_NBT_Gateway.h"
#include "Arbiter_NBT_Node.h"

#include "Arbiter_XB_Gateway.h"
#include "Arbiter_XB_Node.h"

#include "Arbiter_TNX_Gateway.h"
#include "Arbiter_TNX_Node.h"

#include "Arbiter_PM_Node.h"

#include "Arbiter_EM_circuit.h"

#include "Arbiter_Crossbar.h"

#include "Arbiter_MITUCB_MeshNode.h"
#include "Arbiter_MITUCB_MemoryNode.h"

#include "Arbiter_PC.h"

#include "Arbiter_NBSiN.h"

#include "AddressTranslator_Standard.h"
#include "AddressTranslator_SquareRoot.h"
#include "AddressTranslator_FatTree.h"

Define_Module(RouterArbiter)
;

#ifdef ENABLE_ORION
double RouterArbiter::totalEnergy = 0;
int RouterArbiter::numRouters = 0;
#endif

AddressTranslator* RouterArbiter::translator = NULL;

RouterArbiter::RouterArbiter() {

}

RouterArbiter::~RouterArbiter() {

	cancelAndDelete(clk);
}

//this is the req_width, assuming no central buffer
//in_length = n_out * data_width * CrsbarCellWidth;

int RouterArbiter::numInitStages() const {
	return 3;
}

void RouterArbiter::initialize(int stage) {

	if (stage < 2) {
		return;
	}

	numPorts = par("numPorts");
	numVC = par("routerVirtualChannels");
	numPSE = par("numPSE");
	type = par("type");
	flit_width = par("electronicChannelWidth");
	buffer_size = par("routerBufferSize");
	myId = par("id").stringValue();

	numX = par("numX");
	numY = par("numY");

	if (translator == NULL) {
		string trans_str = par("addressTranslator");

		if (trans_str.compare("standard") == 0) {
			translator = new AddressTranslator_Standard();
		} else if (trans_str.compare("square_root") == 0) {
			translator = new AddressTranslator_SquareRoot();
		} else if (trans_str.compare("fat_tree") == 0) {
			translator = new AddressTranslator_FatTree();
		} else {
			opp_error("Invalid address translator in RouterArbiter.");
		}

		Arbiter::translator = translator;

	}

	level = translator->convertLevel(par("level"));

	clockPeriod = 1.0 / (double) par("clockRate");

	XbarCntrlOut = gate("XbarCntrl$o");
	XbarCntrlIn = gate("XbarCntrl$i");

	clk = new cMessage();

	totalEnergy = 0;

	for (int i = 0; i < numPorts; i++) {
		reqIn[i] = gate("req$i", i);
		reqOut[i] = gate("req$o", i);
		ackIn[i] = gate("ackIn", i);
	}

	for (int i = 0; i < numPSE; i++) {
		pseCntrl[i] = gate("pseCntrl", i);

	}

	arbiter = NULL;

	int switchVar = par("switchVariant");

	switch (type) {

	case ARB_PR:
		arbiter = new Arbiter_PR();
		break;
	case ARB_EM:
		arbiter = new Arbiter_EM();
		break;
	case ARB_ET:
		arbiter = new Arbiter_ET();
		break;
	case ARB_EHS:
		arbiter = new Arbiter_EHS();
		break;
	case ARB_FB:
		arbiter = new Arbiter_FB();
		break;
	case ARB_CM:
		arbiter = new Arbiter_CM();
		break;

	case ARB_GWC:
		arbiter = new Arbiter_GWC();
		break;

		//-------------BT----------------
	case ARB_BT_INJ:
		arbiter = new Arbiter_BT_INJ();
		break;
	case ARB_BT_EJ:
		arbiter = new Arbiter_BT_EJ();
		break;
	case ARB_BT_GWS:
		arbiter = new Arbiter_BT_GWS();
		break;
	case ARB_BT_4x4:
		arbiter = new Arbiter_BT_4x4();
		break;

		//------------------------------
		/*case ARB_SQ4x4:
		 arbiter = new Arbiter_SR4x4(switchVar);
		 break;
		 case ARB_SQ8x8:
		 arbiter = new Arbiter_SR8x8(switchVar);
		 break;
		 case ARB_SQ16x16:
		 arbiter = new Arbiter_SR16x16(switchVar);
		 break;*/

	case ARB_SR_QUAD:
		arbiter = new Arbiter_SR_Quad();
		break;
	case ARB_SR_TOP:
		arbiter = new Arbiter_SR_Top();
		break;
	case ARB_SR_MID:
		arbiter = new Arbiter_SR_Middle();
		break;

	case ARB_FT:
		arbiter = new Arbiter_FT();
		break;
	case ARB_FT_Root:
		arbiter = new Arbiter_FT_Root();
		break;
	case ARB_FT_GWC:
		arbiter = new Arbiter_FT_GWC();
		break;

		//----------------NBT---------------
	case ARB_NBT_Gateway:
		arbiter = new Arbiter_NBT_Gateway();
		break;
	case ARB_NBT_Node:
		arbiter = new Arbiter_NBT_Node();
		break;
	case ARB_NBT_Gateway_Conc:
		arbiter = new Arbiter_NBT_Gateway();
		break;
	case ARB_NBT_Node_Conc:
		arbiter = new Arbiter_NBT_Node();
		break;

		//------------------XB---------
	case ARB_XB_Gateway:
		arbiter = new Arbiter_XB_Gateway();
		break;
	case ARB_XB_Node:
		arbiter = new Arbiter_XB_Node();
		break;

	case ARB_TNX_Gateway:
		arbiter = new Arbiter_TNX_Gateway();
		break;
	case ARB_TNX_Node:
		arbiter = new Arbiter_TNX_Node();
		break;
	case ARB_PM_Node:
		arbiter = new Arbiter_PM_Node();
		break;
	case ARB_EM_CC:
		arbiter = new Arbiter_EM_Circuit();
		break;
	case ARB_CrossbarSwitch:
		arbiter = new Arbiter_Crossbar();
		break;
	case ARB_MITUCB_MeshNode:
		arbiter = new Arbiter_MITUCB_MeshNode();
		break;
	case ARB_MITUCB_MemoryNode:
		arbiter = new Arbiter_MITUCB_MemoryNode();
		break;

	case ARB_MIT_PhotonicClos:
		arbiter = new Arbiter_PC();
		break;

	case ARB_SiN_Node:
		arbiter = new Arbiter_NBSiN();
		break;

	default:
		opp_error("ERROR: unknown arbiter type.");
	}

	if (arbiter != NULL) {
		arbiter->init(myId, level, numX, numY, numVC, numPorts, numPSE,
				buffer_size, getFullPath());
		arbiter->maxGrants = par("routerMaxGrants");
		arbiter->variant = switchVar;

	}



#ifdef ENABLE_ORION

	//output arbiter


	int model = RR_ARBITER;
	int ff_model = NEG_DFF;
	int req_width = (numPorts - 1) * numVC;
	double length = numPorts * flit_width * Statistics::DEFAULT_ORION_CONFIG_CNTRL->PARM("CrsbarCellWidth");

	for (int i = 0; i < numPorts; i++) {
		power_out[i] = new ORION_Arbiter();

		power_out[i]->init(model, ff_model, req_width, length, Statistics::DEFAULT_ORION_CONFIG_CNTRL);

	}
	arbiter->setPowerModels(power_out);

	RouterCntrlMsg *msg = new RouterCntrlMsg();
	msg->setType(router_power_arbiter);
	msg->setData((long) power_out[0]);
	sendDirect(msg, getParentModule()->getSubmodule("stat"), "inGate");

	//Statistics::me->registerPart();

	string statName = "Electronic Arbiter";
	P_static = Statistics::registerStat(statName, StatObject::ENERGY_STATIC,
			"electronic");
	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	for (int i = 0; i < numPorts; i++) {
		P_static->track(power_out[i]->report_static_power());
	}

	StatObject *E_count = Statistics::registerStat("Electronic Arbiter",
			StatObject::COUNT, "electronic");

#endif

	numGrant = 0;

	numRouters++;

}

void RouterArbiter::finish() {

#ifdef ENABLE_ORION

	//for now, use statistical switching for crossbar, should save us some computation.
	//if we every have real data, go back to using record() and report()

	for (int i = 0; i < numPorts; i++) {
		delete power_out[i];
	}
#endif

	if (arbiter != NULL)
		delete arbiter;

	if (translator != NULL) {
		delete translator;
		translator = NULL;
	}

}

void RouterArbiter::ServeClock() {
	if (!clk->isScheduled()) {
		scheduleAt(clockPeriod + simTime(), clk);
	}
}

void RouterArbiter::handleMessage(cMessage *msg) {

	if (msg == clk) {
		double e = 0;
		for (int i = 0; i < numPorts; i++) {
			e += power_out[i]->report();
		}
		list<RouterCntrlMsg*>* grant = arbiter->grant(SIMTIME_DBL(simTime()));
		double e2 = 0;
		for (int i = 0; i < numPorts; i++) {
			e2 += power_out[i]->report();
		}

		if (e2 > e) {
			E_dynamic->track(e2 - e);
		}

		if (grant != NULL) {
			list<RouterCntrlMsg*>::iterator iter;

			for (iter = grant->begin(); iter != grant->end(); iter++) {
				switch ((*iter)->getType()) {
				case router_xbar:
					send(*iter, XbarCntrlOut);
					break;
				case router_pse_setup: {
					ElementControlMessage* pse = check_and_cast<
							ElementControlMessage*> (*iter);
					debug(getFullPath(), "sending pse message to ",
							pse->getPSEid(), UNIT_PATH_SETUP);
					send(pse, pseCntrl[pse->getPSEid()]);
					break;
				}
				case router_change_cancel:
				case router_change_blocked: {
					ArbiterRequestMsg* grant = check_and_cast<
							ArbiterRequestMsg*> (*iter);
					send(*iter, reqOut[grant->getPortIn()]);
					break;
				}
				case router_grant_destroy:
				case router_arb_grant: {
					ArbiterRequestMsg* grant = check_and_cast<
							ArbiterRequestMsg*> (*iter);
					send(*iter, reqOut[grant->getPortIn()]);
					break;
				}

				}
			}

			numGrant++;
			grant->clear();
			delete grant;

		}

		if (arbiter->serveAgain()) {
			ServeClock();
		}

	} else {
		for (int i = 0; i < numPorts; i++) {
			if (msg->arrivedOn(reqIn[i]->getId())) {
				RouterCntrlMsg* req = check_and_cast<RouterCntrlMsg*> (msg);

				if (req->getType() == router_bufferAck) {
					debug(getFullPath(), "buffer ack rcvd, vc ", req->getVc(),
							UNIT_BUFFER);
					debug(getFullPath(), "... port ", i, UNIT_BUFFER);
					debug(getFullPath(), "... size ", req->getData(),
							UNIT_BUFFER);
					arbiter->creditsUp(i, req->getVc(), req->getData());

					delete msg;
				} else if (req->getType() == router_arb_req) {
					debug(getFullPath(), "arbiter request port ", i,
							UNIT_ROUTER);
					ArbiterRequestMsg *req =
							check_and_cast<ArbiterRequestMsg*> (msg);

					req->setPortIn(i);
					arbiter->newRequest(req, i);
				} else if (req->getType() == router_arb_unblock) {
					XbarMsg* unb = check_and_cast<XbarMsg*> (msg);

					debug(getFullPath(), "port unblock from port ",
							unb->getFromPort(), UNIT_XBAR);

					arbiter->unblock(unb->getToPort());
					delete msg;

				} else {
					opp_error(
							"ERROR: non request arrived at arbiter's req port");
				}
				ServeClock();

				break;
			}
		}

		if (msg->arrivedOn(XbarCntrlIn->getId())) {
			RouterCntrlMsg* req = check_and_cast<RouterCntrlMsg*> (msg);

			if (req->getType() == router_arb_unblock) {
				XbarMsg* unb = check_and_cast<XbarMsg*> (msg);
				debug(getFullPath(), "port unblock outport ", unb->getToPort(),
						UNIT_ROUTER);
				arbiter->unblock(unb->getToPort());
				delete msg;
			} else {
				opp_error("ERROR: non unblock arrived at xbarcntrl port");
			}
			ServeClock();

		}
	}

}

