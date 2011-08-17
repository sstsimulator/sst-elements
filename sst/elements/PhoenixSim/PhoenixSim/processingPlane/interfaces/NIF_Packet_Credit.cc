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

#include "NIF_Packet_Credit.h"

NIF_Packet_Credit::NIF_Packet_Credit() {
	// TODO Auto-generated constructor stub

}

NIF_Packet_Credit::~NIF_Packet_Credit() {
	// TODO Auto-generated destructor stub
}

void NIF_Packet_Credit::init() {

	packetPortIn = pIn[0];
	packetPortOut = pOut[0];

	transChannelOut = packetPortOut->findTransmissionChannel();

	int numVC = par("routerVirtualChannels");
	int buff = par("routerBufferSize");

	for (int i = 0; i < numVC; i++) {
		credits[i] = buff;
	}

	stalled = false;

	flitWidth = par("electronicChannelWidth");

	clockPeriod_packet = 1.0 / double(par("O_frequency_cntrl"));

	sendPacketMsg = new cMessage("send Packet Message");

	init_packet();
}

void NIF_Packet_Credit::finish() {

	cancelAndDelete(sendPacketMsg);

	this->NIF::finish();
}

void NIF_Packet_Credit::ackMsgArrived(BufferAckMsg* bmsg)
{
	credits[bmsg->getVirtualChannel()] += bmsg->getData();

	if (stalled) {
		//let the source know we're free
		ArbiterRequestMsg* req = new ArbiterRequestMsg();
		req->setType(router_arb_unblock);
		sendDelayed(req, 0, procReqOut);
		stalled = false;
	}

	if (!sendPacketMsg->isScheduled()) {

		scheduleAt(simTime(), sendPacketMsg);
	}

	if (!nextMsg->isScheduled()) {
		scheduleAt(simTime(), nextMsg);
	}
}

void NIF_Packet_Credit::processorMsgSent(ProcessorData* pdata) {

	ackMsg();
}

void NIF_Packet_Credit::ackMsg() {
	//std::cout<<"trying to ack"<<endl;
	if (unacked.size() == 0) {
		opp_error("trying to ack, but nothing to ack");
	}

	ElectronicMessage* emsg = unacked.front();

	//always ack it back
	BufferAckMsg* ack = new BufferAckMsg;
	ack->setVirtualChannel(emsg->getVirtualChannel());
	ack->setData(emsg->getBitLength());
	ack->setMsgType(router_bufferAck);
	ack->setBitLength(int(1 + double(emsg->getBitLength() / flitWidth)));
	debug(getFullPath(), "ack sent", UNIT_BUFFER);

	sendPacket(ack, true, emsg->getVirtualChannel(), 0, true);

	delete emsg;
	unacked.pop();
}

bool NIF_Packet_Credit::sendPacket(cPacket* msg, bool firstTime, int vc, simtime_t delay, bool isAck)
{
	simtime_t time;

	if ((transChannelOut != NULL && transChannelOut->isBusy()) || (packetQ.size() > 0 && packetQ.front() != msg) || (!isAck && credits[vc] < msg->getBitLength()))
	{

		if (firstTime)
		{
			packetQ.push(msg);
			packetVC[msg] = vc;
		}

		if (!sendPacketMsg->isScheduled() && transChannelOut != NULL && transChannelOut->isBusy())
		{

			time = transChannelOut->getTransmissionFinishTime();
			scheduleAt(time, sendPacketMsg);
		}

		return false;
	}


	if (!isAck)
	{
		credits[vc] -= msg->getBitLength();
	}

	sendDelayed(msg, delay, packetPortOut);

	return true;
}

void NIF_Packet_Credit::selfMsgArrived(cMessage *msg) {
	if (msg == sendPacketMsg) {
		if (packetQ.size() > 0) {
			cPacket* pack = packetQ.front();
			ElectronicMessage* emsg = check_and_cast<ElectronicMessage*> (pack);
			bool messageType = (emsg->getMsgType() == router_bufferAck);


			bool succ = sendPacket(pack, false, packetVC[pack],0,messageType);

			if (succ) {
				packetQ.pop();
				packetVC.erase(pack);
				if (packetQ.size() > 0)
					scheduleAt(simTime(), sendPacketMsg);
				else {
					if (!nextMsg->isScheduled()) {
						scheduleAt(simTime(), nextMsg);
					}
				}
			}
		}
	} else {
		this->NIF::selfMsgArrived(msg);
	}

}

void NIF_Packet_Credit::netPortMsgArrived(cMessage* msg) {

	if (msg->getArrivalGate() == packetPortIn) {
		packetPortMsgArrived(msg);
	}
}

int NIF_Packet_Credit::roundToFlit(int length) {
	if (length % flitWidth != 0)
		return ((length / flitWidth) + 1) * flitWidth;
	else
		return length;
}

