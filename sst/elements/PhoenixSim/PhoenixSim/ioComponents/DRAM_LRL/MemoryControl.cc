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

#include "MemoryControl.h"

Define_Module( MemoryControl)
;
Define_Module( MemoryControl_E)
;

MemoryControl::MemoryControl() {
	// TODO Auto-generated constructor stub

}

MemoryControl_E::MemoryControl_E() {
	// TODO Auto-generated constructor stub

}

MemoryControl::~MemoryControl() {
	// TODO Auto-generated destructor stub
}

void MemoryControl::initialize() {

	freq = par("DRAM_freq");

	double temp = par("DRAM_tCAS"); //in clk cycles
	tRCD = temp / (freq / 2);
	temp = par("DRAM_tCL");
	tCL = temp / (freq / 2);
	temp = par("DRAM_tRP");
	tRP = temp / (freq / 2);

	chips = par("DRAM_chipsPerDIMM");
	arrays = par("DRAM_arrays");
	cols = par("DRAM_cols");
	banks = par("DRAM_banksPerChip");

	temp = par("O_frequency_data");
	photonicBitPeriod = 1.0 / temp;
	temp = par("numOfWavelengthChannels");

	accessUnit = arrays * cols * chips;
	accessTime = max(cols / (freq), SIMTIME_DBL(accessUnit * photonicBitPeriod
			/ temp));

	numDIMMs = par("DRAM_DIMMsPerGW");

	fromNIF = gate("fromNIF");
	toNIF = gate("toNIF");
	nifReqIn = gate("nifReq$i");
	nifReqOut = gate("nifReq$o");

	toDIMMs = gate("toDIMMs");

	int p = par("numPSEports");

	for (int i = 0; i < p; i++) {
		switchCntrl[i] = gate("switchCntrl", i);
	}

	currentAccess = NULL;
	chunk = 0;

	moreCommands = new cMessage();

	locked = false;
	unlockMsg = new cMessage();

	ToF = par("maxToF");

	servicingRead = false;
	readPathSetup = false;
}

void MemoryControl::finish() {

	if (moreCommands->isScheduled()) {
		std::cout << "moreCommands msg still scheduled!" << endl;
	}

	if (waiting.size() > 0) {
		ProcessorData* pdata = waiting.front();

		std::cout << "messages still in memory controller queue." << endl;
		std::cout << "front message type = "
				<< ((((MemoryAccess*) pdata->getEncapsulatedPacket())->getAccessType()
						== MemoryReadCmd) ? "Read" : "Write") << endl;
	}

	if (currentAccess != NULL) {
		std::cout << "currentAccess:" << endl;
		std::cout << "type = "
				<< ((((MemoryAccess*) currentAccess->getEncapsulatedPacket())->getAccessType()
						== MemoryReadCmd) ? "Read" : "Write") << endl;
		std::cout << "chunk = " << chunk << endl;
	}

	cancelAndDelete(moreCommands);
	cancelAndDelete(unlockMsg);
}

