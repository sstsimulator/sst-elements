/*
 * RouterInport.cc
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#include "RouterInport.h"
#include "statistics.h"

#include "VirtualChannelControl.h"

Define_Module( RouterInport)
;

RouterInport::RouterInport() {

}

RouterInport::~RouterInport() {

}

void RouterInport::initialize() {

	numVC = par("routerVirtualChannels");
	flit_width = par("electronicChannelWidth");
	buffer_size = par("routerBufferSize");

	//packetTimeout = par("packetTimeout");

	clockPeriod = 1.0 / (double) par("clockRate");

	myId = par("myId");

	inport = gate("in");
	inport->setDeliverOnReceptionStart(true);
	outport = gate("toXbar");
	reqportIn = gate("req$i");
	reqportOut = gate("req$o");
	ackport = gate("ackOut");

	for (int i = 0; i < numVC; i++) {
		buffs[i] = new queue<ElectronicMessage*> ();
		//n_read[i] = 0;
		//n_write[i] = 0;

		timeouts[i] = new cMessage();
	}

	lastTime = 0;

#ifdef ENABLE_ORION
	info = new ORION_Array_Info();

	int buf_type = SRAM;
	int is_fifo = true;
	int n_read_port = 1;
	int n_write_port = 1;
	int n_entry = buffer_size / flit_width;
	int line_width = flit_width;
	int outdrv = true;

	info->init(buf_type, is_fifo, n_read_port, n_write_port, n_entry,
			line_width, outdrv, Statistics::DEFAULT_ORION_CONFIG_CNTRL);

	power = new ORION_Array();
	power->init(info);

	RouterCntrlMsg *msg = new RouterCntrlMsg();
	msg->setType(router_power_inport);
	msg->setData((long) power);
	sendDirect(msg, getParentModule()->getSubmodule("stat"), "inGate");

	//numRouters++;
	//Statistics::me->registerPart();

	string statName = "Electronic Inport";
	P_static = Statistics::registerStat(statName, StatObject::ENERGY_STATIC,
			"electronic");
	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	P_static->track(numVC * power->stat_static_power());

	Statistics::registerStat("Electronic Inport", StatObject::COUNT,
			"electronic");

#endif

}

void RouterInport::finish() {

	for (int i = 0; i < numVC; i++) {
		delete buffs[i];

		delete timeouts[i];
	}
#ifdef ENABLE_ORION

	delete info;

	delete power;

#endif
}

void RouterInport::handleMessage(cMessage *msg)
{

	if (msg->arrivedOn(reqportIn->getId()))
	{ //grant from arbiter
		RouterCntrlMsg *grant = check_and_cast<RouterCntrlMsg*> (msg);
		int vc = grant->getVc();

		if (grant->getType() == router_arb_grant ||
				grant->getType() == router_change_blocked ||
				grant->getType() == router_change_cancel ||
				grant->getType() == router_grant_destroy)
		{

			debug(getFullPath(), "inport arbiter grant", UNIT_ROUTER);

			ElectronicMessage* emsg = buffs[vc]->front();

			if (emsg == NULL)
				opp_error("NULL message coming off the inport queue");
			else if (emsg->getId() != grant->getMsgId())
			{

				std::cout << "msg id: " << emsg->getId() << endl;
				opp_error("inport: message id's don't match");
			}

			if (grant->getType() == router_change_blocked)
			{
				debug(getFullPath(), "message changed to pathBlocked",
						UNIT_ROUTER);
				NetworkAddress* srcId = (NetworkAddress*) emsg->getSrcId();
				// int srcType = emsg->getSrcType();
				emsg->setSrcId(emsg->getDstId());
				//emsg->setSrcType(emsg->getDstType());
				emsg->setDstId((long) srcId);
				// emsg->setDstType(srcType);
				emsg->setMsgType(pathBlocked);

				PathSetupMsg* p = check_and_cast<PathSetupMsg*> (emsg);
				p->setBlockedAddr(grant->getData());

				buffs[vc]->pop();

				//ack back
				BufferAckMsg* ack = new BufferAckMsg();
				ack->setMsgType(router_bufferAck);
				ack->setData(emsg->getBitLength());
				ack->setVirtualChannel(vc);
				ack->setBitLength(emsg->getBitLength() / flit_width);
				ack->setKind(2);
				send(ack, ackport);

				debug(getFullPath(), "sending buffer ack ", UNIT_BUFFER);

				//n_read[vc] += emsg->getBitLength() / flit_width;

#ifdef ENABLE_ORION
				double e = power->stat_energy(
						emsg->getBitLength() / flit_width, 0, 0);
				lastTime = simTime();

				E_dynamic->track(e);
#endif

				emsg->setVirtualChannel(grant->getNewVC());

				sendDelayed(emsg, clockPeriod, outport);


				if (!buffs[vc]->empty())
				{
					sendRequest(buffs[vc]->front(), vc);
				}

			}
			else if (grant->getType() == router_change_cancel)
			{

				debug(getFullPath(), "message changed to pathSetupCancelled",
						UNIT_ROUTER);
				emsg->setDstId(emsg->getSrcId());
				// emsg->setDstType(emsg->getSrcType());
				emsg->setMsgType(pathSetupCancelled);
			}
			else if (grant->getData() == 0)
			{

				ElectronicMessage* newmsg = emsg->dup();
				newmsg->setVirtualChannel(grant->getNewVC());

				NetworkAddress* srcId = (NetworkAddress*) emsg->getSrcId();
				NetworkAddress* destId = (NetworkAddress*) emsg->getDstId();

				ProcessorData* pdata = (ProcessorData*) newmsg->decapsulate();

				pdata->setDestAddr((long) destId->dup());
				pdata->setSrcAddr((long) srcId->dup());

				newmsg->setDstId(pdata->getDestAddr());
				newmsg->setSrcId(pdata->getSrcAddr());

				newmsg->encapsulate(pdata);

				//forward message
				sendDelayed(newmsg, clockPeriod, outport);

#ifdef ENABLE_ORION
				double e = power->stat_energy(newmsg->getBitLength()
						/ flit_width, 0, 0);
				lastTime = simTime();

				E_dynamic->track(e);
#endif
			}
			else
			{

				buffs[vc]->pop();

				//ack back
				BufferAckMsg* ack = new BufferAckMsg();
				ack->setMsgType(router_bufferAck);
				ack->setData(emsg->getBitLength());
				ack->setVirtualChannel(vc);
				ack->setBitLength(emsg->getBitLength() / flit_width);
				ack->setKind(2);
				send(ack, ackport);

				debug(getFullPath(), "sending buffer ack ", UNIT_BUFFER);

				//n_read[vc] += emsg->getBitLength() / flit_width;

#ifdef ENABLE_ORION
				double e = power->stat_energy(
						emsg->getBitLength() / flit_width, 0, 0);
				lastTime = simTime();

				E_dynamic->track(e);
#endif

				if (grant->getType() != router_grant_destroy)
				{
					emsg->setVirtualChannel(grant->getNewVC());

					//forward message
					sendDelayed(emsg, clockPeriod, outport);
				}
				else
				{
					delete emsg;
				}

				if (!buffs[vc]->empty())
				{
					sendRequest(buffs[vc]->front(), vc);
				}
			}
		}
		else
		{
			opp_error(
					"ERROR: unexpected message received at inport from arbiter");
		}

		delete msg;

	}
	else
	{ //message arrival
		ElectronicMessage *emsg = check_and_cast<ElectronicMessage*> (msg);

		if (emsg->getMsgType() == router_bufferAck)
		{
			RouterCntrlMsg* ack = new RouterCntrlMsg();
			ack->setType(router_bufferAck);
			ack->setData(emsg->getData());
			ack->setVc(emsg->getVirtualChannel());
			send(ack, reqportOut);

			debug(getFullPath(),
					"buffer ack arrived at inport, forward to arbiter. size ",
					emsg->getData(), UNIT_BUFFER);

			delete emsg;
			return;
		}

		debug(getFullPath(), "inport message arrived, size ",
				emsg->getBitLength(), UNIT_BUFFER);

		int vc = emsg->getVirtualChannel();

		buffs[vc]->push(emsg);

		//n_write[vc] += emsg->getBitLength() / flit_width;

#ifdef ENABLE_ORION

		double e = power->stat_energy(0, emsg->getBitLength() / flit_width, 0);
		lastTime = simTime();

		E_dynamic->track(e);
#endif

		lastTime = SIMTIME_DBL(simTime());

		if (buffs[vc]->front() == emsg)
		{
			sendRequest(emsg, vc);
		}
	}
}

void RouterInport::sendRequest(ElectronicMessage* emsg, int vc) {

	ArbiterRequestMsg* req = new ArbiterRequestMsg();

	req->setBcast(emsg->getBcast());

	if (req->getBcast()) {
		req->setStage(0);
	} else {
		req->setStage(10000); //not a broadcast, will always finish on first grant
	}
	req->setType(router_arb_req);
	req->setVc(vc);
	req->setDest(emsg->getDstId());
	req->setSrc(emsg->getSrcId());
	//req->setDestType(emsg->getDstType());
	req->setReqType(emsg->getMsgType());
	req->setPortIn(myId);
	req->setSize(emsg->getBitLength());
	req->setTimestamp(simTime());
	req->setMsgId(emsg->getId());
	req->setData((long) emsg->getEncapsulatedPacket());

	sendDelayed(req, clockPeriod, reqportOut);

}

