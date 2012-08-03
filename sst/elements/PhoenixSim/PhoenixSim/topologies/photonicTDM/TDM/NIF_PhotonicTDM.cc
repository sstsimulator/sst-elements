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

#include "NIF_PhotonicTDM.h"

#include <time.h>

Define_Module(NIF_PhotonicTDM)
;

NIF_PhotonicTDM::NIF_PhotonicTDM() {
	// TODO Auto-generated constructor stub

}
/*
 NIF_PhotonicTDM::~NIF_PhotonicTDM() {
 if (waiting.size() > 0) {

 std::cout << "NIF " << myId << ": Unsent messages in waiting queue:"
 << endl;

 list<ProcessorData*>::iterator iter;

 for (iter = waiting.begin(); iter != waiting.end(); iter++) {
 printProcData(*iter);
 delete *iter;
 }
 }
 }


 void NIF_PhotonicTDM::init() {

 tdmPeriod = par("tdmSlotPeriod");
 numX = par("numOfNodesX");
 numTDMslots = (numX - 1) * (numX / 2);
 tdmClock = new cMessage();

 myId = par("id");
 myRow = myId / numX;
 myCol = myId % numX;

 cntrlRate = par("O_frequency_cntrl");
 cntrlPeriod = 1.0 / cntrlRate;

 dataRate = par("O_frequency_data");
 clockPeriod_data = 1.0 / dataRate;
 numWavelengths = par("numOfWavelengthChannels");

 sentThisSlot = false;

 slotWaitTime = 0.2e-9;

 tdmSlot = -1;

 sentCntrl = false;
 //rx = 0;
 //tx = 0;

 srand(time(NULL));

 firstInSlot = true;

 slotGaurdTime = 2e-9;

 dataToSend = NULL;

 string fName = par("cntrlFileName");
 readControlFile(fName);

 scheduleAt(0, tdmClock);

 SO_lat_trans = Statistics::registerStat("Transmission latency",
 StatObject::MMA, "NIF");

 SO_lat_slot = Statistics::registerStat("Slot latency", StatObject::MMA,
 "NIF");

 #ifdef ENABLE_ORION
 ORION_Array_Info *info = new ORION_Array_Info();

 int buf_type = SRAM;
 int is_fifo = false;
 int n_read_port = 1;

 int n_write_port = 1;
 int clockFactor = dataRate / cntrlRate;
 int n_entry = clockFactor * (3); //3 entries - rx, send, arbitrate
 temp_line_width = (SIMTIME_DBL(tdmPeriod) - 2e-9) * cntrlRate
 * numWavelengths;
 int outdrv = false;

 info->init(buf_type, is_fifo, n_read_port, n_write_port, n_entry,
 temp_line_width, outdrv, Statistics::DEFAULT_ORION_CONFIG_CNTRL);

 tempBuffer = new ORION_Array();
 tempBuffer->init(info);

 string statName = "NIF: TempRx-Buffer";
 StatObject* temp_static = Statistics::registerStat(statName,
 StatObject::ENERGY_STATIC, "electronic");
 tempPower = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
 "electronic");

 temp_static->track(tempBuffer->stat_static_power());

 #endif
 }

 bool NIF_PhotonicTDM::goTransmit() {

 if ((*cntrlMatrix[tdmSlot])[myId] >= 0) {

 int netlevel = AddressTranslator::convertLevel("NET");

 //then check the requests
 list<ProcessorData*>::iterator iter2;
 for (iter2 = waiting.begin(); iter2 != waiting.end(); iter2++) {
 NetworkAddress* destAddr =
 (NetworkAddress*) (*iter2)->getDestAddr();
 int netid = destAddr->id[netlevel];

 if (dataToSend == NULL && (*cntrlMatrix[tdmSlot])[myId] == netid) {

 dataToSend = (*iter2);
 debug(getFullPath(), "can send from ", myId, UNIT_NIC);
 debug(getFullPath(), "to ", netid, UNIT_NIC);
 } else if (dataToSend != NULL && (*cntrlMatrix[tdmSlot])[myId]
 == netid) { //next in line
 (*iter2)->setSavedTime2((simTime() - (*iter2)->getNifArriveTime()) + (*iter2)->getSavedTime2());
 (*iter2)->setNifArriveTime(simTime());
 }
 }

 if (dataToSend != NULL) {

 waiting.remove(dataToSend);
 return true;
 }
 }

 return false;
 }

 bool NIF_PhotonicTDM::request() {

 if (!sentThisSlot && tdmClock->getArrivalTime() - simTime() > slotGaurdTime) {

 return waiting.size() > 0 && dataToSend == NULL && goTransmit();
 } else
 return false;
 }

 bool NIF_PhotonicTDM::prepare() {
 middleOfSending = true;

 if (dataToSend != NULL) {
 ProcessorData* toSend = dataToSend->dup();

 double
 time =
 SIMTIME_DBL(tdmClock->getArrivalTime()- simTime() - slotGaurdTime);

 if (time < 0) {
 std::cout << "tdm clock arrival: " << tdmClock->getArrivalTime()
 << endl;
 std::cout << "simTime: " << simTime() << endl;
 opp_error("negative time, negative TDMbitsallowed");
 }

 TDMbitsAllowed = time * dataRate * numWavelengths;

 bitsSent = min((long) toSend->getSize(), TDMbitsAllowed);

 toSend->setSize(bitsSent);

 toSend->setSavedTime1(simTime());

 SO_lat_slot->track(SIMTIME_DBL(simTime() - toSend->getNifArriveTime()));
 SO_qTime->track(SIMTIME_DBL(toSend->getSavedTime2()));


 toSend->setIsComplete(dataToSend->getSize() <= bitsSent);

 //TXstart
 start = new PhotonicMessage;

 start->setMsgType(TXstart);

 start->setId(toSend->getId());
 start->setBitLength(0);
 start->encapsulate(toSend->dup());

 //TXstop
 stop = new PhotonicMessage;

 stop->setMsgType(TXstop);

 stop->setId(toSend->getId());
 stop->setBitLength(bitsSent);

 stop->encapsulate(toSend);

 return true;
 }
 return false;
 }

 simtime_t NIF_PhotonicTDM::send() {

 sendDelayed(start, 0, pOut[0]);

 msgDuration = ((simtime_t) (bitsSent * clockPeriod_data
 / (double) numWavelengths));
 debug(getFullPath(), "msg duration = ", SIMTIME_DBL(msgDuration), UNIT_NIC);

 sendDelayed(stop, msgDuration, pOut[0]);

 SO_bw->track(bitsSent / 1e9);

 sentThisSlot = true;

 dataToSend->setSize(dataToSend->getSize() - bitsSent);

 if (dataToSend->getSize() <= 0) {

 NetworkAddress* adr;

 adr = (NetworkAddress*) dataToSend->getSrcAddr();

 if (adr->id[AddressTranslator::convertLevel("NET")] == myId) {
 //notify processor that data was sent

 RouterCntrlMsg* msgSent = new RouterCntrlMsg();
 msgSent->setType(proc_msg_sent);

 int p = adr->id[AddressTranslator::convertLevel("PROC")];
 msgSent->setMsgId(p);
 msgSent->setData(
 (long) (((ApplicationData*) dataToSend->getEncapsulatedPacket())->dup()));
 sendDelayed(msgSent, msgDuration, procReqOut);

 }
 }

 return msgDuration;
 }

 void NIF_PhotonicTDM::complete() {

 debug(getFullPath(), "sent bits = ", bitsSent, UNIT_NIC);

 if (dataToSend->getSize() <= 0) {

 delete dataToSend;

 debug(getFullPath(), "message sending complete.. ", UNIT_PATH);
 } else {

 //std::cout << "here, size = " << item->procData->getSize()
 //		<< ", done = " << item->done() << endl;
 debug(getFullPath(), "more message to send.. ", UNIT_PATH);

 waiting.push_back(dataToSend);
 }

 start = NULL;
 stop = NULL;

 middleOfSending = false;
 dataToSend = NULL;

 return;

 }

 void NIF_PhotonicTDM::netPortMsgArrived(cMessage* cmsg) {
 PhotonicMessage* pmsg = check_and_cast<PhotonicMessage*> (cmsg);
 if (pmsg->getMsgType() == TXstart) {
 debug(getFullPath(), "TXstart received", UNIT_NIC);

 delete pmsg->decapsulate();

 } else if (pmsg->getMsgType() == TXstop) {

 debug(getFullPath(), "TXstop received", UNIT_NIC);

 ProcessorData* pdata = check_and_cast<ProcessorData*> (
 pmsg->decapsulate());

 SO_lat_trans->track(SIMTIME_DBL(simTime() - pdata->getSavedTime1()));

 NetworkAddress* dest = (NetworkAddress*) pdata->getDestAddr();

 debug(getFullPath(), "going to proc ",
 dest->id[AddressTranslator::convertLevel("PROC")], UNIT_NIC);

 //pdata->setIsComplete(true);

 tempPower->track(tempBuffer->stat_energy(0, min(1, pdata->getSize()
 / temp_line_width), 0));

 sendDataToProcessor(pdata);

 } else {
 opp_error("optical MsgType not set");
 }
 }

 void NIF_PhotonicTDM::processorMsgSent(ProcessorData* pdata) {
 tempPower->track(tempBuffer->stat_energy(min(1, pdata->getSize()
 / temp_line_width), 0, 0));
 }

 void NIF_PhotonicTDM::selfMsgArrived(cMessage* cmsg) {

 if (cmsg == tdmClock) {
 sentThisSlot = false;

 tdmSlot++;
 if (tdmSlot >= numTDMslots)
 tdmSlot = 0;

 if (myId == 0)
 debug(getFullPath(), "Changing TDM slot to ", tdmSlot, UNIT_NIC);
 //debug(getFullPath(), "rx is ", rx, UNIT_NIC);
 //debug(getFullPath(), "tx is ", tx, UNIT_NIC);

 if (!Statistics::isDone)
 scheduleAt(simTime() + tdmPeriod, tdmClock);
 else
 delete tdmClock;

 if (!nextMsg->isScheduled())
 controller();
 }

 }
 */