void MemoryControl::handleMessage(cMessage* msg) {

	if (msg == moreCommands) {

		if (chunk * accessUnit
				< ((MemoryAccess*) currentAccess->getEncapsulatedPacket())->getAccessSize()) {
			sendReadcommands(0);
			chunk++;
			scheduleAt(simTime() + tRCD + tCL + tRP + accessTime, moreCommands);
		} else {
			turnSwitchOff(0);
			servicingRead = false;
			readPathSetup = false;

			//this will tell the NIF to do a path teardown
			RouterCntrlMsg* setup = new RouterCntrlMsg();
			setup->setType(router_arb_unblock);
			setup->setData((long) currentAccess->dup());
			sendDelayed(setup, 0, nifReqOut);

			chunk = 0;
			delete currentAccess;
			currentAccess = NULL;
			startNewTransaction(1e-9);
		}
	} else if (msg == unlockMsg) {
		locked = false;
		startNewTransaction(0);

	} else if (msg->getArrivalGate() == fromNIF) {
		ProcessorData* pdata = check_and_cast<ProcessorData*> (msg);

		/*if (((MemoryAccess*) pdata->getEncapsulatedMsg())->getAccessType()
		 == MemoryWriteCmd && servicingRead) {
		 //this will tell the NIF to do a path blocked
		 RouterCntrlMsg* block = new RouterCntrlMsg();
		 block->encapsulate(pdata);
		 block->setType(router_arb_deny);
		 sendDelayed(block, 0, nifReqOut);
		 } else {*/
		if (((MemoryAccess*) pdata->getEncapsulatedPacket())->getAccessType()
				== MemoryWriteCmd) {
			waiting.push_front(pdata);
		} else {
			waiting.push_back(pdata);
		}
		readMutex();
		startNewTransaction(0);
		//}


	} else if (msg->getArrivalGate() == nifReqIn) { //the request port
		RouterCntrlMsg* rmsg = check_and_cast<RouterCntrlMsg*> (msg);

		if (rmsg->getType() == router_arb_grant) {

		} else if (rmsg->getType() == router_arb_req) {
			//the write is asking for more data
			simtime_t delay = 0;

			//if(simTime() < rmsg->getTime() + accessTime + ToF){
			//	delay = rmsg->getTime() + accessTime + ToF;
			//}

			write((MemoryAccess*) currentAccess->getEncapsulatedPacket(), false,
					delay);

		} else if (rmsg->getType() == router_bufferAck) {
			//read path was setup, continue with DRAM commands
			sendReadcommands(0);
			readPathSetup = true;
			chunk++;
			scheduleAt(simTime() + tRCD + tCL + tRP + accessTime, moreCommands);

		} else if (rmsg->getType() == router_arb_unblock) {
			simtime_t msgstart = rmsg->getTime();
			debug(getFullPath(), "Received teardown, msgstart=", SIMTIME_DBL(
					msgstart), UNIT_DRAM);

			delete currentAccess;
			currentAccess = NULL;
			chunk = 0;
			turnSwitchOff(0);

			if (msgstart + tRP + accessTime + ToF > simTime()) {
				debug(getFullPath(), "locking", UNIT_DRAM);

				locked = true;
				scheduleAt(msgstart + tRP + accessTime + ToF, unlockMsg);
			} else {
				debug(getFullPath(), "starting new transaction...", UNIT_DRAM);

				startNewTransaction(1e-9);
			}
		}

		delete msg;

	}
}

void MemoryControl::startNewTransaction(simtime_t delay) {

	if (currentAccess == NULL && waiting.size() > 0 && !locked) {
		currentAccess = waiting.front();
		currentAccess->setSize(accessUnit); //size of row in bits
		waiting.pop_front();

		MemoryAccess* mem =
				((MemoryAccess*) currentAccess->getEncapsulatedPacket());

		//round up to a full chunk of data
		/*int c = mem->getAccessSize() / accessUnit;
		 if (c * accessUnit != mem->getAccessSize()) {
		 mem->setAccessSize((c + 1) * mem->getAccessSize());

		 }*/

		if (mem->getAccessType() == MemoryReadCmd) {

			servicingRead = true;

			debug(getFullPath(), "Read Transaction started", UNIT_DRAM);

			mem->setDimm(getDIMM(mem->getAddr()));

			setSwitchRead(mem->getDimm(), PSE_ON, delay);
			//setSwitchWrite(mem->getDimm(), PSE_ON, delay);
			setSwitchMods(mem->getDimm(), PSE_ON, delay);

			//this will tell the NIF to do a path setup
			RouterCntrlMsg* setup = new RouterCntrlMsg();
			setup->setType(router_arb_req);
			setup->setData((long) currentAccess);
			sendDelayed(setup, delay, nifReqOut);

		} else if (mem->getAccessType() == MemoryWriteCmd) {
			debug(getFullPath(), "Write Transaction started", UNIT_DRAM);
			write(mem, true, delay);
		}
	}

}

//this function is necessary to check if a write is in the queue,
// we must send back that it has been blocked, to free resources in the network
// to avoid deadlock
void MemoryControl::readMutex() {
	list<ProcessorData*>::iterator iter;
	ProcessorData* rem = NULL;

	for (iter = waiting.begin(); iter != waiting.end(); iter++) {
		MemoryAccess* mem = ((MemoryAccess*) (*iter)->getEncapsulatedPacket());
		if (mem->getAccessType() == MemoryWriteCmd && (servicingRead
				&& !readPathSetup)) {
			rem = *iter;
			//this will tell the NIF to do a path blocked
			RouterCntrlMsg* block = new RouterCntrlMsg();
			block->encapsulate(rem);
			block->setType(router_arb_deny);
			sendDelayed(block, 0, nifReqOut);

			break;
		}
	}

	if (rem != NULL) {
		waiting.remove(rem);
	}
}

