#include "ProcessorDataStripper.h"

Define_Module( PhotonicToProcessorMsg)
;
Define_Module( ElectronicToProcessorMsg)
;

void PhotonicToProcessorMsg::initialize() {

	procOut = gate("procOut");
	procIn = gate("procIn");

	photonicIn = gate("photonicIn$i");
	photonicOut = gate("photonicOut$o");

	wavelengths = par("numOfWavelengthChannels");
	photonicBitPeriod = 1.0 / (double) par("O_frequency_data");
}

void PhotonicToProcessorMsg::handleMessage(cMessage* msg) {

	if (msg->getArrivalGate() == photonicIn) {
		PhotonicMessage* pmsg = check_and_cast<PhotonicMessage*> (msg);

		if (pmsg->getMsgType() == TXstop) {
			ProcessorData *proc = check_and_cast<ProcessorData*> (
					pmsg->decapsulate());

			debug(getFullPath(), "ProcessorData stripped, sending ", UNIT_OPTICS);
			sendDelayed(proc, 0, procOut);

		} else if (pmsg->getMsgType() == TXstart) {
			if (pmsg->getEncapsulatedPacket() != NULL)
				delete pmsg->decapsulate();
		}

		delete msg;
	} else if (msg->getArrivalGate() == procIn) {
		ProcessorData *pdata = check_and_cast<ProcessorData*> (msg);

		PhotonicMessage* start = new PhotonicMessage();
		start->setId(globalMsgId);
		start->setBitLength(0);
		start->setMsgType(TXstart);

		PhotonicMessage* stop = new PhotonicMessage();
		stop->encapsulate(pdata);
		stop->setId(globalMsgId++);
		stop->setMsgType(TXstop);
		stop->setBitLength(pdata->getSize());

		send(start, photonicOut);
		sendDelayed(stop, photonicBitPeriod * pdata->getSize() / (wavelengths),
				photonicOut);

	}

}

void ElectronicToProcessorMsg::initialize() {

	procOut = gate("procOut");
	procIn = gate("procIn");

	electronicIn = gate("electronicIn");
	electronicOut = gate("electronicOut");
}

void ElectronicToProcessorMsg::handleMessage(cMessage* msg) {

	if (msg->getArrivalGate() == electronicIn) {
		ElectronicMessage* emsg = check_and_cast<ElectronicMessage*> (msg);

		ProcessorData *proc = check_and_cast<ProcessorData*> (
				emsg->decapsulate());

		sendDelayed(proc, 0, procOut);

		delete msg;
	} else if (msg->getArrivalGate() == procIn) {
		ProcessorData *pdata = check_and_cast<ProcessorData*> (msg);

		//TXstop
		ElectronicMessage* sendMsg = new ElectronicMessage();

		sendMsg->setMsgType(dataPacket);
		sendMsg->setId(pdata->getId());
		sendMsg->encapsulate(pdata);
		sendMsg->setBitLength(pdata->getSize());

		sendDelayed(sendMsg, 0, electronicOut);
	}
}
