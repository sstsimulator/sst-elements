#include "cDramModule.h"
#include <stdio.h>
#include <stdlib.h>

#include "Processor.h"

#include "AddressTranslator_Standard.h"
#include "AddressTranslator_SquareRoot.h"
#include "AddressTranslator_FatTree.h"

//#include "../sesc/src/libcore/FetchEngine.h"

#include "packetstat.h"

Define_Module( cDramModule)
;

long cDramModule::transactionCount = 0;
AddressTranslator* cDramModule::translator = NULL;

cDramModule::cDramModule() {

}
cDramModule::~cDramModule() {
}

void cDramModule::finish() {

	debug(getFullPath(), "FINISH: start", UNIT_FINISH);

	cancelAndDelete(clkMessage);

	if (dramsim != NULL) {
		dramsim->finish();
		delete dramsim;
		dramsim = NULL;
	}

	if (translator != NULL) {
		delete translator;
		translator = NULL;
	}

	if (myAddr != NULL)
		delete myAddr;

	debug(getFullPath(), "FINISH: done", UNIT_FINISH);

}

int cDramModule::numInitStages() const {
	return 2;
}

void cDramModule::initialize(int stage) {

	if (stage == 0) {
		return;
	}

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	chipClk = 1.0 / (double) par("O_frequency_cntrl");

	string DRAMfile = par("DRAM_config_file");

	fromNif = gate("fromNic");
	toNif = gate("toNic");
	reqOut = gate("nicReq$o");
	reqIn = gate("nicReq$i");

	clk = 1.0 / (double) (par("dramFrequency"));
	clkMessage = new cMessage("clk");

	scheduleAt(simTime() + chipClk, clkMessage);

	myID = par("dramID");

	if (translator == NULL) {
		string trans_str = par("addressTranslator");

		if (trans_str.compare("standard") == 0) {
			translator = new AddressTranslator_Standard();
		} else if (trans_str.compare("square_root") == 0) {
			translator = new AddressTranslator_SquareRoot();
		} else if (trans_str.compare("fat_tree") == 0) {
			translator = new AddressTranslator_FatTree();
		} else {
			opp_error("Invalid address translator in cDramModule.");
		}

		translator->concentration = par("processorConcentration");
		translator->numProcs = Processor::numOfProcs;

		string str_pro = par("networkProfile");
		translator->makeProfile(str_pro.substr(str_pro.find(";") + 1) + "1."
				+ par("processorConcentration").str() + ".", str_pro.substr(0,
				str_pro.find(";")) + "DRAM.PROC.");

	}

	transactionCount = 0;

	int numArgs = 6;
	char** args = (char**) malloc(numArgs * sizeof(char*));

	char str[99];
	sprintf(str, "%f", (1.0 / SIMTIME_DBL(clk)) / 1000000.0);

	//char str2[99];
	//	sprintf(str2, "%f", (1.0 / SIMTIME_DBL(chipClk)) / 1000000.0);
	//fprintf(stdout, "cpu frequency string: %s", str);


	const char* temp = DRAMfile.c_str();
	string powerFile = DRAMfile.substr(0, DRAMfile.find_last_of(".")) + ".dsh";

	args[0] = "DRAMsim";
	args[1] = "-sim:MONET";
	args[2] = "-cpu:frequency";
	args[3] = str;
	args[4] = "-dram:spd_input";
	args[5] = (char*) temp;
	//args[6] = "-dram:power_input";
	//args[7] = (char*) powerFile.c_str();
	//args[8] = "-stat:power";
	//args[9] = "DRAMsimPower.txt";

	dramsim = DRAM_Sim::read_config(0, numArgs, args, 1);
	dramsim->setCallback(this);
	dramsim->set_thread_count(1);
	dramsim->start();

	delete args;

	accessSize = dramsim->config->cacheline_size * 8;

	readLock = false;
	writeLock = false;
	outstandingRequest = false;

	//numOfNodesX = par("numOfNodesX");
	//numOfNodesY = par("numOfNodesY");

	myAddr = NULL;

	SO_BW = Statistics::registerStat("Bandwidth (Gb/s)", StatObject::TIME_AVG,
			"DRAM");

	SO_reads = Statistics::registerStat("Reads", StatObject::TOTAL, "DRAM");
	SO_writes = Statistics::registerStat("Writes", StatObject::TOTAL, "DRAM");

	debug(getFullPath(), "INIT: done", UNIT_INIT);

}

