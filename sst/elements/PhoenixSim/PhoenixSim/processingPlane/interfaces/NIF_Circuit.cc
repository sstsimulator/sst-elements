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

#include "NIF_Circuit.h"

NIF_Circuit::NIF_Circuit() {
	// TODO Auto-generated constructor stub

}

NIF_Circuit::~NIF_Circuit() {
	// TODO Auto-generated destructor stub
}

void NIF_Circuit::init_packet() {

	dataPortIn = pIn[1];
	dataPortOut = pOut[1];

	dataChannelOut = dataPortOut->findTransmissionChannel();

	pathGood = false;
	waitOnDataArrive = false;

	clockPeriod_data = 1.0 / double(par("O_frequency_data"));

	retry = 0;

	allowedTx = 0;

	SO_lat_blocking = Statistics::registerStat("Blocking latency",
			StatObject::MMA, "NIF");
	SO_lat_setup = Statistics::registerStat("Success setup latency",
			StatObject::MMA, "NIF");

	init_circuit();
}

void NIF_Circuit::netPortMsgArrived(cMessage* msg) {

	if (msg->getArrivalGate() == dataPortIn) {
		dataMsgArrived(msg);
	} else if (msg->getArrivalGate() == packetPortIn) {
		packetPortMsgArrived(msg);
	}
}

simtime_t NIF_Circuit::getRetryTime() {

	int ran = intrand(3);
	double delay = 0;
	if (ran == 0) {
		delay = 0;
	} else if (ran == 1) {
		delay = 10e-9;
	} else if (ran == 2) {
		delay = 100e-9;
	}
	retry++;
	//double delay = fabs(normal(.00000001 * retry, .000000004 * retry));

	debug(getFullPath(), "retrying for ", delay, UNIT_PATH);
	return delay;
}

void NIF_Circuit::packetPortMsgArrived(cMessage* cmsg) {

	ElectronicMessage* emsg = check_and_cast<ElectronicMessage*> (cmsg);
	int type = emsg->getMsgType();
	if (type == router_bufferAck) {
		ackMsgArrived(check_and_cast<BufferAckMsg*> (emsg));
		controller();

	} else {

		unacked.push(emsg->dup());
		ackMsg();

		if (type == dataPacket) {
			sendDataToProcessor(emsg);

		} else if (type == pathBlocked) {

			PathSetupMsg* __attribute__ ((unused)) p = check_and_cast<PathSetupMsg*> (emsg);

			simtime_t ret = getRetryTime();
			blockTime += ((simTime() - setupSent) + ret);
			delete emsg->decapsulate();

			//std::cout << "ret = " << ret << endl;

			sendSetup(currData, clockPeriod_packet + ret);
		} else if (type == pathACK) {

			PathSetupMsg* ps = check_and_cast<PathSetupMsg*> (emsg);
			pathGood = true;
			allowedTx = ps->getAllowedTx();
			allowedWL = min(maxDataWidth, ps->getDataWidth());

			retry = 0;

			debug(getFullPath(), "path successful.. ", UNIT_PATH);
			debug(getFullPath(), "src ", AddressTranslator::untranslate_str(
					(NetworkAddress*) emsg->getSrcId()), UNIT_PATH);
			debug(getFullPath(), "dst ", AddressTranslator::untranslate_str(
					(NetworkAddress*) emsg->getDstId()), UNIT_PATH);

			controller();
		} else if (type == pathSetup) {
			sendDataToProcessor(emsg);

			debug(getFullPath(), "pathSetup received from ",
					Processor::translator->untranslate_pid(
							((NetworkAddress*) emsg->getSrcId())), UNIT_NIC);
		} else if (type == pathTeardown) {
			debug(getFullPath(), "teardown confirmed from ",
					AddressTranslator::untranslate_str(
							(NetworkAddress*) emsg->getSrcId()), UNIT_PATH);

			delete ((NetworkAddress*) emsg->getSrcId());
			delete ((NetworkAddress*) emsg->getDstId());

		} else if (type == grantDataTx) {
			PathSetupMsg* ps = check_and_cast<PathSetupMsg*> (emsg);
			allowedTx = ps->getAllowedTx();

			controller();
		}

	}

}

