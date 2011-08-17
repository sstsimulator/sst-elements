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

#include "OCM_Control.h"

Define_Module( OCM_Control)
;

OCM_Control::OCM_Control() {
	// TODO Auto-generated constructor stub

}

OCM_Control::~OCM_Control() {
	// TODO Auto-generated destructor stub
}

void OCM_Control::initialize() {

	phIn = gate("fromChip");
	phOut = gate("toChip");

	double temp = par("DRAM_freq");
	clockPeriod = 1.0 / temp;

	chips = par("DRAM_chipsPerDIMM");
	arrays = par("DRAM_arrays");

	dataOut = gate("toDRAM");
	dataIn = gate("fromDRAM");
	cntrlOut = gate("cntrl");

	freq = par("DRAM_freq");

	tRCD = par("DRAM_tCAS"); //in clk cycles
	tCL = par("DRAM_tCL");
	tRP = par("DRAM_tRP");

	tRCD /= (freq / 2);
	tCL /= (freq / 2);
	tRP /= (freq / 2);

	pwr_idle_down = .0394 * chips;
	pwr_active_down = 0.0788 * chips;

	pwr_idle_standby = .1024 * chips;
	pwr_active_standby = 0.126 * chips;

	pwr_dq = 0.001 * chips * arrays;

	pwr_row_cmd = (.0849 + (pwr_active_standby - pwr_active_down)) * chips;
	pwr_read = (.3386 + (pwr_active_standby - pwr_active_down)) * chips;
	pwr_write = (.4174 + (pwr_active_standby - pwr_active_down)) * chips;

	SO_EN_read = Statistics::registerStat("Read Energy",
			StatObject::ENERGY_EVENT, "DRAM");
	SO_EN_write = Statistics::registerStat("Write Energy",
			StatObject::ENERGY_EVENT, "DRAM");
	SO_EN_act = Statistics::registerStat("Activate Energy",
			StatObject::ENERGY_EVENT, "DRAM");
	SO_EN_back = Statistics::registerStat("Static Energy",
			StatObject::ENERGY_EVENT, "DRAM");
	SO_EN_pre = Statistics::registerStat("Precharge Energy",
			StatObject::ENERGY_EVENT, "DRAM");
	SO_EN_dq = Statistics::registerStat("IO Pin Energy",
			StatObject::ENERGY_EVENT, "DRAM");

	activeBanks = 0;
	lastStaticTime = 0;

	rowRx = false;
	colRx = false;
	bankId = -1;
	connectedto = -1;

	temp = par("O_frequency_data");
	photonicBitPeriod = 1.0 / temp;
	wavelengths = par("numOfWavelengthChannels");

	dataRx = 0;

	bankId = -1;

}

void OCM_Control::finish() {

}

void OCM_Control::handleMessage(cMessage* msg) {

	if (msg->getArrivalGate() == phIn) {

		ProcessorData *proc = check_and_cast<ProcessorData*> (msg);

		if (proc->getDataType() == 0) {
			DRAM_CntrlMsg* dmsg = check_and_cast<DRAM_CntrlMsg*> (proc);

			bankId = dmsg->getBank();

			int bank = dmsg->getBank();

			if (activeBanks > 0) {
				SO_EN_back ->track(pwr_active_down
						* SIMTIME_DBL(simTime() - lastStaticTime));
			} else {
				SO_EN_back ->track(pwr_idle_down
						* SIMTIME_DBL(simTime() - lastStaticTime));
			}

			lastStaticTime = simTime();

			if (dmsg->getType() == Row_Access) {
				activeBanks = activeBanks | (int) (pow(2, bank));
				SO_EN_act->track(pwr_row_cmd * SIMTIME_DBL(tRCD));
			} else if (dmsg->getType() == Col_Access && dmsg->getWriteEn()
					== false) { //read
				int size = dmsg->getBurst();
				SO_EN_read->track(pwr_read * size / freq);
				SO_EN_pre->track(pwr_row_cmd * SIMTIME_DBL(tRP));
				SO_EN_dq->track(pwr_dq * size / freq);
				activeBanks = activeBanks & (0xFFFF ^ (int) pow(2, bank));
			} else if (dmsg->getType() == Write_Data && dmsg->getWriteEn()
					== true) { //read
				int size = dmsg->getBurst();
				SO_EN_write->track(pwr_write * size / freq);
				SO_EN_pre->track(pwr_row_cmd * SIMTIME_DBL(tRP));
				SO_EN_dq->track(pwr_dq * size / freq);
				activeBanks = activeBanks & (0xFFFF ^ (int) pow(2, bank));
			}

			sendDelayed(dmsg, clockPeriod, cntrlOut);

		} else {
			MemoryAccess* mem = (MemoryAccess*) (proc->decapsulate());
			mem->setBank(bankId);
			sendDelayed(mem, clockPeriod, dataOut);
			delete proc;
		}

	} else if (msg->getArrivalGate() == dataIn) {
		dataRx++;

		if (dataRx == chips) {

			ProcessorData* pdata = check_and_cast<ProcessorData*> (msg);

			debug(getFullPath(), "sending data back to ",
					AddressTranslator::untranslate_str(
							(NetworkAddress*) pdata->getDestAddr()), UNIT_DRAM);

			sendDelayed(pdata, 0, phOut);


			dataRx = 0;

			rowRx = false;
			colRx = false;
		} else {
			delete msg;
		}

	}

}

void OCM_Control::trackPower() {

}

