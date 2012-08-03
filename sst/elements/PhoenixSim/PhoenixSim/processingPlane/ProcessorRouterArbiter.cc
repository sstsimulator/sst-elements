/*
 * RouterArbiter.cc
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#include "ProcessorRouterArbiter.h"

Define_Module(ProcessorRouterArbiter)
;

ProcessorRouterArbiter::ProcessorRouterArbiter() {
	served = 0;
}

void ProcessorRouterArbiter::handleMessage(cMessage *msg) {

	arbiter->maxGrants = 4;

	if (msg == clk) {
		served++;

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

				case router_arb_grant: {
					ArbiterRequestMsg* grant = check_and_cast<
							ArbiterRequestMsg*> (*iter);
					grant->setType(proc_grant);
					//no credits in processor router
					arbiter->creditsUp(arbiter->xbarIn[grant->getPortIn()], 0,
							1);
					send(*iter, reqOut[grant->getPortIn()]);
					break;
				}

				default:
					opp_error("Unexpected message type generated from arbiter.");

				}
			}

			numGrant++;
			grant->clear();
			delete grant;

		} else {
			list<ArbiterRequestMsg*> *l = arbiter->reqs[0];
			ArbiterRequestMsg* m = l->front();
			l->pop_front();
			l->push_back(m);

		}

		if (arbiter->serveAgain() && served < 2) {
			ServeClock();
		}

	} else {
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
			served = 0;
			ServeClock();
			return;
		}

		for (int i = 0; i < numPorts; i++) {
			if (msg->arrivedOn(reqIn[i]->getId())) {
				RouterCntrlMsg* req = check_and_cast<RouterCntrlMsg*> (msg);

				if (req->getType() == proc_req_send) {
					debug(getFullPath(), "arbiter request port ", i,
							UNIT_ROUTER);
					ArbiterRequestMsg *req =
							check_and_cast<ArbiterRequestMsg*> (msg);

					req->setPortIn(i);
					req->setType(router_arb_req);
					arbiter->newRequest(req, i);
				} else if (req->getType() == proc_msg_sent) {

					sendDelayed(req, clockPeriod, reqOut[req->getMsgId()]);
				} else if (req->getType() == router_arb_unblock) {
					XbarMsg* unb = check_and_cast<XbarMsg*> (msg);

					debug(getFullPath(), "port unblock to port ",
							unb->getToPort(), UNIT_XBAR);

					arbiter->unblock(unb->getToPort());
					delete msg;

				} else {
					opp_error(
							"ERROR: non request arrived at arbiter's req port");
				}
				served = 0;
				ServeClock();

				break;
			}
		}

	}

}

