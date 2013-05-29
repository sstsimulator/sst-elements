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

#include "TDM_Switch_Controller.h"

Define_Module(TDM_Switch_Controller)
;

TDM_Switch_Controller::TDM_Switch_Controller() {
	// TODO Auto-generated constructor stub

}

TDM_Switch_Controller::~TDM_Switch_Controller() {
	// TODO Auto-generated destructor stub
}

void TDM_Switch_Controller::initialize() {

	numX = par("numOfNodesX");
	numY = numX;
	id = par("id");

	myRow = id / numX;
	myCol = id % numX;

	numTDMslots = (numX - 1) * (numX / 2);

	numDevices = par("numDevices");

	for (int i = 0; i < numDevices; i++) {
		devicePorts[i] = gate("deviceCntrl", i);
	}

	clockPeriod = par("tdmSlotPeriod");
	clockMsg = new cMessage();

	rx = 1;
	tx = 0;
	tdmSlot = 0;
	scheduleAt(0, clockMsg);

	string cntrlFileName = par("cntrlFileName");
	readControlFile(cntrlFileName);

#ifdef ENABLE_ORION
	info = new ORION_Array_Info();

	int buf_type = SRAM;
	int is_fifo = true;
	int n_read_port = 1;
	int n_write_port = 0;
	int n_entry = numTDMslots;
	int line_width = 8;
	int outdrv = false;

	info->init(buf_type, is_fifo, n_read_port, n_write_port, n_entry,
			line_width, outdrv, Statistics::DEFAULT_ORION_CONFIG_CNTRL);

	power = new ORION_Array();
	power->init(info);

	string statName = "TDM Control LUT";
	P_static = Statistics::registerStat(statName, StatObject::ENERGY_STATIC,
			"electronic");
	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	lutEnergy = power->stat_energy(1, 0, 0);

	P_static->track(power->stat_static_power());
	P_static->track(getClockPower());

#endif
}

double TDM_Switch_Controller::getClockPower() {

	//#ifdef ORION_VERSION_2
		double bitline_len, wordline_len;

		double pmosLeakage = 0;
		double nmosLeakage = 0;
		double H_tree_clockcap = 0;
		double H_tree_resistance = 0;
//		double pipereg_clockcap = 0;
		double ClockEnergy = 0;
		double ClockBufferCap = 0;
		double Ctotal = 0;
//		int pipeline_regs = 0;
		double energy = 0;

		double area = 0;

		/* buffer area */
		/* input buffer area */

		ORION_Tech_Config *conf = Statistics::DEFAULT_ORION_CONFIG_CNTRL;

		bitline_len = numTDMslots * (conf->PARM("RegCellHeight") + 2 * conf->PARM("WordlineSpacing"));
		wordline_len = numDevices * (conf->PARM("RegCellWidth") + 2 * (1 + 1) * conf->PARM("BitlineSpacing"));

		/* input buffer area */
		area += 1 * (bitline_len * wordline_len);

		double diagonal = sqrt(2 * area);

		if ((conf->PARM("TRANSISTOR_TYPE")== HVT) || (conf->PARM("TRANSISTOR_TYPE")== NVT)) {
			H_tree_clockcap = (4 + 4 + 2 + 2) * (diagonal * 1e-6) * (conf->PARM("Clockwire"));
			H_tree_resistance = (4 + 4 + 2 + 2) * (diagonal * 1e-6) * (conf->PARM("Reswire"));

			int k;
			double h;
			int *ptr_k = &k;
			double *ptr_h = &h;
			ORION_Link::getOptBuffering(ptr_k, ptr_h, ((4 + 4 + 2 + 2) * (diagonal
					* 1e-6)), conf);
			ClockBufferCap = ((double) k) * (conf->PARM("ClockCap")) * h;

			pmosLeakage = conf->PARM("BufferPMOSOffCurrent") * h * k * 15;
			nmosLeakage = conf->PARM("BufferNMOSOffCurrent") * h * k * 15;
		} else if (conf->PARM("TRANSISTOR_TYPE")== LVT) {
			H_tree_clockcap = (8 + 4 + 4 + 4 + 4) * (diagonal * 1e-6) * (conf->PARM("Clockwire"));
			H_tree_resistance = (8 + 4 + 4 + 4 + 4) * (diagonal * 1e-6) * (conf->PARM("Reswire"));

			int k;
			double h;
			int *ptr_k = &k;
			double *ptr_h = &h;
			ORION_Link::getOptBuffering(ptr_k, ptr_h, ((8 + 4 + 4 + 4 + 4)
					* (diagonal * 1e-6)), conf);
			ClockBufferCap = ((double) k) * (conf->PARM("BufferInputCapacitance")) * h;

			pmosLeakage = conf->PARM("BufferPMOSOffCurrent") * h * k * 29;
			nmosLeakage = conf->PARM("BufferNMOSOffCurrent") * h * k * 29;
		}

		/* total dynamic energy for clock */
		Ctotal = H_tree_clockcap + ClockBufferCap;
		ClockEnergy = Ctotal * conf->PARM("EnergyFactor") * (1.0 / clockPeriod);
		energy += ClockEnergy;

		/* total leakage current for clock */
		/* Here we divide ((pmosLeakage + nmosLeakage) / 2) by SCALE_S is because pmosLeakage and nmosLeakage value is
		 * already for the specified technology, doesn't need to use scaling factor. So we divide SCALE_S here first since
		 * the value will be multiplied by SCALE_S later */
		double I_clock_static = (((pmosLeakage + nmosLeakage) / 2) / conf->PARM("SCALE_S"));
		energy += I_clock_static * conf->PARM("Vdd") * conf->PARM("SCALE_S");

		return energy;
}
void TDM_Switch_Controller::finish() {
	if (clockMsg->isScheduled()) {
		cancelEvent(clockMsg);
	}
	delete clockMsg;
}

