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

#include "NIF_MemGW.h"

Define_Module( NIF_MemGW);

NIF_MemGW::NIF_MemGW() {
	// TODO Auto-generated constructor stub

}

NIF_MemGW::~NIF_MemGW() {
	// TODO Auto-generated destructor stub
}

void NIF_MemGW::init_circuit() {



	id = par("id");
	myAddr = new NetworkAddress(id, 1, 3);

	currData = NULL;
	cols = par("DRAM_cols");

	chips = par("DRAM_chipsPerDIMM");
	arrays = par("DRAM_arrays");
	DRAM_freq = par("DRAM_freq");
	photonicBitRate = par("O_frequency_data");

	servicingRead = false;

}



void NIF_MemGW::dataMsgArrived(cMessage* pmsg) {
	opp_error("ERROR: photonic message arrived at a MemGW NIF");

}


void NIF_MemGW::requestMsgArrived(RouterCntrlMsg* rmsg) {
	if (rmsg->getType() == router_arb_req) { //do a path setup for a read

		ProcessorData* pdata = (ProcessorData*) rmsg->getData();
		retry = 0;
		sendSetup(pdata, clockPeriod);

		debug(getFullPath(), "setting up path back to processor gateway ",
				AddressTranslator::untranslate_str(
						(NetworkAddress*) pdata->getSrcAddr()), UNIT_DRAM);

		servicingRead = true;

	} else if (rmsg->getType() == router_bufferAck) { //ack the path for a write
		debug(getFullPath(), "ACKing path back to processor gateway", UNIT_DRAM);

		PathSetupMsg* emsg = new PathSetupMsg();

		ProcessorData* pdata = (ProcessorData*) rmsg->getData();

		emsg->setDstId(pdata->getSrcAddr());

		emsg->setId(globalMsgId++);
		emsg->setMsgType(pathACK);

		emsg->setSrcId(pdata->getDestAddr());
		emsg->setBitLength(CONTROL_PACKET_SIZE);
		emsg->setVirtualChannel(VirtualChannelControl::control->getVC(pathACK,
				pdata));
		//emsg->setAllowedTx(pdata->getSize()); //the size of a row, for the data allowed to TX
		emsg->setAllowedTx(cols * arrays * chips);

		emsg->setDataWidth((int) ((DRAM_freq * arrays * chips)
				/ photonicBitRate));

		credits[emsg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

		sendDelayed(emsg, clockPeriod_packet, packetPortOut);
	} else if (rmsg->getType() == router_arb_unblock) { // teardown read path
		debug(getFullPath(), "tearing down path from memory gateway..",
				UNIT_DRAM);

		PathSetupMsg* emsg = new PathSetupMsg();

		ProcessorData* pdata = (ProcessorData*) rmsg->getData();

		emsg->setDstId(pdata->getSrcAddr());

		emsg->setId(globalMsgId++);
		emsg->setMsgType(pathTeardown);
		emsg->setSrcId(pdata->getDestAddr());
		emsg->setBitLength(CONTROL_PACKET_SIZE);
		emsg->setVirtualChannel(VirtualChannelControl::control->getVC(
				pathTeardown, pdata));

		delete pdata;

		debug(getFullPath(), "tearing down path.. ", UNIT_PATH);
		debug(getFullPath(), "src ", AddressTranslator::untranslate_str(
				(NetworkAddress*) emsg->getSrcId()), UNIT_PATH);
		debug(getFullPath(), "dst ", AddressTranslator::untranslate_str(
				(NetworkAddress*) emsg->getDstId()), UNIT_PATH);

		credits[emsg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

		sendDelayed(emsg, clockPeriod_packet, packetPortOut);

		servicingRead = false;
	} else if (rmsg->getType() == router_arb_grant) { //message that says the NIF can transmit more write data
		debug(getFullPath(),
				"telling processor gateway to go ahead and transmit more",
				UNIT_DRAM);

		PathSetupMsg* emsg = new PathSetupMsg();

		ProcessorData* pdata = (ProcessorData*) rmsg->getData();

		emsg->setDstId(pdata->getSrcAddr());

		emsg->setId(globalMsgId++);
		emsg->setMsgType(grantDataTx);
		emsg->setSrcId(pdata->getDestAddr());
		emsg->setBitLength(CONTROL_PACKET_SIZE);
		emsg->setVirtualChannel(VirtualChannelControl::control->getVC(
				grantDataTx, pdata));

		//emsg->setAllowedTx(pdata->getSize()); //the size of a row, for the data allowed to TX
		emsg->setAllowedTx(cols * arrays * chips);

		credits[emsg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

		sendDelayed(emsg, clockPeriod_packet, packetPortOut);
	} else if (rmsg->getType() == router_arb_deny) { //message that says the NIF can transmit more write data
		debug(getFullPath(),
				"read began with a write in the queue, sending back path blocked to clear network resources",
				UNIT_DRAM);

		ProcessorData* pdata = (ProcessorData*) rmsg->decapsulate();

		PathSetupMsg* msg = new PathSetupMsg();

		msg->setSrcId(pdata->getDestAddr());
		msg->setDstId(pdata->getSrcAddr());
		msg->setId(globalMsgId++);
		msg->setMsgType(pathBlocked);
		msg->setBitLength(CONTROL_PACKET_SIZE);
		msg->setVirtualChannel(VirtualChannelControl::control->getVC(
				pathBlocked, NULL));
		msg->encapsulate(pdata);

		msg->setBlockedAddr((long) myAddr);

		credits[msg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

		sendDelayed(msg, clockPeriod_packet, packetPortOut);
	} else {
		opp_error("NIF_MemGW: unexpected router cntrl message type");
	}

}

void NIF_MemGW::packetPortMsgArrived(cMessage* cmsg) {
	ElectronicMessage* emsg = check_and_cast<ElectronicMessage*>(cmsg);
	int type = (ElectronicMessageTypes) (emsg->getMsgType());

	if (type == router_bufferAck) {
		ackMsgArrived(check_and_cast<BufferAckMsg*> (emsg));
		//controller();

	} else {
		//if (emsg->getDstId() == myId) {

		//always ack it back
		BufferAckMsg* ack = new BufferAckMsg;
		ack->setVirtualChannel(emsg->getVirtualChannel());
		ack->setData(emsg->getBitLength());
		ack->setMsgType(router_bufferAck);
		ack->setBitLength(emsg->getBitLength() / flitWidth);
		debug(getFullPath(), "ack sent", UNIT_BUFFER);
		sendDelayed(ack, 0, packetPortOut);

		if (type == dataPacket) {

			debug(getFullPath(), "transaction received at MemGW_NIF from ",
					AddressTranslator::untranslate_str(
							(NetworkAddress*) emsg->getSrcId()), UNIT_DRAM);

			ProcessorData* pdata = check_and_cast<ProcessorData*> (
					emsg->decapsulate());

			if (pdata->getIsComplete())
				sendDelayed(pdata, clockPeriod, toProc);

		} else if (type == pathSetup) {
			/*if (servicingRead) { //send back a pathBlocked if we're doing a read
				PathSetupMsg* msg = new PathSetupMsg();

				msg->setSrcId(emsg->getDstId());
				msg->setDstId(emsg->getSrcId());
				msg->setId(globalMsgId++);
				msg->setMsgType(pathBlocked);
				msg->setBitLength(CONTROL_PACKET_SIZE);
				msg->setVirtualChannel(VirtualChannelControl::control->getVC(
						pathBlocked, NULL));
				msg->encapsulate(emsg->decapsulate());

				msg->setBlockedAddr((long) myAddr);

				credits[msg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

				sendDelayed(msg, clockPeriod, elOut);

			} else {*/
				debug(getFullPath(), "transaction received at MemGW_NIF",
						UNIT_DRAM);

				ProcessorData* pdata = check_and_cast<ProcessorData*> (
						emsg->decapsulate());

				if (pdata->getIsComplete())
					sendDelayed(pdata, clockPeriod, toProc);
				else
					delete pdata;

			//}

		} else if (type == pathACK) {

			debug(getFullPath(), "pathACKed, informing memory controller",
					UNIT_DRAM);
			//this should be coming from a read
			RouterCntrlMsg* rmsg = new RouterCntrlMsg();
			rmsg->setType(router_bufferAck);
			sendDelayed(rmsg, clockPeriod, procReqOut);

			debug(getFullPath(), "path successful.. ", UNIT_PATH);
			debug(getFullPath(), "src ", AddressTranslator::untranslate_str(
					(NetworkAddress*) emsg->getSrcId()), UNIT_PATH);
			debug(getFullPath(), "dst ", AddressTranslator::untranslate_str(
					(NetworkAddress*) emsg->getDstId()), UNIT_PATH);


		} else if (type == requestDataTx) {
			debug(getFullPath(),
					"requesting more tx data, informing memory controller",
					UNIT_DRAM);
			RouterCntrlMsg* rmsg = new RouterCntrlMsg();
			rmsg->setType(router_arb_req);
			rmsg->setTime(emsg->getTime());
			sendDelayed(rmsg, clockPeriod, procReqOut);
		} else if (type == pathTeardown) {

			//tell the memory controller to turn the switch off
			RouterCntrlMsg* rmsg = new RouterCntrlMsg();
			rmsg->setType(router_arb_unblock);
			rmsg->setTime(emsg->getTime());
			sendDelayed(rmsg, clockPeriod, procReqOut);

			debug(getFullPath(), "teardown confirmed from ",
					AddressTranslator::untranslate_str(
							(NetworkAddress*) emsg->getSrcId()), UNIT_PATH);

			delete ((NetworkAddress*) emsg->getSrcId());
			delete ((NetworkAddress*) emsg->getDstId());



		} else if (type == pathBlocked) {
			PathSetupMsg* __attribute__ ((unused)) psm = check_and_cast<PathSetupMsg*> (emsg);


			ProcessorData* p = (ProcessorData*) emsg->decapsulate();
			sendSetup(p, clockPeriod + getRetryTime());
			delete p;
		} else {
			opp_error("NIF_MemGW: unexpected electronic message type");
		}

	}

}

bool NIF_MemGW::request() {
	return false;
}

bool NIF_MemGW::prepare() {
	return false;
}

simtime_t NIF_MemGW::send() {
	return 0;
}
void NIF_MemGW::complete() {

}

void NIF_MemGW::sendSetup(ProcessorData* pdata, simtime_t delay) {
	debug(getFullPath(), "sending out read setup to ",
			AddressTranslator::untranslate_str(
					(NetworkAddress*) pdata->getSrcAddr()), UNIT_DRAM);

	PathSetupMsg* emsg = new PathSetupMsg();

	emsg->setDstId(pdata->getSrcAddr());

	emsg->setId(globalMsgId++);
	emsg->setMsgType(pathSetup);
	emsg->setSrcId(pdata->getDestAddr());
	emsg->encapsulate(pdata->dup());
	emsg->setBitLength(CONTROL_PACKET_SIZE);
	emsg->setVirtualChannel(VirtualChannelControl::control->getVC(pathSetup,
			NULL));

	credits[emsg->getVirtualChannel()] -= CONTROL_PACKET_SIZE;

	sendDelayed(emsg, delay, packetPortOut);
}

