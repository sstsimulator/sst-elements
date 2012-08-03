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

#include "mux.h"

Define_Module(mux)
;
Define_Module(demux)
;
Define_Module(inoutConverter)
;

mux::mux() {
	// TODO Auto-generated constructor stub

}

mux::~mux() {
	// TODO Auto-generated destructor stub
}

demux::demux() {
	// TODO Auto-generated constructor stub

}

demux::~demux() {
	// TODO Auto-generated destructor stub
}

inoutConverter::inoutConverter() {

}

void demux::initialize() {
	numOut = par("numOut");

	for (int i = 0; i < numOut; i++) {
		out[i] = gate("out", i);
	}

}

void inoutConverter::initialize() {
	gin = gate("in");
	gout = gate("out");
	io_in = gate("io_in$i");
	io_out = gate("io_out$o");
}

void mux::initialize() {
	outgate = gate("out");
	qMessage = new cMessage("muxQmessage");
	timerMessage = new cMessage("muxTimerMessage");\
	clockPeriod = 1.0 / (double)par("O_frequency_cntrl");
	transChannel = outgate->findTransmissionChannel();
}
void mux::handleMessage(cMessage *msg) {

	if(msg == qMessage){
		if(transChannel->isBusy()){
			std::cout << "WTF: channel is still busy?" << endl;
			return;
		}
		cMessage* m = fakeQ.front();
		fakeQ.pop();
		send(m, outgate);

		if(fakeQ.size() > 0)
			scheduleAt(simTime() + clockPeriod, timerMessage);

	}else if(msg == timerMessage){
		if(transChannel->isBusy())
			scheduleAt(transChannel->getTransmissionFinishTime(), qMessage);
		else
			scheduleAt(simTime(), qMessage);

	}else if (transChannel != NULL) {
		if (transChannel->isBusy() || qMessage->isScheduled() || timerMessage->isScheduled()) {
			fakeQ.push(msg);

			if(!qMessage->isScheduled() && !timerMessage->isScheduled())
				scheduleAt(transChannel->getTransmissionFinishTime(), qMessage);
		} else
			send(msg, outgate);
	} else
		send(msg, outgate);
}

void mux::finish(){

	cancelAndDelete(qMessage);
	cancelAndDelete(timerMessage);
}

void demux::handleMessage(cMessage *msg) {
	PhotonicMessage *orig_pmsg = (PhotonicMessage*) msg;

	PhotonicMessage *pmsg;

	for (int i = 0; i < numOut - 1; i++) {
		pmsg = orig_pmsg->dup();
		//pmsg->setPacketStat((long)(new PacketStat((PacketStat*)(orig_pmsg->getPacketStat()))));
		send(pmsg, out[i]);

	}

	send(msg, out[numOut - 1]);
}

void inoutConverter::handleMessage(cMessage *msg) {
	if (msg->getArrivalGate() == gin) {
		send(msg, io_out);
	} else if (msg->getArrivalGate() == io_in) {
		send(msg, gout);
	} else {
		opp_error(
				"inoutConverter: messages arriving where they're not supposed to");
	}
}