NIF_PhotonicTDM::~NIF_PhotonicTDM() {
	if (tQueue.size() > 0) {
		std::cout << "------ " << myId
				<< ": messages left in tQueue ----------" << endl;

		list<ProcessorData*>::iterator iter;

		for (iter = tQueue.begin(); iter != tQueue.end(); iter++) {
			NetworkAddress* dest = (NetworkAddress*) (*iter)->getDestAddr();
			int d = dest->id[AddressTranslator::convertLevel("NET")];
			std::cout << "type: " << (*iter)->getType() << ", dest: " << d
					<< endl;
		}
	}
}

void NIF_PhotonicTDM::init() {

	tdmPeriod = par("tdmSlotPeriod");
	numX = par("numOfNodesX");
	numTDMslots = (numX - 1) * (numX / 2);
	tdmClock = new cMessage();

	myId = par("id");
	myRow = myId / numX;
	myCol = myId % numX;

	cntrlRate = par("O_frequency_cntrl");
	cntrlPeriod = 1.0 / cntrlRate;

	dataRate = par("O_frequency_data");
	procRate = par("O_frequency_proc");
	clockPeriod_data = 1.0 / dataRate;
	numWavelengths = par("numOfWavelengthChannels");

	ScheduleItem::dataRate = dataRate;
	ScheduleItem::numWavelengths = numWavelengths;
	ScheduleItem::cntrlPeriod = cntrlPeriod;

	sentThisSlot = false;

	freq = par("DRAM_freq");

	double temp = par("DRAM_tCAS"); //in clk cycles
	tRCD = 2 * temp / freq;
	temp = par("DRAM_tCL");
	tCL = 2 * temp / freq;
	temp = par("DRAM_tRP");
	tRP = 2 * temp / freq;

	DRAMguardTime = 3e-9;

	slotWaitTime = 0.2e-9;

	chips = par("DRAM_chipsPerDIMM");
	arrays = par("DRAM_arrays");
	cols = par("DRAM_cols");
	banks = par("DRAM_banksPerChip");
	rows = par("DRAM_rows");

	useIOplane = par("useIOplane");

	ScheduleItem_Mem::rows = rows;
	ScheduleItem_Mem::cols = cols;
	ScheduleItem_Mem::banks = banks;
	ScheduleItem_Mem::chips = chips;
	ScheduleItem_Mem::arrays = arrays;

	accessUnit = arrays * cols * chips;
	accessTime = max(cols / (freq), SIMTIME_DBL(accessUnit * clockPeriod_data
			/ temp));

	readPropConst = 2e-9;
	maxBurst = (int) ((SIMTIME_DBL(tdmPeriod - slotWaitTime - readPropConst)
			* dataRate * numWavelengths) / (arrays * chips));

	for (int i = 0; i < banks; i++) {
		//openRow[i] = new map<int, bool> ();
		//precharging[i] = new map<int, bool> ();
		bankSet[i] = false;

		//for (int j = 0; j < rows; j++) {
		//	(*openRow[i])[j] = false;
		//	(*precharging[i])[j] = false;
		//}
	}

	tdmSlot = -1;

	sentCntrl = false;
	//rx = 0;
	//tx = 0;

	srand(time(NULL));

	firstInSlot = true;

	//dataToSend = NULL;

	string fName = par("cntrlFileName");
	readControlFile(fName);

	scheduleAt(0, tdmClock);

	SO_lat_queue = Statistics::registerStat("Queuing latency", StatObject::MMA,
			"NIF");
	SO_lat_slot = Statistics::registerStat("Slot latency", StatObject::MMA,
			"NIF");

	SO_lat_trans = Statistics::registerStat("Transmission latency",
			StatObject::MMA, "NIF");

#ifdef ENABLE_ORION
	/*ORION_Array_Info *info = new ORION_Array_Info();

	 int buf_type = SRAM;
	 int is_fifo = false;
	 int n_read_port = 1;

	 int n_write_port = 1;
	 int clockFactor = dataRate / procRate;
	 int n_entry = clockFactor * (numX - 1); //an entry for each receive slot
	 xy_line_width = (SIMTIME_DBL(tdmPeriod) - 2e-9) * cntrlRate
	 * numWavelengths;

	 //int n_entry = (SIMTIME_DBL(tdmPeriod) - 2e-9) * dataRate * (numX - 1);
	 //xy_line_width = numWavelengths;
	 int outdrv = false;

	 info->init(buf_type, is_fifo, n_read_port, n_write_port, n_entry,
	 xy_line_width, outdrv, Statistics::DEFAULT_ORION_CONFIG_CNTRL);

	 xyBuffer = new ORION_Array();
	 xyBuffer->init(info);

	 string statName = "NIF: XY-Buffer";
	 StatObject* xy_static = Statistics::registerStat(statName,
	 StatObject::ENERGY_STATIC, "electronic");
	 xyPower = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
	 "electronic");

	 xy_static->track(xyBuffer->stat_static_power());*/

#endif
}

