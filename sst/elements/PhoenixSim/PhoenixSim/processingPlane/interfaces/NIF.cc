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

#include "NIF.h"

#include "packetstat.h"

int NIF::procDataCount = 0;
//int NIF::pathsEstablished = 0;

int NIF::numOfNIFs = 0;
int NIF::nifsDone = 0;

NIF::NIF() {

}

NIF::~NIF() {

}

void NIF::initialize() {

	numNetPorts = par("numNetPorts");

	numOfNIFs++;
	numProcsDone = 0;
	procIsDone = false;

	for (int i = 0; i < numNetPorts; i++) {
		pIn[i] = gate("portIn", i);
		pOut[i] = gate("portOut", i);

	}

	concentration = par("processorConcentration");

	procReqIn = gate("procReq$i");
	procReqOut = gate("procReq$o");
	fromProc = gate("fromProc");
	toProc = gate("toProc");

	nextMsg = new cMessage();

	sentRequest = false;

	completeMsg = new cMessage();

	init();

	SO_bw = Statistics::registerStat("Bandwidth NIF Out (Gb/s)",
			StatObject::TIME_AVG, "NIF");
	SO_qTime
			= Statistics::registerStat("Time in NIF Q", StatObject::MMA, "NIF");


}

void NIF::finish() {
	if (nextMsg->isScheduled()) {
		cancelEvent(nextMsg);
	}
	delete nextMsg;

	cancelAndDelete(completeMsg);

	list<ProcessorData*>::iterator iter;

	 StatObject* SO_latency = Statistics::registerStat("Latency (us)", StatObject::MMA, "application");

	 for (iter = waiting.begin(); iter != waiting.end(); iter++)
	 {
		 ProcessorData* pdata = (*iter);

		 ApplicationData* adata = (ApplicationData*) pdata->decapsulate();
		 SO_latency->track(SIMTIME_DBL(simTime() - adata->getCreationTime()) / 1e-6); //put into us
		 delete adata;
		 delete pdata;
	 }
}

void NIF::printProcData(ProcessorData* pdata){
	std::cout << "----- Processor Data -----" << endl;
	std::cout << "type: " << pdata->getType() << endl;
	std::cout << "size: " << pdata->getSize() << endl;

	NetworkAddress* dest = (NetworkAddress*) pdata->getDestAddr();
	NetworkAddress* src = (NetworkAddress*) pdata->getSrcAddr();

	std::cout << "src: " << src->id[AddressTranslator::convertLevel("NET")] << endl;
	std::cout << "dest: " << dest->id[AddressTranslator::convertLevel("NET")] << endl;

}

void NIF::handleMessage(cMessage* msg) {

	cGate* arrivalGate = msg->getArrivalGate();

	if (msg->isSelfMessage()) {
		if (msg == nextMsg) {
			controller();
		}else if(msg == completeMsg){
			complete();
			controller();
		}
		else {
			selfMsgArrived(msg);
		}

	} else if (arrivalGate == procReqIn) { //schedule new message
		RouterCntrlMsg* rmsg = check_and_cast<RouterCntrlMsg*> (msg);

		requestMsgArrived(rmsg);

		delete msg;
	} else if (arrivalGate == fromProc) {

		ProcessorData* pdata = check_and_cast<ProcessorData*> (msg);
		procMsgArrived(pdata);

	} else {
		netPortMsgArrived(msg);

		delete msg;
	}
}

void NIF::sendDataToProcessor(ProcessorData* pdata) {
	//debug(getFullPath(), "start sendDataToProcessor", UNIT_NIC);

	if(pdata != NULL)
		toProcQ.push(pdata);
	else
		pdata = toProcQ.front();

	if (!sentRequest && toProcQ.size() > 0) {

		ArbiterRequestMsg* req = new ArbiterRequestMsg();
		req->setType(proc_req_send);
		req->setVc(0);
		req->setDest(pdata->getDestAddr());
		req->setReqType(dataPacket);
		req->setPortIn(concentration);
		req->setSize(1);
		req->setTimestamp(simTime());
		req->setMsgId(pdata->getId());
		req->setStage(10000);

		//debug(getFullPath(), "sending proc_req_send", UNIT_NIC);

		sendDelayed(req, 0, procReqOut);

		sentRequest = true;
	}

	//debug(getFullPath(), "end sendDataToProcessor", UNIT_NIC);
}

void NIF::procMsgArrived(ProcessorData* pdata) {

	debug(getFullPath(), "proc Msg arrived ", UNIT_NIC);

	waiting.push_back(pdata);

	if(waiting.size() > 1e5)
	{
		std::cout<<"ending simulation since a NIF queue reached 100000"<<endl;
		this->endSimulation();
	}
	pdata->setNifArriveTime(simTime());



	controller();
}

void NIF::requestMsgArrived(RouterCntrlMsg* rmsg) {

	if (rmsg->getType() == proc_grant) {
		if (toProcQ.size() == 0) {
			opp_error("got a proc_grant, but nothing in the toProcQ");
		}
		sendDelayed(toProcQ.front(), 0, toProc);

		processorMsgSent(toProcQ.front());

		toProcQ.pop();

		sentRequest= false;
		sendDataToProcessor(NULL);

	} else if (rmsg->getType() == proc_req_send) { //from the proc, immediately grant
		RouterCntrlMsg* gr = new RouterCntrlMsg();
		gr->setType(proc_grant);
		sendDelayed(gr, 0, procReqOut);
	}

}

void NIF::controller() {

	//simtime_t nextTime = -1;
	//do {
		//nextTime = -1;
//std::cout << "controller " << simTime() << endl;
	if(!completeMsg->isScheduled()){
		if (request()) {
			if (prepare()) {
				simtime_t delay = send();
				scheduleAt(simTime() + delay, completeMsg);

			}
		}
	}

		//if (!nextMsg->isScheduled() && nextTime > 0) {
		//	scheduleAt(simTime() + nextTime, nextMsg);
		//}

	//} while (nextTime == 0);

}