void MemoryControl::write(MemoryAccess* mem, bool first, simtime_t delay) {

	if (first)
		mem->setDimm(getDIMM(mem->getAddr()));

	setSwitchMods(mem->getDimm(), PSE_ON, delay);

	sendWritecommands(delay + photonicBitPeriod);

	setSwitchMods(mem->getDimm(), PSE_OFF, delay + photonicBitPeriod * 8 + tRCD);
	setSwitchWrite(mem->getDimm(), PSE_ON, delay + photonicBitPeriod * 8 + tRCD);

	//this will tell the NIF to do a moreDataTx
	RouterCntrlMsg* setup = new RouterCntrlMsg();
	setup->setType(first ? router_bufferAck : router_arb_grant);
	setup->setData((long) currentAccess);
	sendDelayed(setup, delay + photonicBitPeriod * 8 + tRCD, nifReqOut);

	chunk++;

}

void MemoryControl::sendReadcommands(simtime_t delay) {

	debug(getFullPath(), "Sending read commands..", UNIT_DRAM);

	int r = chunk / banks;
	int c = 0;
	int b = chunk % banks;

	MemoryAccess* memA = ((MemoryAccess*) currentAccess->getEncapsulatedPacket());

	int burst = min(cols, ((memA->getAccessSize() - chunk * accessUnit)
			/ (arrays * chips)) + 1);
	bool last = (chunk + 1) * accessUnit >= memA->getAccessSize();

	PhotonicMessage * pmsg;

	DRAM_CntrlMsg* row = new DRAM_CntrlMsg();
	row->setType(Row_Access);
	row->setRow(r);
	row->setWriteEn(false);
	row->setBank(b);
	row->setCoreAddr(currentAccess->getSrcAddr());

	pmsg = new PhotonicMessage();
	pmsg->setMsgType(TXstart);
	pmsg->setId(globalMsgId);
	pmsg->setBitLength(0);
	sendDelayed(pmsg, delay, toDIMMs);
	pmsg = new PhotonicMessage();
	pmsg->setId(globalMsgId++);
	pmsg->setMsgType(TXstop);
	pmsg->encapsulate(row);
	pmsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(pmsg, delay + photonicBitPeriod * 2, toDIMMs);

	DRAM_CntrlMsg* col = new DRAM_CntrlMsg();
	col->setType(Col_Access);
	col->setCol(c);
	col->setBurst(burst);
	col->setWriteEn(false);
	col->setBank(b);
	col->setLastAccess(last);
	col->setCreationTime(
			((MemoryAccess*) currentAccess->getEncapsulatedPacket())->getCreationTime());
	col->setData((long)memA);

	pmsg = new PhotonicMessage();
	pmsg->setMsgType(TXstart);
	pmsg->setId(globalMsgId);
	pmsg->setBitLength(0);
	sendDelayed(pmsg, delay + tRCD + photonicBitPeriod * 3, toDIMMs);
	pmsg = new PhotonicMessage();
	pmsg->setId(globalMsgId++);
	pmsg->setMsgType(TXstop);
	pmsg->encapsulate(col);
	pmsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(pmsg, delay + photonicBitPeriod * 5 + tRCD, toDIMMs);

}

void MemoryControl::sendWritecommands(simtime_t delay) {

	int r = chunk / banks;
	int c = 0;
	int b = chunk % banks;

	debug(getFullPath(), "Sending write commands to bank..", b, UNIT_DRAM);

	MemoryAccess* memA = ((MemoryAccess*) currentAccess->getEncapsulatedPacket());

	int burst = min(cols, ((memA->getAccessSize() - chunk * accessUnit)
			/ (arrays * chips)) + 1);

	PhotonicMessage * pmsg;

	DRAM_CntrlMsg* row = new DRAM_CntrlMsg();
	row->setType(Row_Access);
	row->setRow(r);
	row->setWriteEn(true);
	row->setBank(b);

	row->setCoreAddr(currentAccess->getSrcAddr());
	pmsg = new PhotonicMessage();
	pmsg->setMsgType(TXstart);
	pmsg->setId(globalMsgId);
	pmsg->setBitLength(0);
	sendDelayed(pmsg, delay, toDIMMs);
	pmsg = new PhotonicMessage();
	pmsg->setId(globalMsgId++);
	pmsg->setMsgType(TXstop);
	pmsg->encapsulate(row);
	pmsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(pmsg, delay + photonicBitPeriod * 2, toDIMMs);

	DRAM_CntrlMsg* col = new DRAM_CntrlMsg();
	col->setType(Col_Access);
	col->setCol(c);
	col->setBurst(burst);
	col->setWriteEn(true);
	col->setBank(b);

	pmsg = new PhotonicMessage();
	pmsg->setMsgType(TXstart);
	pmsg->setId(globalMsgId);
	pmsg->setBitLength(0);
	sendDelayed(pmsg, delay + photonicBitPeriod * 3 + tRCD, toDIMMs);
	pmsg = new PhotonicMessage();
	pmsg->setId(globalMsgId++);
	pmsg->setMsgType(TXstop);
	pmsg->encapsulate(col);
	pmsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(pmsg, delay + photonicBitPeriod * 5 + tRCD, toDIMMs);

}