void NIF_PhotonicTDM::procMsgArrived(ProcessorData* pdata) {
	//send unblock
	XbarMsg *unblock = new XbarMsg();
	unblock->setType(router_arb_unblock);
	unblock->setToPort(concentration);

	sendDelayed(unblock, 0, procReqOut);

	pdata->setNifArriveTime(simTime());

	addToSchedule(pdata, false);

	if (!nextMsg->isScheduled())
		controller();

}

int NIF_PhotonicTDM::getBankId(int addr) {
	int bankBits = log2(banks);
	//int colBits = log2(cols);
	int mask = 0;
	for (int i = 0; i < bankBits; i++) {
		mask = mask | (1 << i);
	}
	return ((addr / (rows * cols * arrays * chips / 8)) & mask);
}

int NIF_PhotonicTDM::getRow(int addr) {
	int rowBits = log2(rows);
	//int bankBits = log2(banks);
	//int colBits = log2(cols);
	int mask = 0;
	for (int i = 0; i < rowBits; i++) {
		mask = mask | (1 << i);
	}
	return ((addr / (cols * arrays * chips / 8)) & mask);
}

int NIF_PhotonicTDM::getCol(int addr) {

	/*int colBits = log2(cols);
	 int mask = 0;
	 for (int i = 0; i < colBits; i++) {
	 mask = mask | (1 << i);
	 }
	 return ((addr / (arrays * chips / 8)) & mask);
	 */
	return 0;

}

