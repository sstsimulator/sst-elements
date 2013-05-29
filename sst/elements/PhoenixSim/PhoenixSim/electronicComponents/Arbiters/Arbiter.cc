/*
 * Arbiter.cc
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#include "Arbiter.h"
#include "statistics.h"

AddressTranslator* Arbiter::translator;

Arbiter::Arbiter() {

}

Arbiter::~Arbiter() {
	for (int i = 0; i < numVC; i++) {
		delete reqs[i];
	}

	for (int i = 0; i < numPorts; i++) {
		delete credits[i];
	}

	delete myAddr;
}

void Arbiter::init(string id, int lev, int x, int y, int numV, int numP,
		int numpse, int buff_size, string n) {

	myAddr = new NetworkAddress(id, translator->numLevels);

	level = lev;

	numX = x;
	numY = y;

	myX = myAddr->id[level] % numX;
	myY = myAddr->id[level] / numX;

	numPorts = numP;
	numVC = numV;
	numPSE = numpse;

	currVC = 0;

	name = n;

	for (int i = 0; i < numVC; i++) {
		reqs[i] = new list<ArbiterRequestMsg*> ();
	}

	for (int i = 0; i < numPorts; i++) {
		blockedIn[i] = false;
		blockedOut[i] = false;
		xbar[i] = -1;
		xbarIn[i] = -1;

		credits[i] = new map<int, int> ();
		for (int j = 0; j < numVC; j++) {
			(*credits[i])[j] = buff_size;
		}
	}

	stalled = false;

}

int Arbiter::routeLevel(ArbiterRequestMsg* rmsg) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();

	for (int i = 0; i < translator->numLevels; i++) {

		if (myAddr->id[i] != addr->id[i]) {
			if (i < level) {
				return getUpPort(rmsg, i);
			} else if (i == level) {
				return route(rmsg);
			} else if (i > level) {
				return getDownPort(rmsg, i);
			}
		}

	}

	return route(rmsg);

}

void Arbiter::newRequest(ArbiterRequestMsg* msg, int port) {
	int vc = msg->getVc();
	msg->setNewVC(vc);

	reqs[vc]->push_back(msg);

}

list<RouterCntrlMsg*>* Arbiter::grant(double time) {

	int vc = VirtualChannelControl::control->nextVC(&reqs, currVC);
	currVC = vc;

	list<ArbiterRequestMsg*>::iterator iter;

	list<ArbiterRequestMsg*>* lst = reqs[vc];

	map<int, int> requests;
	map<int, int> grants;

	for (int i = 0; i < numPorts; i++) {
		requests[i] = 0;
		grants[i] = NA;
	}

	list<RouterCntrlMsg*> * msgs = new list<RouterCntrlMsg*> ();

	int numGrants = 0;

	for (iter = lst->begin(); iter != lst->end() && numGrants < maxGrants;) {
		int inport = (*iter)->getPortIn();
		//int reqType = (*iter)->getReqType();

		int outport;
		int path;
		list<RouterCntrlMsg*> *tempLst;

		if ((*iter)->getBcast()) {

			outport = getBcastOutport(*iter);
			if (outport < 0) {
				if (numGrants == 0) {

					RouterCntrlMsg *grant = (*iter)->dup();
					grant->setType(router_grant_destroy);
					grant->setData(1);
					msgs->push_back(grant);
					delete *iter;
					iter = lst->erase(iter);
					goto exit;

				} else {

					goto exit;
				}

			}
		} else {
			debug(name, "getting Outport... ", UNIT_ROUTER);
			outport = getOutport(*iter);

		}
		requests[outport] = requests[outport] | ((int) inport);

		debug(name, "inport ... ", inport, UNIT_ROUTER);
		debug(name, "outport ... ", outport, UNIT_ROUTER);

		debug(name, "destination: ", translator->untranslate_str(
				((NetworkAddress*) (*iter)->getDest())), UNIT_ROUTER);
		debug(name, "myAddr: ", translator->untranslate_str(myAddr),
				UNIT_ROUTER);

		path = pathStatus(*iter, outport);

		if (isPathGood(path)) {
			debug(name, "...good", UNIT_ROUTER);

			tempLst = setupPath(*iter, outport);
			numGrants++;
			grants[outport] = inport;

			if (requestComplete(*iter)) {
				if (tempLst->size() > 0) {
					RouterCntrlMsg* r = tempLst->front();
					r->setData(1);
				}
				/*done:*/ delete *iter;
				iter = lst->erase(iter);
			} else {
				if (tempLst->size() > 0) {
					RouterCntrlMsg* r = tempLst->front();
					r->setData(0);
				}
			}

			msgs->insert(msgs->begin(), tempLst->begin(), tempLst->end());
			tempLst->clear();
			delete tempLst;

		} else {
			pathNotSetup(*iter, path);
			iter++;
		}
	}

	exit:
