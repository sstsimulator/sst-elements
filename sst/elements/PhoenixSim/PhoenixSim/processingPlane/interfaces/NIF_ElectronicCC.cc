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

#include "NIF_ElectronicCC.h"

Define_Module( NIF_ElectronicCC)
;

NIF_ElectronicCC::NIF_ElectronicCC() {
	// TODO Auto-generated constructor stub

}

void NIF_ElectronicCC::dataMsgArrived(cMessage* cmsg) {
	//if (pmsg->getDstId() == myId) {
	ElectronicMessage* emsg = check_and_cast<ElectronicMessage*> (cmsg);

	waitOnDataArrive = false;

	debug(getFullPath(), "Data received", UNIT_NIC);

	ProcessorData* pdata = check_and_cast<ProcessorData*> (emsg->decapsulate());

	sendDelayed(pdata, 0, this->toProc);

	controller();

}

bool NIF_ElectronicCC::prepare() {

	bitsSent = min(allowedTx, currData->getSize());

	ProcessorData* toSend = currData->dup();
	toSend->setDataType(1);

	toSend->setIsComplete(bitsSent == currData->getSize());
	toSend->setSize(bitsSent);

	//TXstop
	sendMsg = new ElectronicMessage;

	sendMsg->setMsgType(dataPacket);

	sendMsg->setId(currData->getId());
	sendMsg->encapsulate(toSend);
	sendMsg->setBitLength(bitsSent);

	return true;

}

simtime_t NIF_ElectronicCC::send() {

	sendDelayed(sendMsg, 0, dataPortOut);

	msgDuration = ((simtime_t) (bitsSent * clockPeriod_data
			/ (double) maxDataWidth));
	debug(getFullPath(), "msg duration = ", SIMTIME_DBL(msgDuration), UNIT_NIC);

	SO_bw->track(bitsSent / 1e9);
	SO_qTime->track((simTime() - enqueueTimeOfSendingMessage).dbl());

	if (bitsSent >= currData->getSize()) {
		//notify processor that data was sent
		RouterCntrlMsg* msgSent = new RouterCntrlMsg();
		msgSent->setType(proc_msg_sent);
		NetworkAddress* adr = (NetworkAddress*) currData->getSrcAddr();
		int p = adr->id[AddressTranslator::convertLevel("PROC")];
		msgSent->setMsgId(p);
		msgSent->setData(
				(long) (((ApplicationData*) currData->getEncapsulatedMsg())->dup()));
		sendDelayed(msgSent, 0, procReqOut);
	}

	sourceAddr = ((NetworkAddress*) currData->getSrcAddr())->dup();
	destAddr = ((NetworkAddress*) currData->getDestAddr())->dup();

	return clockPeriod_packet + (msgDuration * (maxDataWidth / allowedWL));

}