//constraints to check:
//	1) not sending to 2 different places
//	2) not receiving from 2 different places
//	3) slot is long enough to accommodate all transmissions
bool NIF_PhotonicTDM::okToSchedule(ScheduleItem* it, int slot) {

	if (slot == tdmSlot)
		return false;
	else if (schedule[slot]->size() > 1) {
		return false;
	} else if (schedule[slot]->size() == 1) {
		ScheduleItem* item = schedule[slot]->front();

		if (item->type == (ScheduleItem::RX && it->type
				!= ScheduleItem::MEM_LCL_READ) || (it->type
				== ScheduleItem::MEM_WRITE_DATA && item->type
				== ScheduleItem::MEM_WRITE))
			return true;
		else
			return false;

	} else
		return true;

}

bool NIF_PhotonicTDM::addToSchedule(ProcessorData* pdata, bool isXY) {

	if (pdata->getType() == DM_readLocal || pdata->getType() == DM_writeLocal) {
		memSchedule:

		//debug(getFullPath(), "scheduling a local memory access", UNIT_NIC);
		int cmdSlot = -1;
		int dataSlot = -1;
		MemoryAccess* mem = (MemoryAccess*) pdata->getEncapsulatedPacket();
		bool scheduled = false;

		int addr = mem->getAddr();
		int bankId = getBankId(addr);
		int row = getRow(addr);
		int size = mem->getAccessSize();

		ScheduleItem_RowCmd* cmd = new ScheduleItem_RowCmd(pdata->dup(),
				bankId, row, size / (arrays * chips) + 1, maxBurst);
		ScheduleItem* data = NULL;

		if (pdata->getType() == DM_readLocal) {
			data = new ScheduleItem_ReadData(pdata->dup(), bankId, row, getCol(
					addr), size / (arrays * chips) + 1, maxBurst);
			//cmd->size = min(maxBurst, cols);
		} else if (pdata->getType() == DM_writeLocal || pdata->getType()
				== DM_writeRemote) {
			data = new ScheduleItem_WriteCmd(pdata->dup(), bankId, row, getCol(
					addr), size / (arrays * chips) + 1, maxBurst);
		}

		if (!bankSet[bankId]) {
			for (int slot = (tdmSlot + 1) % numTDMslots; slot != tdmSlot
					&& (cmdSlot < 0 || dataSlot < 0); slot = (slot + 1)
					% numTDMslots) {
				if ((*cntrlMatrix[slot])[myId] < 0) {
					if (okToSchedule(cmd, slot) && cmdSlot < 0) {
						cmdSlot = slot;
					}

					if (cmdSlot >= 0 && slot != cmdSlot) {
						if (slot > cmdSlot) {
							if ((slot - cmdSlot) * tdmPeriod >= tRCD + tCL
									+ DRAMguardTime) {
								if (okToSchedule(data, slot)) {
									dataSlot = slot;
									scheduled = true;
								}
							}
						} else {
							if ((27 - cmdSlot + slot) * tdmPeriod >= tRCD + tCL
									+ DRAMguardTime) {
								if (okToSchedule(data, slot)) {
									dataSlot = slot;
									scheduled = true;
								}
							}
						}
					}
				}
			}
		}

		if (!scheduled) {
			tQueue.push_back(pdata);
			delete cmd;
			delete data;
			return false;
		} else {
			debug(getFullPath(), "adding to schedule, bank = ", bankId,
					UNIT_NIC);
			debug(getFullPath(), "adding to schedule, cmdslot = ", cmdSlot,
					UNIT_NIC);
			debug(getFullPath(), "adding to schedule, dataslot = ", dataSlot,
					UNIT_NIC);
			debug(getFullPath(), "adding to schedule, current slot = ",
					tdmSlot, UNIT_NIC);
			schedule[cmdSlot]->push_back(cmd);
			schedule[dataSlot]->push_back(data);

			if (pdata->getType() == DM_writeLocal || pdata->getType()
					== DM_writeRemote) {
				data = new ScheduleItem_WriteData(pdata->dup(), bankId, row,
						getCol(addr), size / (arrays * chips) + 1, maxBurst);
				schedule[dataSlot]->push_back(data);

			}

			delete pdata;

			bankSet[bankId] = true;

			return true;
		}

	} else {
		int netlevel = AddressTranslator::convertLevel("NET");
		NetworkAddress* destAddr = (NetworkAddress*) pdata->getDestAddr();
		int netid = destAddr->id[netlevel];

		if ((pdata->getType() == DM_readRemote || pdata->getType()
				== DM_writeRemote)) {
			if (netid != myId) { //send it there
				//debug(getFullPath(),
				//		"scheduling sending a remote memory access", UNIT_NIC);
				goto sending;
			} else if (pdata->getType() == DM_writeRemote) {
				goto memSchedule;
			} else { //just arrived here, access memory
				debug(
						getFullPath(),
						"scheduling a remote memory access which arrived to its destination",
						UNIT_NIC);
				NetworkAddress* srcAddr = (NetworkAddress*) pdata->getSrcAddr();
				int returnid = srcAddr->id[netlevel];

				int dataSlot = (*reverseMatrix[myId])[returnid];

				if (dataSlot == tdmSlot || dataSlot < 0) {
					tQueue.push_back(pdata);
					return false;
				}

				int cmdSlot = -1;

				MemoryAccess* mem =
						(MemoryAccess*) pdata->getEncapsulatedPacket();
				bool scheduled = false;

				int addr = mem->getAddr();
				int bankId = getBankId(addr);
				int row = getRow(addr);
				long size = mem->getAccessSize();

				ScheduleItem* data = NULL;

				if (pdata->getType() == DM_readRemote) {

					data = new ScheduleItem_ReadData_Remote(pdata->dup(),
							bankId, row, getCol(addr), size / (arrays * chips)
									+ 1, maxBurst);

				} else if (pdata->getType() == DM_writeRemote) {
					data
							= new ScheduleItem_WriteCmd(pdata->dup(), bankId,
									row, getCol(addr), size / (arrays * chips)
											+ 1, maxBurst);
				}

				if (!okToSchedule(data, dataSlot)) {
					tQueue.push_back(pdata);
					delete data;
					return false;
				}

				ScheduleItem_RowCmd* cmd = new ScheduleItem_RowCmd(
						pdata->dup(), bankId, row, size / (arrays * chips) + 1,
						maxBurst);

				//if (pdata->getType() == DM_readRemote) {
				//cmd->size = min(maxBurst, cols);
				//}

				if (!bankSet[bankId]) {
					for (int slot = (dataSlot > 0) ? (dataSlot - 1)
							: (numTDMslots - 1); slot != tdmSlot && slot
							!= dataSlot && (cmdSlot < 0); slot
							= (slot > 0) ? (slot - 1) : (numTDMslots - 1)) {
						if ((*cntrlMatrix[slot])[myId] < 0) { //free slot

							//check for business in the schedule
							if (((slot > dataSlot) && ((dataSlot + (numTDMslots
									- 1 - slot)) * tdmPeriod >= tRCD + tCL
									+ DRAMguardTime)) || ((slot < dataSlot)
									&& ((dataSlot - slot) * tdmPeriod >= tRCD
											+ tCL + DRAMguardTime))) {
								if (okToSchedule(cmd, slot)) {
									cmdSlot = slot;
									scheduled = true;
								}

							}

						}

					}
				}

				if (!scheduled) {
					tQueue.push_back(pdata);
					delete cmd;
					delete data;
					return false;
				} else {
					debug(getFullPath(),
							"remote - adding to schedule, bank = ", bankId,
							UNIT_NIC);
					debug(getFullPath(),
							"remote - adding to schedule, cmdslot = ", cmdSlot,
							UNIT_NIC);
					debug(getFullPath(),
							"remote - adding to schedule, dataslot = ",
							dataSlot, UNIT_NIC);
					debug(getFullPath(),
							"remote - adding to schedule, current slot = ",
							tdmSlot, UNIT_NIC);

					schedule[cmdSlot]->push_back(cmd);
					schedule[dataSlot]->push_back(data);

					if (pdata->getType() == DM_writeRemote) {
						data = new ScheduleItem_WriteData(pdata->dup(), bankId,
								row, getCol(addr), size / (arrays * chips) + 1,
								maxBurst);
						schedule[dataSlot]->push_back(data);
					}

					delete pdata;

					bankSet[bankId] = true;

					return true;
				}
			}
		} else {
			//debug(getFullPath(), "scheduling a send", UNIT_NIC);
			sending:

			int slot = (*reverseMatrix[myId])[netid];

			//if (schedule[slot]->size() == 0) {
			pdata->setSavedTime2(simTime());
			ScheduleItem *item = new ScheduleItem_Send(pdata->dup());
			item->isXY = isXY;

			if (okToSchedule(item, slot)) {

				debug(getFullPath(), "scheduling a send in slot ", slot,
						UNIT_NIC);
				schedule[slot]->push_back(item);

				SO_lat_queue->track(
						SIMTIME_DBL(simTime() - pdata->getNifArriveTime()));

				delete pdata;
			} else {
				delete item;

				tQueue.push_back(pdata);
				return false;
			}

			return true;
		}
	}

}

