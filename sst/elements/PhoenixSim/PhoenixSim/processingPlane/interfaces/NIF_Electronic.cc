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

#include "NIF_Electronic.h"

Define_Module( NIF_Electronic)
;

NIF_Electronic::NIF_Electronic() {
	// TODO Auto-generated constructor stub

}

NIF_Electronic::~NIF_Electronic() {
	// TODO Auto-generated destructor stub
}

void NIF_Electronic::init_packet() {

	currData = NULL;

	maxPacketSize = par("maxPacketSize");

	conc = par("processorConcentration");

	SO_lat_ser = Statistics::registerStat("Serialization latency",
			StatObject::MMA, "NIF");
	SO_lat_prop = Statistics::registerStat("Propagation latency",
			StatObject::MMA, "NIF");

}

void NIF_Electronic::packetPortMsgArrived(cMessage* cmsg) {
	debug(getFullPath(), "packet arrived", UNIT_NIC);
	ElectronicMessage* emsg = check_and_cast<ElectronicMessage*> (cmsg);
	int type = (ElectronicMessageTypes) (emsg->getMsgType());

	if (type == router_bufferAck) {
		ackMsgArrived(check_and_cast<BufferAckMsg*> (emsg));

	} else {
		//if (emsg->getDstId() == myId) {
		unacked.push(emsg->dup());

		if (type == dataPacket) {
			//if (emsg->getComplete()) {
			ProcessorData* pdata = check_and_cast<ProcessorData*> (
					emsg->decapsulate());

			debug(getFullPath(), "pushing processor data to Q", UNIT_NIC);

			if(pdata->getIsComplete()){
				SO_lat_prop->track(SIMTIME_DBL(simTime() - pdata->getSavedTime1()));
			}
			sendDataToProcessor(pdata);

			//}
		} else {

			opp_error("Unknown electronic message type at NIF_electronic");
		}
		//}


	}

}

bool NIF_Electronic::request() {

	if (currData == NULL && waiting.size() > 0) {
		currData = waiting.front();
		waiting.pop_front();

		leftToSend = currData->getSize();

		currVC = VirtualChannelControl::control->getVC(dataPacket, currData);

		SO_qTime->track(SIMTIME_DBL(simTime() - currData->getNifArriveTime()));
		startSerTime = simTime();

	}

	if (!nextMsg->isScheduled() && !completeMsg->isScheduled() && currData
			!= NULL) {
		if (credits[currVC] > 0 && packetQ.size() < 10) {
			return true;
		} else {
			//let the source know we're backed up
			ArbiterRequestMsg* req = new ArbiterRequestMsg();
			req->setType(router_arb_deny);
			sendDelayed(req, 0, procReqOut);
			stalled = true;
		}
	}

	return false;
}

bool NIF_Electronic::prepare() {

	currMsg = new ElectronicMessage();

	bitsSent = min(credits[currVC], min(leftToSend
			+ DATA_HEADER_SIZE, maxPacketSize));
	currMsg->setId(globalMsgId++);
	currMsg->setMsgType(dataPacket);

	ProcessorData* sending;
	sending = currData->dup();

	bool comp = (bitsSent >= leftToSend + DATA_HEADER_SIZE) && currData->getIsComplete();
	sending->setIsComplete(comp);
	//currData->setIsComplete(true);

	sending->setSavedTime1(simTime());

	if(comp){
		SO_lat_ser->track(SIMTIME_DBL(simTime() - startSerTime));
	}

	sending->setSize(bitsSent - DATA_HEADER_SIZE);
	currMsg->setSrcId(sending->getSrcAddr());
	currMsg->setDstId(sending->getDestAddr());

	currMsg->encapsulate(sending);

	currMsg->setBitLength(bitsSent);

	currMsg->setVirtualChannel(currVC);

	if(sending->getType() == snoopReq){
		currMsg->setBcast(true);
	}

	return true;

}

simtime_t NIF_Electronic::send() {

	sendPacket(currMsg, true, currVC);

	SO_bw->track(bitsSent / 1e9);

	simtime_t ret = bitsSent * clockPeriod_packet / flitWidth;

	leftToSend -= (bitsSent - DATA_HEADER_SIZE);


	return ret;

}

void NIF_Electronic::complete() {

	if (leftToSend <= 0) {
		RouterCntrlMsg* ack = new RouterCntrlMsg();
		ack->setType(proc_msg_sent);

		NetworkAddress* adr = (NetworkAddress*) currData->getSrcAddr();
		int p = adr->id[AddressTranslator::convertLevel("PROC")];
		ack->setMsgId(p);
		ack->setData((long) ((ApplicationData*) currData->decapsulate()));
		sendDelayed(ack, 0, procReqOut);

		delete currData;

		currData = NULL;
	}

	return;
}

