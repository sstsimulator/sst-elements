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

#include "DRAM_Bank.h"

Define_Module( DRAM_Bank)
;

DRAM_Bank::DRAM_Bank() {
	// TODO Auto-generated constructor stub

}

DRAM_Bank::~DRAM_Bank() {
	// TODO Auto-generated destructor stub
}

void DRAM_Bank::initialize() {

	rows = par("DRAM_rows");
	cols = par("DRAM_cols");
	arrays = par("DRAM_arrays");
	chips = par("DRAM_chipsPerDIMM");
	banks = par("DRAM_banksPerChip");

	freq = par("DRAM_freq");

	tRCD = par("DRAM_tCAS"); //in clk cycles
	tCL = par("DRAM_tCL");
	tRP = par("DRAM_tRP");

	tRCD /= (freq / 2);
	tCL /= (freq / 2);
	tRP /= (freq / 2);

	//std::cout << "tRCD: " << tRCD << ", tCL: " << tCL << ", tRP: " << tRP << endl;

	cntrlIn = gate("cntrlIn");
	dataIn = gate("dataIn");
	dataOut = gate("dataOut");

	currentOpenRow = -1;
	currentOpenCol = -1;

	precharging = false;
	prechargeMsg = new cMessage();
	burst = cols;

	id = par("id");

	SO_read = Statistics::registerStat("Bits Read", StatObject::TOTAL, "DRAM");
	SO_write = Statistics::registerStat("Bits Written", StatObject::TOTAL,
			"DRAM");
	SO_BW = Statistics::registerStat("Bandwidth (Gb/s)", StatObject::TIME_AVG,
			"DRAM");

	writing = false;
	writingMsg = new cMessage();
}

void DRAM_Bank::finish() {
	cancelAndDelete(prechargeMsg);
	cancelAndDelete(writingMsg);
}

void DRAM_Bank::handleMessage(cMessage* msg) {

	if (msg->isSelfMessage()) {
		if (msg == prechargeMsg) {
			precharging = false;
			currentOpenRow = -1;
		} else if (msg == writingMsg) {
			writing = false;
			precharging = true;
			currentOpenCol = -1;
			scheduleAt(simTime() + tRP, prechargeMsg);
		}

	} else if (msg->getArrivalGate() == cntrlIn) {

		DRAM_CntrlMsg* dmsg = check_and_cast<DRAM_CntrlMsg*> (msg);

		if (dmsg->getBank() == id) {
			if (precharging || writing) {
				opp_error(
						"DRAM control msg to a bank that is writing or precharging.");
			}

			if (dmsg->getType() == Row_Access) {
				int r = dmsg->getRow();
				if (currentOpenRow >= 0) {
					std::cout << "time: " << simTime() << endl;
					std::cout << "write finish time: "
							<< writingMsg->getArrivalTime() << endl;
					std::cout << "currently open row: " << currentOpenRow << endl;
					opp_error("DRAM_Bank: row access with a row already open");
				} else if (r > rows) {
					opp_error(
							"DRAM_Bank: row access greater than number of rows");
				} else {
					currentOpenRow = r;
					coreAddr = (NetworkAddress*) (dmsg->getCoreAddr());

				}

				debug(getFullPath(), "Row access at bank ", id, UNIT_DRAM);

			} else if (dmsg->getType() == Col_Access) {
				int c = dmsg->getCol();
				burst = dmsg->getBurst();
				if (currentOpenRow < 0) {
					opp_error(
							"DRAM_Bank: col access attempted with no open row");
				} else if (c > cols) {
					opp_error(
							"DRAM_Bank: col access greater than number of cols");
				} else {
					currentOpenCol = dmsg->getCol();
					//burst = dmsg->getBurst();
					lastAccess = dmsg->getLastAccess();

					debug(getFullPath(), "Col access at bank ", id, UNIT_DRAM);
					debug(getFullPath(), "burst length ", burst, UNIT_DRAM);

					if (burst > cols - currentOpenCol) {
						opp_error(
								"DRAM_Bank: can't burst past the end of the row");
					}

					if (!dmsg->getWriteEn()) {
						MemoryAccess* memA = (MemoryAccess*)dmsg->getData();

						ProcessorData *pdata = new ProcessorData();
						pdata->setIsComplete(lastAccess);
						pdata->setDestAddr((long) (lastAccess ? coreAddr->dup()
								: coreAddr->dup()));
						pdata->setSrcAddr((long) coreAddr->dup());
						pdata->setSize(burst * arrays * chips);
						pdata->setType(M_readResponse);

						MemoryAccess* data = new MemoryAccess();
						data->setAccessSize(burst * arrays * chips);
						data->setIsComplete(lastAccess);
						data->setPayloadSize(burst * arrays * chips);
						data->setCreationTime(dmsg->getCreationTime());
						data->setType(M_readResponse);

						data->setPayloadArraySize(memA->getPayloadArraySize());
						for(int i = 0; i < memA->getPayloadArraySize(); i++){
							data->setPayload(i, memA->getPayload(i));
						}

						pdata->encapsulate(data);

						sendDelayed(pdata, 0, dataOut);


						debug(getFullPath(), "Returning read data at bank  ",
								id, UNIT_DRAM);
						precharging = true;
						scheduleAt(simTime() + tRP, prechargeMsg);

						SO_read->track(burst * arrays);
						SO_BW->track(burst * arrays / 1e9);

						currentOpenCol = -1;

					}

				}
			} else if (dmsg->getType() == Write_Data) {
				if (dmsg->getBank() == id) {
					if (precharging) {
						opp_error(
								"Attempting to write to a DRAM bank that is precharging.");
					}

					//writing = true;
					//simtime_t accessTime = burst / freq;
					//scheduleAt(simTime() + accessTime, writingMsg);

					debug(getFullPath(), "Writing data at bank  ", id,
							UNIT_DRAM);
					SO_write->track(burst * arrays);
					SO_BW->track(burst * arrays / 1e9);

					currentOpenCol = -1;
					precharging = true;
					scheduleAt(simTime() + tRP, prechargeMsg);

				}
			}
		}

		delete msg;

	} else if (msg->getArrivalGate() == dataIn) {

		if (writing) {

			std::cout << "time: " << simTime() << endl;
			std::cout << "write finish time: " << writingMsg->getArrivalTime()
					<< endl;
			opp_error(
					"Attempting to write to a DRAM bank while the data bus is already in use.");
		}
		//ProcessorData *proc = check_and_cast<ProcessorData*> (msg);
		//MemoryAccess* mem = check_and_cast<MemoryAccess*> (proc->decapsulate());
		MemoryAccess* mem = check_and_cast<MemoryAccess*>(msg);

		if (mem->getBank() == id) {

			if (precharging) {
				opp_error(
						"Attempting to write to a DRAM bank that is precharging.");
			}

			//writing = true;
			//simtime_t accessTime = burst / freq;
			//scheduleAt(simTime() + accessTime, writingMsg);

			currentOpenCol = -1;
			precharging = true;
			scheduleAt(simTime() + tRP, prechargeMsg);


			debug(getFullPath(), "Writing data at bank  ", id, UNIT_DRAM);
			SO_write->track(burst * arrays);
			SO_BW->track(burst * arrays / 1e9);

		}

		delete mem;
		//delete msg;

	}

}
