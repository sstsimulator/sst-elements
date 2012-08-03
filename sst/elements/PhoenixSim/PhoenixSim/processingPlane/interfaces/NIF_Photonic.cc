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

#include "NIF_Photonic.h"

#include "packetstat.h"

Define_Module( NIF_Photonic)
;

double NIF_Photonic::DetectorSensitivity;
//int NIF_Photonic::numX;
//int NIF_Photonic::numY;


NIF_Photonic::NIF_Photonic() {
	// TODO Auto-generated constructor stub
	//std::cout << "NIF_Photonic reporting for duty" << endl;
}

NIF_Photonic::~NIF_Photonic() {

}

void NIF_Photonic::init_circuit() {

	maxDataWidth = par("numOfWavelengthChannels");
	maxPacketSize = par("maxPacketSize");
	guardTimeGap = par("guardTimeGap");

	NIFselection = par("NIFselection").stringValue();
	NIFthreshold = par("NIFthreshold");

	DetectorSensitivity = par("DetectorSensitivity");

	//numX = par("numOfNodesX");
	//numY = par("numOfNodesY");
	currData = NULL;
	start = NULL;
	stop = NULL;

	SO_lat_trans = Statistics::registerStat("Transmission latency",
			StatObject::MMA, "NIF");

	numOfNodesX = par("numOfNodesX");

}

void NIF_Photonic::dataMsgArrived(cMessage* cmsg) {
	//if (pmsg->getDstId() == myId) {
	PhotonicMessage* pmsg = check_and_cast<PhotonicMessage*> (cmsg);
	if (pmsg->getMsgType() == TXstart) {
		debug(getFullPath(), "TXstart received", UNIT_NIC);

		delete pmsg->decapsulate();

		waitOnDataArrive = false;
		controller();
	} else if (pmsg->getMsgType() == TXstop) {

		debug(getFullPath(), "TXstop received", UNIT_NIC);

		ProcessorData* pdata = check_and_cast<ProcessorData*> (
				pmsg->decapsulate());

		SO_lat_trans->track(SIMTIME_DBL(simTime() - pdata->getSavedTime1()));
		sendDelayed(pdata, 0, this->toProc);

	} else {
		opp_error("optical MsgType not set");
	}
}

void NIF_Photonic::putInPacketQ(ProcessorData* pdata) {
	int bitsleft = pdata->getSize();
	int vc = VirtualChannelControl::control->getVC(dataPacket, pdata);
	while (bitsleft > 0) {
		ElectronicMessage *currMsg = new ElectronicMessage();

		bitsSent = min(bitsleft + DATA_HEADER_SIZE, maxPacketSize);
		currMsg->setId(globalMsgId++);
		currMsg->setMsgType(dataPacket);

		ProcessorData* sending;
		sending = (bitsleft - bitsSent <= 0) ? pdata : pdata->dup();

		bool comp = (bitsSent >= bitsleft + DATA_HEADER_SIZE)
				&& pdata->getIsComplete();
		sending->setIsComplete(comp);
		//currData->setIsComplete(true);

		sending->setSavedTime1(simTime());

		sending->setSize(bitsSent - DATA_HEADER_SIZE);
		currMsg->setSrcId(sending->getSrcAddr());
		currMsg->setDstId(sending->getDestAddr());

		currMsg->encapsulate(sending);

		currMsg->setBitLength(bitsSent);

		currMsg->setVirtualChannel(vc);

		if (sending->getType() == snoopReq) {
			currMsg->setBcast(true);
		}

		sendPacket(currMsg, true, vc);
		bitsleft -= bitsSent;

		SO_bw->track(bitsSent / 1e9);
	}

}