void MemoryControl::turnSwitchOff(simtime_t delay) {
	debug(getFullPath(), "Turning off photonic switch..", UNIT_DRAM);

	for (int i = 0; i < numDIMMs - 1; i++) {
		ElementControlMessage* set = new ElementControlMessage();
		set->setState(PSE_OFF);

		set->setType(router_pse_setup);

		sendDelayed(set, delay, switchCntrl[i]);

		set = new ElementControlMessage();

		set->setState(PSE_OFF);
		set->setType(router_pse_setup);

		sendDelayed(set, delay, switchCntrl[i + numDIMMs - 1]);
	}

	ElementControlMessage* set = new ElementControlMessage();

	set->setState(PSE_OFF);
	set->setType(router_pse_setup);

	sendDelayed(set, delay, switchCntrl[(numDIMMs - 1) * 2]);

}

void MemoryControl::setSwitchRead(int dimm, int st, simtime_t delay) {

	debug(getFullPath(), "Setting up switch for a read..", UNIT_DRAM);

	if (dimm != 0) {

		ElementControlMessage* setIn = new ElementControlMessage();

		setIn->setState(st);
		setIn->setType(router_pse_setup);

		sendDelayed(setIn, delay, switchCntrl[dimm - 1]);

	}

}

void MemoryControl::setSwitchWrite(int dimm, int st, simtime_t delay) {

	debug(getFullPath(), "Setting up switch for write, PSE #", dimm - 1
			+ (numDIMMs - 1), UNIT_DRAM);

	if (dimm != 0) {

		ElementControlMessage* setOut = new ElementControlMessage();
		setOut->setState(st);
		setOut->setType(router_pse_setup);

		sendDelayed(setOut, delay, switchCntrl[dimm - 1 + (numDIMMs - 1)]);
	}

}

void MemoryControl::setSwitchMods(int dimm, int on, simtime_t delay) {

	debug(getFullPath(), "Setting up switch to transmit commands..", UNIT_DRAM);

	ElementControlMessage* setcntrl = new ElementControlMessage();
	setcntrl->setState(on);
	setcntrl->setType(router_pse_setup);

	sendDelayed(setcntrl, delay, switchCntrl[(numDIMMs - 1) * 2]);

	if (dimm != 0) {

		ElementControlMessage* setOut = new ElementControlMessage();
		setOut->setState(on);
		setOut->setType(router_pse_setup);

		sendDelayed(setOut, delay, switchCntrl[dimm - 1 + (numDIMMs - 1)]);
	}
}

int MemoryControl::getDIMM(int addr) {
	int d = intuniform(0, numDIMMs - 1);
	debug(getFullPath(), "Selectring DIMM ", d, UNIT_DRAM);

	return d;
}

void MemoryControl_E::turnSwitchOff(simtime_t delay) {
	debug(getFullPath(), "MC_E: Turning off photonic switch..", UNIT_DRAM);

	for (int i = 0; i < numDIMMs + 2; i++) {
		ElementControlMessage* set = new ElementControlMessage();
		set->setState(-1);
		set->setPSEid(i);
		set->setType(router_pse_setup);

		debug(getFullPath(), "clearing port ", i, UNIT_DRAM);

		sendDelayed(set, delay, switchCntrl[0]);
	}

}

void MemoryControl_E::setSwitchRead(int dimm, int st, simtime_t delay) {

	debug(getFullPath(), "Setting up switch for a read..", UNIT_DRAM);

	ElementControlMessage* setIn = new ElementControlMessage();

	setIn->setState((st == PSE_ON ? 0 : -1));
	setIn->setPSEid(2 + dimm);
	setIn->setType(router_pse_setup);

	sendDelayed(setIn, delay, switchCntrl[0]);

}