void NIF_PhotonicTDM::tryTqueue() {

	if (tQueue.size() > 0) {
		ProcessorData* p = tQueue.front();

		bool succ = addToSchedule(p, false);

		if (succ) {

			debug(getFullPath(), "tryTqueue: successful schedule ", UNIT_NIC);
			tQueue.pop_front();
		} else {
			debug(getFullPath(), "tryTqueue: UNsuccessful schedule ", UNIT_NIC);
			tQueue.pop_back();
		}
	}
}

bool NIF_PhotonicTDM::request() {

	/*if (!sentThisSlot && (SIMTIME_DBL(tdmClock->getArrivalTime() - simTime())> 1e-9))
	 return goTransmit();
	 else
	 return false;*/

	//if (!sentThisSlot && SIMTIME_DBL(tdmClock->getArrivalTime() - simTime()) > 2e-9) {
	if (!sentThisSlot) {

		return schedule[tdmSlot]->size() > 0;
	} else
		return false;
}

bool NIF_PhotonicTDM::prepare() {
	middleOfSending = true;

	//dataToSend = schedule[tdmSlot]->front()->procData;
	ScheduleItem* item = schedule[tdmSlot]->front();
	if (item->sentThisSlot)
		return false;

	if (!item->cntrlSetup) {
		sendRingCntrl();
	}

	ProcessorData* toSend = item->getMsg();

	if (toSend != NULL) {

		debug(getFullPath(), "preparing schedule type ", item->type, UNIT_NIC);
		debug(getFullPath(), "at time slot ", tdmSlot, UNIT_NIC);

		double
				time =
						SIMTIME_DBL(tdmClock->getArrivalTime()- simTime() - item->getWaitTime());

		if (time < 0) {
			/*std::cout << "tdm clock arrival: " << tdmClock->getArrivalTime()
			 << endl;
			 std::cout << "simTime: " << simTime() << endl;
			 std::cout << "wait time: " << item->getWaitTime() << endl;
			 opp_error("negative time, negative TDMbitsallowed");*/
			return false;
		}

		toSend = toSend->dup();

		TDMbitsAllowed = time * dataRate * numWavelengths;

		bitsSent = min((long) toSend->getSize(), TDMbitsAllowed);

		toSend->setSize(bitsSent);

		item->sendingBits(bitsSent);

		toSend->setIsComplete(toSend->getIsComplete() && item->done());

		simtime_t extra = (simTime() > tdmClock->getArrivalTime() - tdmPeriod
				+ slotWaitTime ? 0 : (tdmClock->getArrivalTime() - tdmPeriod
				+ slotWaitTime - simTime()));

		SO_lat_slot->track(SIMTIME_DBL(simTime() - toSend->getSavedTime2()));

		/*if(!schedule[tdmSlot]->front()->isXY) {
		 toSend->setIsComplete(bitsSent == dataToSend->getSize());
		 }
		 else {
		 originalComplete = dataToSend->getIsComplete();
		 toSend->setIsComplete(bitsSent == dataToSend->getSize() && dataToSend->getIsComplete());
		 }*/

		//toSend->setIsComplete(true);

		ProcessorData* startDup = toSend->dup();
		startDup->setSavedTime1(simTime() + extra);
		toSend->setSavedTime1(simTime() + extra);

		//TXstart
		start = new PhotonicMessage;

		start->setMsgType(TXstart);

		start->setId(toSend->getId());
		start->setBitLength(0);
		start->encapsulate(startDup);

		//TXstop
		stop = new PhotonicMessage;

		stop->setMsgType(TXstop);

		stop->setId(toSend->getId());
		stop->setBitLength(bitsSent);

		stop->encapsulate(toSend);

		return true;
	} else if (schedule[tdmSlot]->size() > 1) {
		toSend = NULL;
		ScheduleItem *item = schedule[tdmSlot]->front();
		schedule[tdmSlot]->pop_front();
		schedule[tdmSlot]->push_back(item);

		if (!nextMsg->isScheduled())
			controller();
	}
	return false;
}