void cDramModule::handleMessage(cMessage *msg) {

	if (msg == clkMessage) {

		if (dramsim->update()) {
			if(!clkMessage->isScheduled())
					scheduleAt(simTime() + chipClk, clkMessage);
		}

		return;
	} else if (msg->getArrivalGate() == fromNif) {
		debug(getFullPath(), "message arrived ", UNIT_DRAM);

		ProcessorData* pdata = check_and_cast<ProcessorData*> (msg);

		debug(getFullPath(), "new Transaction arrived with dest ",
				translator->untranslate_str(
						(NetworkAddress*) pdata->getDestAddr()), UNIT_DRAM);
		debug(getFullPath(), "and src ", translator->untranslate_str(
				(NetworkAddress*) pdata->getSrcAddr()), UNIT_DRAM);

		MemoryAccess* mem = (MemoryAccess*) pdata->decapsulate();

		if (mem->getAccessType() == MemoryReadCmd) {
			if (pdata->getIsComplete()) {
				int total = mem->getAccessSize();
				mem->setAccessSize(accessSize);
				int i = 0;

				while (total > accessSize) {
					MemoryAccess* m2 = mem->dup();
					m2->setAddr(mem->getAddr() + i * (accessSize / 8));
					m2->setIsComplete(false);
					i++;
					pending.push(m2);
					total -= accessSize;
				}
				mem->setIsComplete(true);
				mem->setAddr(mem->getAddr() + i * (accessSize / 8));
				pending.push(mem);
			}
		} else {

			pending.push(mem);
		}

		tryNewTrans();

		if (myAddr == NULL) {
			myAddr = ((NetworkAddress*) pdata->getDestAddr())->dup();
		}

		if (pdata->getIsComplete()) {
			delete (NetworkAddress*) pdata->getDestAddr();
			delete (NetworkAddress*) pdata->getSrcAddr();
		}
		delete pdata;

	}

	else if (msg->getArrivalGate() == reqIn) { //the request port
		RouterCntrlMsg* rmsg = check_and_cast<RouterCntrlMsg*> (msg);

		if (rmsg->getType() == proc_grant) {
			send(theQ.front(), toNif);

			theQ.pop();

			outstandingRequest = false;

			if (theQ.size() > 0) {
				sendRequest(theQ.front());

				outstandingRequest = true;
			}
		} else if (rmsg->getType() == proc_req_send) { //from the nic
			debug(getFullPath(), "proc_req_send arrived ", UNIT_DRAM);
			RouterCntrlMsg* gr = new RouterCntrlMsg();
			gr->setType(proc_grant);

			if (pending.size() == 0) {
				debug(getFullPath(), "...granting ", UNIT_DRAM);
				send(gr, reqOut);
			} else {
				debug(getFullPath(), "...queueing up ", UNIT_DRAM);
				waitingNIF.push(gr);
			}
		} else if (rmsg->getType() == proc_msg_sent) { //ignore it
			ApplicationData* adata = (ApplicationData*) rmsg->getData();

			delete adata;
		} else if (rmsg->getType() == router_arb_deny) { //readLock
			readLock = true;
			debug(getFullPath(), "locking DRAM", UNIT_DRAM);
		} else if (rmsg->getType() == router_arb_unblock) { //readLock
			readLock = false;
			debug(getFullPath(), "unlocking DRAM", UNIT_DRAM);
			//scheduleAt(simTime() + chipClk, clkMessage);
			tryNewTrans();
		}

		delete msg;

	}

}

void cDramModule::sendRequest(ProcessorData* pdata) {

	ArbiterRequestMsg* req = new ArbiterRequestMsg();
	req->setType(proc_req_send);
	req->setVc(0);
	req->setDest(pdata->getDestAddr());
	req->setReqType(dataPacket);
	req->setPortIn(0);
	req->setSize(1);
	req->setTimestamp(simTime());
	req->setMsgId(pdata->getId());

	send(req, reqOut);
}