bool NIF_Photonic::request() {
	if (!waitOnDataArrive) {
		if (!nextMsg->isScheduled()) {
			if (currData == NULL && waiting.size() > 0) {
				ProcessorData* pdata = waiting.front();

				bool
						measureLat =
								((ApplicationData*) pdata->getEncapsulatedPacket())->getType()
										== MPI_send;
				currData = pdata;
				enqueueTimeOfSendingMessage = pdata->getNifArriveTime();

				if (NIFselection.compare("size") == 0) {
					if (currData->getSize() <= NIFthreshold) {
						currData = NULL;
						if (packetQ.size() > 0) {
							return false;
						}
						putInPacketQ(pdata);

						//send unblock
						XbarMsg *unblock = new XbarMsg();
						unblock->setType(router_arb_unblock);
						unblock->setToPort(concentration);
						sendDelayed(unblock, 0, procReqOut);

					} else {
						sendSetup(pdata, clockPeriod_packet);

					}
				} else if (NIFselection.compare("distance") == 0) {
					NetworkAddress* adr1 =
							(NetworkAddress*) pdata->getSrcAddr();
					NetworkAddress* adr2 =
							(NetworkAddress*) pdata->getDestAddr();
					int dist = abs((adr1->id[AddressTranslator::convertLevel(
							"NET")] / numOfNodesX)
							- (adr2->id[AddressTranslator::convertLevel("NET")]
									/ numOfNodesX));

					dist
							+= abs(
									(adr1->id[AddressTranslator::convertLevel(
											"NET")] % numOfNodesX)
											- (adr2->id[AddressTranslator::convertLevel(
													"NET")] % numOfNodesX));

					debug(getFullPath(), "dist= ", dist, UNIT_NIC);
					debug(getFullPath(), "threshold= ", NIFthreshold, UNIT_NIC);

					if (dist <= NIFthreshold) {

						currData = NULL;
						if (packetQ.size() > 0) {
							return false;
						}
						putInPacketQ(pdata);

						//send unblock
						XbarMsg *unblock = new XbarMsg();
						unblock->setType(router_arb_unblock);
						unblock->setToPort(concentration);
						sendDelayed(unblock, 0, procReqOut);

					} else {
						sendSetup(pdata, clockPeriod_packet);

					}

				} else if (NIFselection.compare("sizedistance") == 0) {
					NetworkAddress* adr1 =
							(NetworkAddress*) pdata->getSrcAddr();
					NetworkAddress* adr2 =
							(NetworkAddress*) pdata->getDestAddr();
					int dist = abs((adr1->id[AddressTranslator::convertLevel(
							"NET")] / numOfNodesX)
							- (adr2->id[AddressTranslator::convertLevel("NET")]
									/ numOfNodesX));

					dist
							+= abs(
									(adr1->id[AddressTranslator::convertLevel(
											"NET")] % numOfNodesX)
											- (adr2->id[AddressTranslator::convertLevel(
													"NET")] % numOfNodesX));

					debug(getFullPath(), "dist= ", dist, UNIT_NIC);
					debug(getFullPath(), "szedist= ", dist*pdata->getSize(), UNIT_NIC);
					debug(getFullPath(), "threshold= ", NIFthreshold, UNIT_NIC);

					if (dist*pdata->getSize() <= NIFthreshold) {

						currData = NULL;
						if (packetQ.size() > 0) {
							return false;
						}
						putInPacketQ(pdata);

						//send unblock
						XbarMsg *unblock = new XbarMsg();
						unblock->setType(router_arb_unblock);
						unblock->setToPort(concentration);
						sendDelayed(unblock, 0, procReqOut);

					} else {
						sendSetup(pdata, clockPeriod_packet);

					}
				} else {

					sendSetup(pdata, clockPeriod_packet);

				}

				waiting.pop_front();

				blockTime = 0;

				if (measureLat) {
					SO_qTime->track(
							SIMTIME_DBL(simTime() - pdata->getCreationTime()));
					pdata->setSavedTime2(simTime());
				}

				return false;

			} else if (currData != NULL) {
				if (pathGood && allowedTx > 0)
					return true;
				else {
					debug(getFullPath(), "Waiting on pathgood: " , pathGood, UNIT_NIC);
					debug(getFullPath(), "..and allowedTx: ", allowedTx, UNIT_NIC);
					return false;
				}
			}
		}
	} else
		debug(getFullPath(), "Waiting on data to arrive", UNIT_NIC);

	return false;
}