simtime_t NIF_PhotonicTDM::send() {
	ScheduleItem* item = schedule[tdmSlot]->front();

	item->myTimeSlot = tdmSlot;

	simtime_t extra = (simTime() > tdmClock->getArrivalTime() - tdmPeriod
			+ slotWaitTime ? 0 : (tdmClock->getArrivalTime() - tdmPeriod
			+ slotWaitTime - simTime()));
	sendDelayed(start, extra, pOut[0]);

	msgDuration = ((simtime_t) (bitsSent * clockPeriod_data
			/ (double) numWavelengths));
	debug(getFullPath(), "msg duration = ", SIMTIME_DBL(msgDuration), UNIT_NIC);

	sendDelayed(stop, msgDuration + extra, pOut[0]);

	SO_bw->track(bitsSent / 1e9);

	simtime_t wait = item->getWaitTime();

	sentItem = item;
	schedule[tdmSlot]->pop_front();

	if (item->done()) {
		if (item->notifyProc()) {

			NetworkAddress* adr;
			if (item->type == ScheduleItem::MEM_WRITE_DATA)
				adr = (NetworkAddress*) item->procData->getDestAddr();
			else
				adr = (NetworkAddress*) item->procData->getSrcAddr();

			if (adr->id[AddressTranslator::convertLevel("NET")] == myId) {
				//notify processor that data was sent
				if (!sentItem->isXY) {
					RouterCntrlMsg* msgSent = new RouterCntrlMsg();
					msgSent->setType(proc_msg_sent);

					int p = adr->id[AddressTranslator::convertLevel("PROC")];
					msgSent->setMsgId(p);
					msgSent->setData(
							(long) (((ApplicationData*) item->procData->getEncapsulatedPacket())->dup()));
					sendDelayed(msgSent, 0, procReqOut);
				}
			}
		}
	}

	return msgDuration + wait + extra;
}