void NIF_Circuit::sendDataToProcessor(ElectronicMessage* emsg) {

	elMsgs.push(emsg->dup());
	ProcessorData* pdata = (ProcessorData*) emsg->getEncapsulatedPacket();

	debug(getFullPath(), "sending request to send to proc ",
			Processor::translator->untranslate_str(
					((NetworkAddress*) emsg->getDstId())), UNIT_NIC);

	ArbiterRequestMsg* req = new ArbiterRequestMsg();
	req->setStage(10000);
	req->setType(proc_req_send);
	req->setVc(0);
	//req->setDest(pdata->getDestAddr());
	req->setDest(emsg->getDstId());
	req->setReqType(dataPacket);
	req->setPortIn(concentration);
	req->setSize(1);
	req->setTimestamp(simTime());
	req->setMsgId(pdata->getId());

	sendDelayed(req, 0, procReqOut);
}

void NIF_Circuit::requestMsgArrived(RouterCntrlMsg* rmsg) {

	if (rmsg->getType() == proc_grant) {

		debug(getFullPath(), "processor grant ", UNIT_NIC);

		ElectronicMessage* emsg = elMsgs.front();
		ProcessorData* pdata = (ProcessorData*) emsg->decapsulate();

		if (emsg->getMsgType() == dataPacket) {

			sendDelayed(pdata, 0, toProc);

			delete emsg;

			elMsgs.pop();
		} else if (emsg->getMsgType() == pathSetup) {

			//ack the path, the processor says it's ok to send
			PathSetupMsg *msg = new PathSetupMsg;
			msg->setId(globalMsgId++);
			msg->setMsgType(pathACK);
			NetworkAddress* src = (NetworkAddress*) emsg->getSrcId();
			msg->setSrcId(emsg->getDstId());
			msg->setDstId((long) src);
			msg->setBitLength(CONTROL_PACKET_SIZE);

			msg->setAllowedTx(pdata->getSize());

			msg->setDataWidth(maxDataWidth);

			msg->setVirtualChannel(VirtualChannelControl::control->getVC(
					pathACK, pdata));
			sendPacket(msg, true, msg->getVirtualChannel());

			delete pdata;

			delete emsg;

			elMsgs.pop();
		}

	} else if (rmsg->getType() == proc_req_send) { //from the proc, immediately grant
		RouterCntrlMsg* gr = new RouterCntrlMsg();
		gr->setType(proc_grant);
		sendDelayed(gr, 0, procReqOut);
	}

}

bool NIF_Circuit::sendSetup(ProcessorData* pdata, simtime_t delay) {
	ApplicationData* adata = (ApplicationData*) pdata->getEncapsulatedPacket();

	int packType;
	bool isDataPack = false;
	bool isBcast = false;

	setupSent = simTime();

	if (adata->getType() == DM_readRemote || adata->getType() == DM_readLocal
			|| adata->getType() == SM_read || pdata->getType() == procSynch) {

		packType = dataPacket;
		isDataPack = true;
	} else if (adata->getType() == snoopReq || adata->getType() == snoopResp) {
		packType = dataPacket;
		isDataPack = true;
		if (adata->getType() == snoopReq)
			isBcast = true;
	} else {
		packType = pathSetup;
	}
	int vc = VirtualChannelControl::control->getVC(packType, pdata);

	debug(getFullPath(), "Sending setup", UNIT_NIC);
	//SO_qTime->track(SIMTIME_DBL(simTime() + msgDuration
	//		- pdata->getNifArriveTime()));

	PathSetupMsg * msg = new PathSetupMsg();

	msg->setId(globalMsgId++);
	msg->setMsgType(packType);
	msg->setSrcId(pdata->getSrcAddr());
	msg->setBitLength(CONTROL_PACKET_SIZE);
	msg->setDstId(pdata->getDestAddr());
	msg->setVirtualChannel(vc);
	pdata->setBitLength(0);
	msg->encapsulate(pdata->dup());
	msg->setBcast(isBcast);

	if (isDataPack) {

		//send unblock
		XbarMsg *unblock = new XbarMsg();
		unblock->setType(router_arb_unblock);
		unblock->setToPort(concentration);

		sendDelayed(unblock, clockPeriod_packet, procReqOut);

		delete pdata;
		currData = NULL;

		waitOnDataArrive = (adata->getType() != procSynch);
	}

	return sendPacket(msg, true, vc, delay);

}