void TDM_Switch_Controller::handleMessage(cMessage* cmsg) {

	if (cmsg == clockMsg) {
		map<PORTS, PORTS> setup;

		//std::cout << "TDM slot: " << tdmSlot << endl;

		map<int, int>::iterator iter;

		for (iter = cntrlMatrix[tdmSlot]->begin(); iter
				!= cntrlMatrix[tdmSlot]->end(); iter++) {
			if (id == iter->first) { //i'm a sender
				if (iter->second >= 0) {
					if (iter->second % numX == id % numX) { // same col
						if (iter->second > id) {
							setup[MOD] = S;
						} else {
							setup[MOD] = N;
						}

					} else {
						if (iter->second % numX > id % numX) {
							setup[MOD] = E;
						} else {
							setup[MOD] = W;
						}

					}
				}

			} else if (id == iter->second) { //i'm a receiver
				if (iter->first / numX == id / numX) { // same row
					if (iter->first > id) {
						setup[E] = DET;
					} else {
						setup[W] = DET;
					}
				} else {
					if (iter->first / numX > id / numX) {
						setup[S] = DET;
					} else {
						setup[N] = DET;
					}

				}

			}

			//i'm the turning point from x to y
			if (iter->second % numX != iter->first % numX &&
					iter->second / numX != iter->first / numX) { //not the same column
				if (iter->second % numX == id % numX && iter->first / numX
						== id / numX) { //it's me

					if (iter->second % numX > iter->first % numX) { //coming from W
						if (iter->second / numX > iter->first / numX) {//going S
							setup[W] = S;
						} else {
							setup[W] = N;
						}

					} else { //coming from E
						if (iter->second / numX > iter->first / numX) {//going S
							setup[E] = S;
						} else {
							setup[E] = N;
						}
					}
				}

			}

		}

		sendCntrlMessages(setup);
		E_dynamic->track(lutEnergy);

		tdmSlot++;
		if (tdmSlot >= numTDMslots)
			tdmSlot = 0;

		if (!Statistics::isDone)
			scheduleAt(simTime() + clockPeriod, clockMsg);
	}

}

void TDM_Switch_Controller::sendCntrlMessages(map<PORTS, PORTS> set) {

	map<PORTS, PORTS>::iterator iter;
	map<int, PSE_STATE> deviceMap;
	for (int i = 0; i < numDevices; i++) {
		deviceMap[i] = PSE_OFF;
	}

	for (iter = set.begin(); iter != set.end(); iter++) {
		int device = -1;
		if (iter->first == MOD) {
			device = iter->second;

		} else if (iter->second == DET) {
			device = 4 + iter->first;
		} else {
			if (iter->first == W && iter->second == N)
				device = 8;
			else if (iter->first == E && iter->second == N)
				device = 9;
			else if (iter->first == E && iter->second == S)
				device = 10;
			else if (iter->first == W && iter->second == S)
				device = 11;

		}

		deviceMap[device] = PSE_ON;

		debug(getFullPath(), "Turning on PSE ", device, UNIT_PATH_SETUP);
	}

	for (int i = 0; i < numDevices; i++) {
		ElementControlMessage* pse = new ElementControlMessage();
		pse->setState(deviceMap[i]);
		send(pse, devicePorts[i]);

	}

}

void TDM_Switch_Controller::readControlFile(string fName) {

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
	}

	for (int i = 0; i < numX * numX; i++) {
		printMatrix[i] = new map<int, int> ();

		for (int j = 0; j < numX * numX; j++) {
			(*printMatrix[i])[j] = -1;
		}
	}

	for (int i = 0; i < numX * numY; i++) {
		cntrlFile.getline(temp, 6, ',');
		tempInt = atoi(temp);

		//std::cout << tempInt << endl;
	}

	//now dig in
	for (int i = 0; i < numX * numY; i++) {
		cntrlFile.getline(temp, 6, ',');
		for (int j = 0; j < numX * numY; j++) {
			cntrlFile.getline(temp, 6, ',');
			//std::cout << temp << endl;
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
				(*printMatrix[j])[i] = tempInt;
				//std::cout << "setting sender " << j << " to " << i
				//		<< " at time slot " << tempInt << endl;
			}
		}

	}

	//print out the control matrix
	/*for (int i = 0; i < numX * numY; i++) {
	 std::cout << endl;
	 for (int j = 0; j < numX * numY; j++) {
	 int slot = (*printMatrix[j])[i];
	 if(slot >= 0)
	 std::cout << slot << ",";
	 else
	 std::cout << " ,";
	 }
	 }

	 std::cout << endl;*/

	cntrlFile.close();
}