void NIF_PhotonicTDM::complete() {

	debug(getFullPath(), "sent bits = ", bitsSent, UNIT_NIC);

	ScheduleItem* item = sentItem;

	//sentThisSlot = !(item->repeat());
	sentThisSlot = false;

	item->sentThisSlot = true;

	firstInSlot = false;

	simtime_t extra = (simTime() > tdmClock->getArrivalTime() - tdmPeriod
			+ slotWaitTime ? 0 : (tdmClock->getArrivalTime() - tdmPeriod
			+ slotWaitTime - simTime()));

	if (item->done()) {

		if (item->type == ScheduleItem::MEM_LCL_READ || item->type
				== ScheduleItem::MEM_REM_READ || item->type
				== ScheduleItem::MEM_WRITE_DATA) {
			int b = ((ScheduleItem_Mem*) item)->bankId;
			cMessage* pre = new cMessage();
			pre->setKind(b);
			scheduleAt(simTime() + tRP + msgDuration + extra, pre);
		}

		//ScheduleItem* it = schedule[tdmSlot]->front();
		ScheduleItem* it = sentItem;

		ProcessorData* res = it->getRescheduleMsg();
		if (res != NULL) {
			tQueue.push_back(res);
		}
		delete it;

		tryTqueue();


		debug(getFullPath(), "message sending complete.. ", UNIT_PATH);
	} else {

		//std::cout << "here, size = " << item->procData->getSize()
		//		<< ", done = " << item->done() << endl;
		debug(getFullPath(), "more message to send.. ", UNIT_PATH);

		//ScheduleItem* it = schedule[tdmSlot]->front();
		ScheduleItem* it = sentItem;
		//schedule[tdmSlot]->pop_front();
		schedule[it->myTimeSlot]->push_back(it);

		debug(getFullPath(), "size of schedule[tdmslot].. ",
				schedule[tdmSlot]->size(), UNIT_PATH);
	}

	start = NULL;
	stop = NULL;

	middleOfSending = false;

	return;

}

void NIF_PhotonicTDM::netPortMsgArrived(cMessage* cmsg) {
	PhotonicMessage* pmsg = check_and_cast<PhotonicMessage*> (cmsg);
	if (pmsg->getMsgType() == TXstart) {
		debug(getFullPath(), "TXstart received", UNIT_NIC);

		ProcessorData* pdata = (ProcessorData*) pmsg->decapsulate();

		delete pdata;

	} else if (pmsg->getMsgType() == TXstop) {

		debug(getFullPath(), "TXstop received", UNIT_NIC);

		ProcessorData* pdata = check_and_cast<ProcessorData*> (
				pmsg->decapsulate());

		SO_lat_trans->track(SIMTIME_DBL(simTime() - pdata->getSavedTime1()));

		if (pdata->getType() == M_readResponse || pdata->getType() == MPI_send) {
			NetworkAddress* dest = (NetworkAddress*) pdata->getDestAddr();

			if (dest->id[AddressTranslator::convertLevel("NET")] == myId) {

				debug(getFullPath(), "going to proc ",
						dest->id[AddressTranslator::convertLevel("PROC")],
						UNIT_NIC);

				//pdata->setIsComplete(true);
				sendDataToProcessor(pdata);
			} else {

				if (pdata->getIsComplete()) {
					ApplicationData* adata =
							(ApplicationData*) pdata->getEncapsulatedPacket();
					pdata->setSize(adata->getPayloadSize());
					//model xy-buffer power

					pdata->setNifArriveTime(simTime());

					addToSchedule(pdata, true);
				} else
					delete pdata;
			}
		} else {
			//xyBuffer.push_back(pdata);


			if (pdata->getIsComplete()) {
				ApplicationData* adata =
						(ApplicationData*) pdata->getEncapsulatedPacket();
				pdata->setSize(adata->getPayloadSize());

				addToSchedule(pdata, true);
			} else
				delete pdata;
		}

	} else {
		opp_error("optical MsgType not set");
	}
}

void NIF_PhotonicTDM::processorMsgSent(ProcessorData* pdata) {

}