bool NIF_Photonic::prepare() {

	SO_lat_blocking->track(SIMTIME_DBL(blockTime));
	SO_lat_setup->track(SIMTIME_DBL(simTime() - setupSent));

	bitsSent = min(allowedTx, currData->getSize());

	currData->setSavedTime1(simTime());
	ProcessorData* toSend = currData->dup();
	toSend->setDataType(1);

	toSend->setIsComplete(bitsSent == currData->getSize());
	toSend->setSize(bitsSent);

	//TXstart
	start = new PhotonicMessage;

	start->setMsgType(TXstart);

	start->setId(currData->getId());
	start->setBitLength(0);
	start->encapsulate(toSend->dup());

	//TXstop
	stop = new PhotonicMessage;

	stop->setMsgType(TXstop);

	stop->setId(currData->getId());
	stop->setBitLength(bitsSent);

	stop->encapsulate(toSend);

	return true;

}

simtime_t NIF_Photonic::send() {

	sendDelayed(start, 0, dataPortOut);

	msgDuration = ((simtime_t) (bitsSent * clockPeriod_data
			/ (double) maxDataWidth));
	debug(getFullPath(), "msg duration = ", SIMTIME_DBL(msgDuration), UNIT_NIC);

	sendDelayed(stop, msgDuration, dataPortOut);

	SO_bw->track(bitsSent / 1e9);

	if (bitsSent >= currData->getSize()) {
		//notify processor that data was sent
		RouterCntrlMsg* msgSent = new RouterCntrlMsg();
		msgSent->setType(proc_msg_sent);
		NetworkAddress* adr = (NetworkAddress*) currData->getSrcAddr();
		int p = adr->id[AddressTranslator::convertLevel("PROC")];
		msgSent->setMsgId(p);
		msgSent->setData(
				(long) (((ApplicationData*) currData->getEncapsulatedPacket())->dup()));
		sendDelayed(msgSent, 0, procReqOut);
	}

	sourceAddr = ((NetworkAddress*) currData->getSrcAddr())->dup();
	destAddr = ((NetworkAddress*) currData->getDestAddr())->dup();

	return 2 * clockPeriod_packet + guardTimeGap + (msgDuration * (maxDataWidth
			/ allowedWL));

}

void NIF_Photonic::complete() {
	//teardown

	debug(getFullPath(), "sent bits = ", bitsSent, UNIT_NIC);

	if (bitsSent >= currData->getSize()) {

		//send unblock
		XbarMsg *unblock = new XbarMsg();
		unblock->setType(router_arb_unblock);
		unblock->setToPort(concentration);

		sendDelayed(unblock, 0, procReqOut);

		//teardown path
		ElectronicMessage *msg = new ElectronicMessage;
		msg->setId(globalMsgId++);
		msg->setMsgType(pathTeardown);
		msg->setSrcId((long) sourceAddr);
		msg->setBitLength(CONTROL_PACKET_SIZE);
		msg->setDstId((long) destAddr);
		msg->setVirtualChannel(VirtualChannelControl::control->getVC(
				pathTeardown, currData));
		msg->setTime(simTime());

		sendPacket(msg, true, msg->getVirtualChannel());

		delete currData;

		currData = NULL;
		start = NULL;
		stop = NULL;

		pathGood = false;

		debug(getFullPath(), "message sending complete, tearing down.. ",
				UNIT_PATH);
		debug(getFullPath(), "src ", AddressTranslator::untranslate_str(
				(NetworkAddress*) msg->getSrcId()), UNIT_PATH);
		debug(getFullPath(), "dst ", AddressTranslator::untranslate_str(
				(NetworkAddress*) msg->getDstId()), UNIT_PATH);

	} else {
		currData->setSize(currData->getSize() - bitsSent);

		debug(getFullPath(), "requesting that more data be transmitted.. ",
				UNIT_NIC);

		ElectronicMessage *msg = new ElectronicMessage;
		msg->setId(globalMsgId++);
		msg->setMsgType(requestDataTx);
		msg->setSrcId((long) sourceAddr);

		msg->setBitLength(CONTROL_PACKET_SIZE);
		msg->setDstId((long) destAddr);
		msg->setVirtualChannel(VirtualChannelControl::control->getVC(
				requestDataTx, currData));
		msg->setTime(simTime());
		sendPacket(msg, true, msg->getVirtualChannel());
	}

	allowedTx = 0;

	return;
}