void MemoryControl_E::setSwitchWrite(int dimm, int st, simtime_t delay) {

	debug(getFullPath(), "Setting up switch for write, PSE #", dimm - 1
			+ (numDIMMs - 1), UNIT_DRAM);

	ElementControlMessage* setOut = new ElementControlMessage();
	setOut->setState((st == PSE_ON ? 2 + dimm : -1));
	setOut->setPSEid(0);
	setOut->setType(router_pse_setup);

	sendDelayed(setOut, delay, switchCntrl[0]);

}

void MemoryControl_E::setSwitchMods(int dimm, int on, simtime_t delay) {

	debug(getFullPath(), "Setting up switch to transmit commands..", UNIT_DRAM);

	ElementControlMessage* setcntrl = new ElementControlMessage();
	setcntrl->setState((on == PSE_ON ? 2 + dimm : -1));
	setcntrl->setPSEid(1);
	setcntrl->setType(router_pse_setup);

	sendDelayed(setcntrl, delay, switchCntrl[0]);
}

void MemoryControl_E::sendReadcommands(simtime_t delay) {

	debug(getFullPath(), "Sending read commands..", UNIT_DRAM);

	int r = chunk / banks;
	int c = 0;
	int b = chunk % banks;

	MemoryAccess* memA = ((MemoryAccess*) currentAccess->getEncapsulatedPacket());

	int burst = min(cols, ((memA->getAccessSize() - chunk * accessUnit)
			/ (arrays * chips)) + 1);
	bool last = (chunk + 1) * accessUnit >= memA->getAccessSize();

	ElectronicMessage * sendMsg;

	DRAM_CntrlMsg* row = new DRAM_CntrlMsg();
	row->setType(Row_Access);
	row->setRow(r);
	row->setWriteEn(false);
	row->setBank(b);
	row->setCoreAddr(currentAccess->getSrcAddr());

	sendMsg = new ElectronicMessage;
	sendMsg->setMsgType(dataPacket);
	sendMsg->setId(globalMsgId++);
	sendMsg->encapsulate(row);
	sendMsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(sendMsg, delay, toDIMMs);

	DRAM_CntrlMsg* col = new DRAM_CntrlMsg();
	col->setType(Col_Access);
	col->setCol(c);
	col->setBurst(burst);
	col->setWriteEn(false);
	col->setBank(b);
	col->setLastAccess(last);
	col->setCreationTime(
			((MemoryAccess*) currentAccess->getEncapsulatedPacket())->getCreationTime());
	col->setData((long)memA);

	sendMsg = new ElectronicMessage;
	sendMsg->setMsgType(dataPacket);
	sendMsg->setId(globalMsgId++);
	sendMsg->encapsulate(col);
	sendMsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(sendMsg, delay + photonicBitPeriod * 3 + tRCD, toDIMMs);

}

void MemoryControl_E::sendWritecommands(simtime_t delay) {

	int r = chunk / banks;
	int c = 0;
	int b = chunk % banks;

	debug(getFullPath(), "Sending write commands to bank..", b, UNIT_DRAM);

	MemoryAccess* memA = ((MemoryAccess*) currentAccess->getEncapsulatedPacket());

	int burst = min(cols, ((memA->getAccessSize() - chunk * accessUnit)
			/ (arrays * chips)) + 1);

	ElectronicMessage * sendMsg;

	DRAM_CntrlMsg* row = new DRAM_CntrlMsg();
	row->setType(Row_Access);
	row->setRow(r);
	row->setWriteEn(true);
	row->setBank(b);

	row->setCoreAddr(currentAccess->getSrcAddr());

	sendMsg = new ElectronicMessage;
	sendMsg->setMsgType(dataPacket);
	sendMsg->setId(globalMsgId++);
	sendMsg->encapsulate(row);
	sendMsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(sendMsg, delay, toDIMMs);

	DRAM_CntrlMsg* col = new DRAM_CntrlMsg();
	col->setType(Col_Access);
	col->setCol(c);
	col->setBurst(burst);
	col->setWriteEn(true);
	col->setBank(b);

	sendMsg = new ElectronicMessage;
	sendMsg->setMsgType(dataPacket);
	sendMsg->setId(globalMsgId++);
	sendMsg->encapsulate(col);
	sendMsg->setBitLength(DRAM_CNTRL_SIZE);
	sendDelayed(sendMsg, delay + photonicBitPeriod * 3 + tRCD, toDIMMs);

}