void NIF_PhotonicTDM::selfMsgArrived(cMessage* cmsg) {

	if (cmsg == tdmClock) {
		sentThisSlot = false;

		tdmSlot++;
		if (tdmSlot >= numTDMslots)
			tdmSlot = 0;

		list<ScheduleItem*>::iterator iter;

		for (iter = schedule[tdmSlot]->begin(); iter
				!= schedule[tdmSlot]->end(); iter++) {
			(*iter)->cntrlSetup = false;
			(*iter)->sentThisSlot = false;
		}
		sentCntrl = false;
		firstInSlot = true;

		/*if (tdmSlot % 2 == 0) {
		 rx++;

		 if (rx == tx)
		 rx++;

		 if (rx >= numX) {
		 rx = 0;
		 tx++;

		 if (tx > (numX - 1) / 2) {
		 tx = 0;
		 rx = 1;
		 }
		 }
		 }*/

		if (schedule[tdmSlot]->size() > 0) {
			sendRingCntrl();
		}

		//if (myId == 0)
		//debug(getFullPath(), "Changing TDM slot to ", tdmSlot, UNIT_NIC);
		//debug(getFullPath(), "rx is ", rx, UNIT_NIC);
		//debug(getFullPath(), "tx is ", tx, UNIT_NIC);

		if (!Statistics::isDone)
			scheduleAt(simTime() + tdmPeriod, tdmClock);
		else
			delete tdmClock;

		if (!nextMsg->isScheduled())
			controller();
	} else { //it's a precharge is done message
		int b = cmsg->getKind();
		bankSet[b] = false;

		delete cmsg;

	}

	tryTqueue();

}

void NIF_PhotonicTDM::sendRingCntrl() {

	if (useIOplane) {
		//map<int, PSE_STATE> rings = schedule[tdmSlot]->front()->getRingCntrl();
		list<ScheduleItem*>::iterator iter;
		int bitmap = 0;
		for (iter = schedule[tdmSlot]->begin(); iter
				!= schedule[tdmSlot]->end(); iter++) {
			bitmap = bitmap | (*iter)->getRingCntrl();
			(*iter)->cntrlSetup = true;
		}

		//debug(getFullPath(), "setting ring cntrl bitmap ", bitmap, UNIT_NIC);

		for (int i = 0; i < 6; i++) {
			ElementControlMessage* pse = new ElementControlMessage();
			int rid = pow(2, i);
			int ison = bitmap & rid;
			//debug(getFullPath(), "setting ring rid ", rid, UNIT_NIC);
			//debug(getFullPath(), "setting ring ison ", ison, UNIT_NIC);

			pse->setState((ison > 0) ? PSE_ON : PSE_OFF);
			sendDelayed(pse, 0, pOut[i + 1]);
		}
	}
	sentCntrl = true;
}

void NIF_PhotonicTDM::readControlFile(string fName) {

	ifstream cntrlFile;

	cntrlFile.open(fName.c_str(), ios::in);

	if (!cntrlFile.is_open()) {
		std::cout << "Filename: " << fName << endl;
		opp_error("cannot open cotnrol file");
	}

	//cntrlFile.get();
	char temp[6];
	int tempInt;

	cntrlFile.getline(temp, 6, ',');

	numTDMslots = atoi(temp);

	for (int i = 0; i < numTDMslots; i++) {
		cntrlMatrix[i] = new map<int, int> ();

		for (int j = 0; j < numX * numX; j++) {
			(*cntrlMatrix[i])[j] = -1;
		}

		schedule[i] = new list<ScheduleItem*> ();
	}

	for (int i = 0; i < numX * numX; i++) {
		reverseMatrix[i] = new map<int, int> ();
		for (int j = 0; j < numX * numX; j++) {
			(*reverseMatrix[i])[j] = -1;
		}
	}

	for (int i = 0; i < numX * numX; i++) {
		cntrlFile.getline(temp, 6, ',');
		tempInt = atoi(temp);

		//std::cout << tempInt << endl;
	}

	//now dig in
	for (int i = 0; i < numX * numX; i++) {
		cntrlFile.getline(temp, 6, ',');
		//cntrlFile >> tempInt;
		//cntrlFile.get();
		for (int j = 0; j < numX * numX; j++) {
			cntrlFile.getline(temp, 6, ',');
			//cntrlFile >> tempInt;
			//cntrlFile.get();
			std::cout << "\"" << temp << "\"" << endl;
			string strTemp = temp;

			if (strTemp != "") {
				tempInt = atoi(temp);
				if (tempInt >= numTDMslots) {
					std::cout << "sender = " << j << ", receiver = " << i
							<< ", time slot = " << tempInt << endl;
					opp_error(
							"control matrix is wrong, found higher time slot than expected");
				}
				(*cntrlMatrix[tempInt])[j] = i;
				(*reverseMatrix[j])[i] = tempInt;

				if (i == myId) {
					ScheduleItem* item = new ScheduleItem_Rx();
					if (schedule[tempInt]->size() != 0) {
						opp_error("scheduling more than one receive in a slot");
					}
					schedule[tempInt]->push_back(item);
				}

				//(*printMatrix[j])[i] = tempInt;
				if (myId == 0) {
					std::cout << "setting sender " << j << " to " << i
							<< " at time slot " << tempInt << endl;
				}

			}

		}

	}
	cntrlFile.close();
}