#ifdef ENABLE_ORION
	for (int i = 0; i < numPorts; i++) {
		power[i]->record(requests[i], grants[i]);
	}

#endif

	//cleanup(numGrants);


	return msgs;
}

int Arbiter::getOutport(ArbiterRequestMsg* rmsg) {

	return routeLevel(rmsg);
}

int Arbiter::getBcastOutport(ArbiterRequestMsg* bmsg) {

	opp_error("This kind of arbiter does not support broadcasting.");

	return -1;
}

bool Arbiter::requestComplete(ArbiterRequestMsg* rmsg) {

	rmsg->setStage(rmsg->getStage() + 1);

	bool ret = rmsg->getStage() >= numPorts;

	if (ret)
		rmsg->setBcast(false);

	return ret;
}

void Arbiter::cleanup(int numG) {

	if (numG == 0)
		stalled = true;
}

bool Arbiter::isPathGood(int p) {
	return p == GO_OK;
}

list<RouterCntrlMsg*>* Arbiter::setupPath(ArbiterRequestMsg* rmsg, int outport) {

	int vc = rmsg->getNewVC();
	(*credits[outport])[vc] -= rmsg->getSize();
	blockedOut[outport] = true;
	blockedIn[rmsg->getPortIn()] = true;
	xbar[outport] = rmsg->getPortIn();
	xbarIn[rmsg->getPortIn()] = outport;

	list<RouterCntrlMsg*> *ret = new list<RouterCntrlMsg*> ();

	RouterCntrlMsg *grant = rmsg->dup();
	grant->setType(router_arb_grant);
	ret->push_back(grant);

	ElementControlMessage* xbarmsg = new ElementControlMessage();
	xbarmsg->setType(router_xbar);
	xbarmsg->setState(outport);
	xbarmsg->setPSEid(rmsg->getPortIn());
	ret->push_back(xbarmsg);

	return ret;

}

void Arbiter::creditsUp(int port, int vc, int size) {

	(*credits[port])[vc] += size;

	debug(name, "credits up ", (*credits[port])[vc], UNIT_BUFFER);

	stalled = false;
}

void Arbiter::unblock(int outport) {
	int inport = xbar[outport];
	if (!blockedOut[outport] || !blockedIn[inport])
		return;
	//opp_error(
	//		"trying to unblock ports that weren't blocked. something is wrong.");

	debug(name, "port unblock to port ", outport, UNIT_XBAR);

	blockedOut[outport] = false;
	blockedIn[inport] = false;

	stalled = false;
}

void Arbiter::setPowerModels(map<int, ORION_Arbiter*> p) {
	power = p;
}

bool Arbiter::serveAgain() {
	if (stalled) {
		return false;
	}

	for (int i = 0; i < numVC; i++) {
		list<ArbiterRequestMsg*>* lst = reqs[i];
		//if (i != currVC && !lst->empty()) {
		if (!lst->empty()) {
			return true;
		}
	}
	return false;
}