void cDramModule::return_transaction(int thread_id, int trans_id) {

	debug(getFullPath(), "Transaction returned", UNIT_DRAM);

	MemoryAccess* mem = (*mem_requests[thread_id])[trans_id];

	mem_requests[thread_id]->erase(mem_requests[thread_id]->find(trans_id));

	simtime_t accessTime = simTime() - request_times[mem];
	request_times.erase(mem);

	SO_BW->track(accessSize / 1e9);

	if (mem->getAccessType() == MemoryReadCmd) { //it was a read, send it back

		int srcId = mem->getSrcId();

		mem->setSrcId(mem->getDestId());
		mem->setDestId(srcId);
		mem->setType(M_readResponse);
		mem->setPayloadSize(accessSize);

		ProcessorData* pdata = new ProcessorData();
		pdata->setSize(accessSize);

		NetworkAddress* src = myAddr->dup();
		NetworkAddress* dest = translator->translateAddr(mem);

		pdata->setSrcAddr((long) src);
		pdata->setDestAddr((long) dest);
		pdata->setIsComplete(mem->getIsComplete());
		pdata->encapsulate(mem);

		SO_reads->track(1);

		debug(getFullPath(), "Returning Read Data to ",
				translator->untranslate_str(
						(NetworkAddress*) pdata->getDestAddr()), UNIT_DRAM);
		debug(getFullPath(), "from ", translator->untranslate_str(
				(NetworkAddress*) pdata->getSrcAddr()), UNIT_DRAM);
		debug(getFullPath(), "read size ", accessSize, UNIT_DRAM);

		theQ.push(pdata);
		if (!outstandingRequest) {
			outstandingRequest = true;

			sendRequest(pdata);
		}

	} else if (mem->getAccessType() == MemoryWriteCmd) {
		SO_writes->track(1);
		delete mem;

	}

	tryNewTrans();

}

void cDramModule::tryNewTrans() {

	if (pending.size() > 0) {

		MemoryAccess* access = (MemoryAccess*) pending.front();

		int addr = access->getAddr();

		if ((access->getAccessType() == MemoryWriteCmd)
				|| (access->getAccessType() == MemoryReadCmd && !readLock)) {

			if (access->getAccessType() == MemoryWriteCmd) {
				if (addrOffset.find(access->getId()) == addrOffset.end()) {
					addrOffset[access->getId()] = 0;
				}

				addr += addrOffset[access->getId()];

			}

			int access_type = access->getAccessType();
			int thread_id = access->getThreadId();
			int priority = access->getPriority();
			int transaction_id = transactionCount++;

			bool success = dramsim->add_new_transaction(thread_id,
					transaction_id, addr, access_type, priority);
			if (success) {

				if (access->getAccessType() == MemoryWriteCmd) {
					addrOffset[access->getId()] += accessSize / 8;
				}

				pending.pop();

				debug(getFullPath(), "transaction scheduled with addr ", addr,
						UNIT_DRAM);
				debug(getFullPath(), "... and id  ", access->getId(), UNIT_DRAM);

				//std::cout << "here3" << endl;
				if (mem_requests.find(thread_id) == mem_requests.end()) {
					//	std::cout << "here2" << endl;
					mem_requests[thread_id] = new map<int, MemoryAccess*> ();
				}

				//std::cout << "here1" << endl;
				if (mem_requests[thread_id]->find(transaction_id)
						!= mem_requests[thread_id]->end()) {
					opp_error(
							"encountered same memory transaction id in cDramModule.");
				} else {
					(*mem_requests[thread_id])[transaction_id] = access;
					request_times[access] = simTime();
				}

				if (!clkMessage->isScheduled()) {
					//cancelEvent(clkMessage);
					scheduleAt(simTime() + chipClk, clkMessage);
				}

				tryNewTrans();
			}
		} else {
			debug(getFullPath(), "unsuccessful schedule", UNIT_DRAM);
		}
	}

	if (pending.size() < WRITE_LOCK_THRESHOLD && waitingNIF.size() > 0) {
		debug(getFullPath(), "sending to nif", UNIT_DRAM);
		send(waitingNIF.front(), reqOut);
		waitingNIF.pop();
	}

}

