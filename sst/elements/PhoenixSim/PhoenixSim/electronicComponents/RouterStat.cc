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

#include "RouterStat.h"

Define_Module( RouterStat)
;

double RouterStat::area = 0;
bool RouterStat::areaInit = false;

RouterStat::RouterStat() {
	// TODO Auto-generated constructor stub

}

RouterStat::~RouterStat() {
	// TODO Auto-generated destructor stub
}

void RouterStat::initialize() {

	numVC = par("routerVirtualChannels");
	flit_width = par("electronicChannelWidth");
	numPorts = par("numPorts");
	buffer_size = par("routerBufferSize");
	clockRate = par("clockRate");

	//numRouters++;
	//Statistics::me->registerPart();


	string statName = "Electronic Clock Tree";
	P_static = Statistics::registerStat(statName, StatObject::ENERGY_STATIC,
			"electronic");
	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	area = 0;
	areaInit = false;

	rxArb = false;
	rxXB = false;
	rxIn = false;

	buffType = -1;

}

void RouterStat::finish() {

	area = 0;
	areaInit = false;
}

void RouterStat::handleMessage(cMessage* msg) {

	RouterCntrlMsg *rc = check_and_cast<RouterCntrlMsg*> (msg);

	if (rc->getType() == router_power_arbiter) {
		arbModel = ((ORION_Arbiter*) rc->getData())->model;
		arb_reqWidth = ((ORION_Arbiter*) rc->getData())->req_width;
		rxArb = true;
	} else if (rc->getType() == router_power_crossbar) {
		ORION_Crossbar* xbar = (ORION_Crossbar*) rc->getData();
		xbarModel = xbar->model;
		xbarDegree = xbar->degree;
		rxXB = true;
	} else if (rc->getType() == router_power_inport) {
		ORION_Array* arr = (ORION_Array*) rc->getData();
		buffType = arr->info->arr_buf_type;
		share = arr->info->share_rw;
		data_bitline_pre_model = arr->info->data_bitline_pre_model;
		rows = arr->info->n_set / arr->info->data_ndbl / arr->info->data_nspd;
		data_end = arr->info->data_end;
		rxIn = true;
	}

	if (rxArb && rxXB && rxIn) {
		if (!areaInit) {
			calculateArea();
			areaInit = true;
		}

		P_static->track(getClockPower());
	}

	delete rc;

}

double RouterStat::getClockPower() {
	//#ifdef ORION_VERSION_2
	//clock power for the whole router
	double pmosLeakage = 0;
	double nmosLeakage = 0;
	double H_tree_clockcap = 0;
	double H_tree_resistance = 0;
	double pipereg_clockcap = 0;
	double ClockEnergy = 0;
	double ClockBufferCap = 0;
	double Ctotal = 0;
	int pipeline_regs = 0;
	double energy = 0;

	ORION_Tech_Config *conf = Statistics::DEFAULT_ORION_CONFIG_CNTRL;

	if (conf->PARM("pipelined")) {
		/*pipeline registers after the link traversal stage */
		pipeline_regs += numPorts * flit_width;

		/*pipeline registers for input buffer*/

		if (share) {
			pipeline_regs += numPorts * flit_width;
		} else {
			pipeline_regs += numPorts * flit_width * numVC;
		}

		/*pipeline registers for crossbar*/

		pipeline_regs += numPorts * flit_width * numVC;

		/*for vc allocator and sw allocator, the clock power has been included in the dynamic power part,
		 * so we will not calculate them here to avoid duplication */

		pipereg_clockcap = pipeline_regs * conf->PARM("ClockCap");
	}

	double diagonal = sqrt(2 * area);

	if ((conf->PARM("TRANSISTOR_TYPE")== HVT) ||
			(conf->PARM("TRANSISTOR_TYPE")== NVT)) {
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
		H_tree_clockcap = (8 + 4 + 4 + 4 + 4) * (diagonal * 1e-6) * conf->PARM("Clockwire");
		H_tree_resistance = (8 + 4 + 4 + 4 + 4) * (diagonal * 1e-6) * conf->PARM("Reswire");

		int k;
		double h;
		int *ptr_k = &k;
		double *ptr_h = &h;
		ORION_Link::getOptBuffering(ptr_k, ptr_h, ((8 + 4 + 4 + 4 + 4)
				* (diagonal * 1e-6)), conf);
		ClockBufferCap = ((double) k) * conf->PARM("BufferInputCapacitance") * h;

		pmosLeakage = conf->PARM("BufferPMOSOffCurrent") * h * k * 29;
		nmosLeakage = conf->PARM("BufferNMOSOffCurrent") * h * k * 29;
	}

	pipereg_clockcap = 4 * flit_width * conf->PARM("ClockCap");

	/* total dynamic energy for clock */
	Ctotal = pipereg_clockcap + H_tree_clockcap + ClockBufferCap;
	ClockEnergy = Ctotal * conf->PARM("EnergyFactor") * clockRate;
	energy += ClockEnergy;

	/* total leakage current for clock */
	/* Here we divide ((pmosLeakage + nmosLeakage) / 2) by SCALE_S is because pmosLeakage and nmosLeakage value is
	 * already for the specified technology, doesn't need to use scaling factor. So we divide SCALE_S here first since
	 * the value will be multiplied by SCALE_S later */
	double I_clock_static = (((pmosLeakage + nmosLeakage) / 2) / conf->PARM("SCALE_S")
			+ (pipeline_regs * TABS::DFF_TAB[0] * conf->PARM("Wdff")));
	energy += I_clock_static * conf->PARM("Vdd") * conf->PARM("SCALE_S");

	return energy;
	//#endif
}

