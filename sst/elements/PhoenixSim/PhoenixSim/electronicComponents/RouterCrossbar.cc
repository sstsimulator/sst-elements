/*
 * RouterCrossbar.cc
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#include "RouterCrossbar.h"
#include "statistics.h"

Define_Module( RouterCrossbar)
;

RouterCrossbar::RouterCrossbar() {

}

RouterCrossbar::~RouterCrossbar() {

}

void RouterCrossbar::initialize() {

	numPorts = par("numPorts");
	flit_width = par("electronicChannelWidth");

	//std::cout << getName() << ": flit width = " << flit_width << endl;

	for (int i = 0; i < numPorts; i++) {
		xbar[i] = -1;
		ports[i] = gate("out", i);
		inports[i] = gate("in", i);
		inports[i]->setDeliverOnReceptionStart(true);
	}

	cntrlIn = gate("cntrl$i");
	cntrlOut = gate("cntrl$o");

	clockPeriod = 1.0 / (double) par("clockRate");

	lastEnergy = 0;

#ifdef ENABLE_ORION
	power = new ORION_Crossbar();

	int model = MATRIX_CROSSBAR;
	int ins = numPorts;
	int outs = numPorts;
	int in_seg = 0; //used by segmented crossbar
	int out_seg = 0; //used by segmented crossbar
	int data_width = flit_width;
	int degree = 4; //used by multree crossbar
	int connect_type = TRISTATE_GATE;
	int trans_type = NP_GATE;
	double in_len = 0; //use 0 if unknown, ORION will figure it out
	double out_len = 0;

	ORION_Tech_Config *conf;

	if ((bool) par("isDataPlane")) {
		conf = Statistics::DEFAULT_ORION_CONFIG_DATA;
	} else if ((bool) par("isProcPlane")) {
		conf = Statistics::DEFAULT_ORION_CONFIG_PROC;
	} else {
		conf = Statistics::DEFAULT_ORION_CONFIG_CNTRL;
	}

	power->init(model, ins, outs, in_seg, out_seg, data_width, degree,
			connect_type, trans_type, in_len, out_len, conf);

	if (getParentModule()->getSubmodule("stat") != NULL) {
		RouterCntrlMsg *msg = new RouterCntrlMsg();
		msg->setType(router_power_crossbar);
		msg->setData((long) power);

		sendDirect(msg, getParentModule()->getSubmodule("stat"), "inGate");
	}

	//numRouters++;
	//Statistics::me->registerPart();

	autounblock = par("autounblock");

	string statName = "Electronic Crossbar";
	P_static = Statistics::registerStat(statName, StatObject::ENERGY_STATIC,
			"electronic");
	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	P_static->track(power->report_static_power());

	StatObject *E_count = Statistics::registerStat("Electronic Crossbar",
			StatObject::COUNT, "electronic");

#endif

}

void RouterCrossbar::finish() {

#ifdef ENABLE_ORION

	delete power;

#endif
}

void RouterCrossbar::handleMessage(cMessage *msg) {

	if (msg->isSelfMessage()) {
		XbarMsg* unset = check_and_cast<XbarMsg*> (msg);
		xbar[unset->getFromPort()] = -1;
		delete unset;

	} else if (msg->arrivedOn(cntrlIn->getId())) {
		ElementControlMessage *setup = check_and_cast<ElementControlMessage*> (
				msg);

		int from = setup->getPSEid();
		int to = setup->getState();

		debug(getFullPath(), "setting up crossbar from ", from, UNIT_XBAR);
		debug(getFullPath(), "setting up crossbar to ", to, UNIT_XBAR);

		//if (to >= 0 && xbar[from] != -1) {
		//	opp_error(
		//			"trying to set up a crossbar while message being transmitted");
		//}

		//i'm a dumb xbar, just set it up
		xbar[from] = to;

		delete msg;
	} else {
		cPacket* emsg = check_and_cast<cPacket*> (msg);
		for (int i = 0; i < numPorts; i++) {
			if (msg->arrivedOn(inports[i]->getId())) {
				if (xbar[i] == -1) {
					std::cout << "inport: " << i << endl;
					opp_error("ERROR: Crossbar: path not set up");
				} else {
					debug(getFullPath(), "traversing crossbar from ", i,
							UNIT_XBAR);
					debug(getFullPath(), "traversing crossbar to ", xbar[i],
							UNIT_XBAR);

					power->record(0, i, xbar[i], (int) (emsg->getBitLength()
							/ flit_width)); //uses 0.5 switching probability.  use overloaded function if we have real data
					power->record(1, i, xbar[i], (int) (emsg->getBitLength()
							/ flit_width)); //uses 0.5 switching probability.  use overloaded function if we have real data
					double e = power->report();
					E_dynamic->track(e - lastEnergy);

					lastEnergy = e;

					sendDelayed(msg, clockPeriod, ports[xbar[i]]);

					if (autounblock) {
						//un-set it
						XbarMsg *unset = new XbarMsg();
						unset->setToPort(xbar[i]);
						unset->setFromPort(i);
						scheduleAt(simTime() + clockPeriod * (1
								+ emsg->getBitLength() / flit_width), unset);

						//send unblock
						XbarMsg *unblock = new XbarMsg();
						unblock->setType(router_arb_unblock);
						unblock->setToPort(xbar[i]);
						unblock->setFromPort(i);
						debug(getFullPath(), "blocking for ", clockPeriod * (1
								+ emsg->getBitLength() / flit_width),
								UNIT_ROUTER );
						debug(getFullPath(), "..message size ",
								emsg->getBitLength(), UNIT_ROUTER );
						debug(getFullPath(), "..delay ", clockPeriod * (1
								+ emsg->getBitLength() / flit_width),
								UNIT_ROUTER );

						//sendDelayed(unblock, ports[xbar[i]]->getTransmissionFinishTime() - simTime(), cntrlOut);
						sendDelayed(unblock, clockPeriod * (1
								+ emsg->getBitLength() / flit_width), cntrlOut);
					}

				}
				break;
			}
		}
	}

}