void RouterStat::calculateArea() {
	//#ifdef ORION_VERSION_2
	double bitline_len, wordline_len, xb_in_len, xb_out_len;
	double depth, nMUX, boxArea;
	int req_width;

	int n_v_channel = numVC;

	ORION_Tech_Config *conf = Statistics::DEFAULT_ORION_CONFIG_CNTRL;

	/* buffer area */
	/* input buffer area */
	switch (buffType) {
	case SRAM:
		bitline_len = (buffer_size / flit_width) * (conf->PARM("RegCellHeight") + 2
				* conf->PARM("WordlineSpacing"));
		wordline_len = flit_width * (conf->PARM("RegCellWidth") + 2 * (1 + 1)
				* conf->PARM("BitlineSpacing"));

		/* input buffer area */
		area += numPorts * (bitline_len * wordline_len) * (share ? 1
				: n_v_channel);
		break;

	case REGISTER:
		area += conf->PARM("AreaDFF") * numPorts * flit_width * (buffer_size / flit_width)
				* (share ? 1 : n_v_channel);
		break;

	default:
		break;
	}

	/* crossbar area */
	if (xbarModel && xbarModel < CROSSBAR_MAX_MODEL) {
		switch (xbarModel) {
		case MATRIX_CROSSBAR:
			xb_in_len = numPorts * flit_width * conf->PARM("CrsbarCellWidth");
			xb_out_len = numPorts * flit_width * conf->PARM("CrsbarCellHeight");
			area += xb_in_len * xb_out_len;
			break;

		case MULTREE_CROSSBAR:
			if (xbarDegree == 2) {
				depth = ceil((log(numPorts) / log(2)));
				nMUX = pow(2, depth) - 1;
				boxArea = 1.5 * nMUX * conf->PARM("AreaMUX2");
				area += numPorts * flit_width * boxArea * numPorts;
			} else if (xbarDegree == 3) {
				depth = ceil((log(numPorts) / log(3)));
				nMUX = ((pow(3, depth) - 1) / 2);
				boxArea = 1.5 * nMUX * conf->PARM("AreaMUX3");
				area += numPorts * flit_width * boxArea * numPorts;
			} else if (xbarDegree == 4) {
				depth = ceil((log(numPorts) / log(4)));
				nMUX = ((pow(4, depth) - 1) / 3);
				boxArea = 1.5 * nMUX * conf->PARM("AreaMUX4");
				area += numPorts * flit_width * boxArea * numPorts;
			}
			break;

		default:
			printf("error\n"); /* some error handler */

		}
	}

	/* switch allocator area */
	if (arbModel) {
		req_width = arb_reqWidth;
		switch (arbModel) {
		case MATRIX_ARBITER:
			area += ((conf->PARM("AreaNOR") * 2 * (req_width - 1) * req_width) +
					(conf->PARM("AreaINV")
			* req_width) + (conf->PARM("AreaDFF")
			* (req_width * (req_width - 1) / 2))) * numPorts;
			break;

		case RR_ARBITER:
			area += ((6 * req_width * conf->PARM("AreaNOR")) + (2 * req_width * conf->PARM("AreaINV"))
					+ (req_width * conf->PARM("AreaDFF"))) * 1.3 * numPorts;
			break;

		case QUEUE_ARBITER:
			area += conf->PARM("AreaDFF") * buffer_size * numPorts;

			break;

		default:
			printf("error\n"); /* some error handler */
		}
	}
	//#endif

}

