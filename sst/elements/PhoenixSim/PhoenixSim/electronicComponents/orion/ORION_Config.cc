#include "ORION_Config.h"

double TABS::NMOS_TAB[1];
double TABS::PMOS_TAB[1];
double TABS::NAND2_TAB[4];
double TABS::NOR2_TAB[4];
double TABS::DFF_TAB[1];

ORION_Tech_Config::ORION_Tech_Config(int tech, int type, double volt,
		double freq) {
	technology = tech;
	trans_type = (TRANS_TYPES) type;
	voltage = volt;
	frequency = freq;

	initParams();
	initTABS();
}

double ORION_Tech_Config::PARM(string name) {

	if (name == "TECH_POINT") {
		return technology;
	} else if (name == "Freq") {
		return frequency;
	} else if (name == "Vdd") {
		return voltage;
	} else if (name == "TRANSISTOR_TYPE") {
		return trans_type;
	} else if (params.find(name) == params.end()) {
		std::cout << "Param " + name + " not found" << endl;
		throw "Param " + name + " not found";
	}

	return params[name];
}

void ORION_Tech_Config::initTABS() {

	if (PARM("TECH_POINT") == 90 && PARM("TRANSISTOR_TYPE") == LVT) {
		TABS::NMOS_TAB[0] = 19.9e-9;
		TABS::PMOS_TAB[0] = 16.6e-9;

		TABS::NAND2_TAB[0] = 7.8e-9;
		TABS::NAND2_TAB[1] = 24.6e-9;
		TABS::NAND2_TAB[2] = 14.1e-9;
		TABS::NAND2_TAB[3] = 34.3e-9;

		TABS::NOR2_TAB[0] = 51.2e-9;
		TABS::NOR2_TAB[1] = 23.9e-9;
		TABS::NOR2_TAB[2] = 19.5e-9;
		TABS::NOR2_TAB[3] = 8.4e-9;

		TABS::DFF_TAB[0] = 219.7e-9;
	} else if (PARM("TECH_POINT") == 90 && PARM("TRANSISTOR_TYPE") == NVT) {
		TABS::NMOS_TAB[0] = 15.6e-9;
		TABS::PMOS_TAB[0] = 11.3e-9;

		TABS::NAND2_TAB[0] = 2.8e-9;
		TABS::NAND2_TAB[1] = 19.6e-9;
		TABS::NAND2_TAB[2] = 10.4e-9;
		TABS::NAND2_TAB[3] = 29.3e-9;

		TABS::NOR2_TAB[0] = 41.5e-9;
		TABS::NOR2_TAB[1] = 13.1e-9;
		TABS::NOR2_TAB[2] = 14.5e-9;
		TABS::NOR2_TAB[3] = 1.4e-9;

		TABS::DFF_TAB[0] = 194.7e-9;
	} else if (PARM("TECH_POINT") == 90 && PARM("TRANSISTOR_TYPE") == HVT) {
		TABS::NMOS_TAB[0] = 12.2e-9;
		TABS::PMOS_TAB[0] = 9.3e-9;

		TABS::NAND2_TAB[0] = 1.8e-9;
		TABS::NAND2_TAB[1] = 12.4e-9;
		TABS::NAND2_TAB[2] = 8.9e-9;
		TABS::NAND2_TAB[3] = 19.3e-9;

		TABS::NOR2_TAB[0] = 29.5e-9;
		TABS::NOR2_TAB[1] = 8.3e-9;
		TABS::NOR2_TAB[2] = 11.1e-9;
		TABS::NOR2_TAB[3] = 0.9e-9;

		TABS::DFF_TAB[0] = 194.7e-9;
	} else if (PARM("TECH_POINT") <= 65 && PARM("TRANSISTOR_TYPE") == LVT) {
		TABS::NMOS_TAB[0] = 311.7e-9;
		TABS::PMOS_TAB[0] = 674.3e-9;

		TABS::NAND2_TAB[0] = 303.0e-9;
		TABS::NAND2_TAB[1] = 423.0e-9;
		TABS::NAND2_TAB[2] = 498.3e-9;
		TABS::NAND2_TAB[3] = 626.3e-9;

		TABS::NOR2_TAB[0] = 556.0e-9;
		TABS::NOR2_TAB[1] = 393.7e-9;
		TABS::NOR2_TAB[2] = 506.7e-9;
		TABS::NOR2_TAB[3] = 369.7e-9;

		TABS::DFF_TAB[0] = 970.4e-9;
	} else if (PARM("TECH_POINT") <= 65 && PARM("TRANSISTOR_TYPE") == NVT) {
		TABS::NMOS_TAB[0] = 115.1e-9;
		TABS::PMOS_TAB[0] = 304.8e-9;

		TABS::NAND2_TAB[0] = 111.4e-9;
		TABS::NAND2_TAB[1] = 187.2e-9;
		TABS::NAND2_TAB[2] = 230.7e-9;
		TABS::NAND2_TAB[3] = 306.9e-9;

		TABS::NOR2_TAB[0] = 289.7e-9;
		TABS::NOR2_TAB[1] = 165.7e-9;
		TABS::NOR2_TAB[2] = 236.9e-9;
		TABS::NOR2_TAB[3] = 141.4e-9;

		TABS::DFF_TAB[0] = 400.3e-9;
	} else if (PARM("TECH_POINT") <= 65 && PARM("TRANSISTOR_TYPE") == HVT) {
		TABS::NMOS_TAB[0] = 18.4e-9;
		TABS::PMOS_TAB[0] = 35.2e-9;

		TABS::NAND2_TAB[0] = 19.7e-9;
		TABS::NAND2_TAB[1] = 51.3e-9;
		TABS::NAND2_TAB[2] = 63.0e-9;
		TABS::NAND2_TAB[3] = 87.6e-9;

		TABS::NOR2_TAB[0] = 23.4e-9;
		TABS::NOR2_TAB[1] = 37.6e-9;
		TABS::NOR2_TAB[2] = 67.9e-9;
		TABS::NOR2_TAB[3] = 12.3e-9;

		TABS::DFF_TAB[0] = 231.3e-9;
	} else if (PARM("TECH_POINT") >= 110) {
		/* HACK: we assume leakage power above 110nm is small, so we don't provide leakage power for 110nm and above */
		TABS::NMOS_TAB[0] = 0;
		TABS::PMOS_TAB[0] = 0;

		TABS::NAND2_TAB[0] = 0;
		TABS::NAND2_TAB[1] = 0;
		TABS::NAND2_TAB[2] = 0;
		TABS::NAND2_TAB[3] = 0;

		TABS::NOR2_TAB[0] = 0;
		TABS::NOR2_TAB[1] = 0;
		TABS::NOR2_TAB[2] = 0;
		TABS::NOR2_TAB[3] = 0;

		TABS::DFF_TAB[0] = 0;
	}

}

void ORION_Tech_Config::initParams() {

	params["Powerfactor"] = (frequency * voltage * voltage);
	params["EnergyFactor"] = (voltage * voltage);
	params["Period"] = ((double) 1 / frequency);
	/* router module parameters */

	/* virtual channel parameters */
	params["v_class"] = 1; /* # of total message classes */
	params["v_channel"] = 4; /* # of virtual channels per virtual message class*/
	params["cache_class"] = 0; /* # of cache port virtual classes */
	params["mc_class"] = 0; /* # of memory controller port virtual classes */
	params["io_class"] = 0; /* # of I/O device port virtual classes */
	/* ?? */
	params["in_share_buf"] = 0; /* do input virtual channels physically share buffers? */
	params["out_share_buf"] = 0; /* do output virtual channels physically share buffers? */
	/* ?? */
	params["in_share_switch"] = 1; /* do input virtual channels share crossbar input ports? */
	params["out_share_switch"] = 1; /* do output virtual channels share crossbar output ports? */

	/* HACK HACK HACK */
	params["exp_xb_model"] = SIM_NO_MODEL; /* the other parameter is MATRIX_CROSSBAR */
	params["exp_in_seg"] = 2;
	params["exp_out_seg"] = 2;

	/* input buffer parameters */

	/* array parameters shared by various buffers */
	params["wordline_model"] = CACHE_RW_WORDLINE;
	params["bitline_model"] = RW_BITLINE;
	params["mem_model"] = NORMAL_MEM;
	params["row_dec_model"] = GENERIC_DEC;
	params["row_dec_pre_model"] = SINGLE_OTHER;
	params["col_dec_model"] = SIM_NO_MODEL;
	params["col_dec_pre_model"] = SIM_NO_MODEL;
	params["mux_model"] = SIM_NO_MODEL;
	params["outdrv_model"] = REG_OUTDRV;

	/* these 3 should be changed together */
	/* use double-ended bitline because the array is too large */
	params["data_end"] = 2;
	params["amp_model"] = GENERIC_AMP;
	params["bitline_pre_model"] = EQU_BITLINE;
	//params["data_end			1
	//params["amp_model		SIM_NO_MODEL
	//params["bitline_pre_model	SINGLE_OTHER


	/* virtual channel allocator arbiter parameters */
	params["vc_allocator_type"] = TWO_STAGE_ARB; /*vc allocator type, ONE_STAGE_ARB, TWO_STAGE_ARB, VC_SELECT*/
	params["vc_in_arb_model"] = RR_ARBITER; /*input side arbiter model type for TWO_STAGE_ARB. MATRIX_ARBITER, RR_ARBITER, QUEUE_ARBITER*/
	params["vc_in_arb_ff_model"] = NEG_DFF; /* input side arbiter flip-flop model type */
	params["vc_out_arb_model"] = RR_ARBITER; /*output side arbiter model type (for both ONE_STAGE_ARB and TWO_STAGE_ARB). MATRIX_ARBITER, RR_ARBITER, QUEUE_ARBITER */
	params["vc_out_arb_ff_model"] = NEG_DFF; /* output side arbiter flip-flop model type */
	params["vc_select_buf_type"] = REGISTER; /* vc_select buffer type, SRAM or REGISTER */

	/*link wire parameters*/
	params["WIRE_LAYER_TYPE"] = INTERMEDIATE; /*wire layer type, INTERMEDIATE or GLOBAL*/
	params["width_spacing"] = DWIDTH_DSPACE; /*choices are SWIDTH_SSPACE, SWIDTH_DSPACE, DWIDTH_SSPACE, DWIDTH_DSPACE*/
	params["buffering_scheme"] = MIN_DELAY; /*choices are MIN_DELAY, STAGGERED */
	params["shielding"] = FALSE; /*choices are TRUE, FALSE */

	/*clock power parameters*/
	params["pipelined"] = 1; /*1 means the router is pipelined, 0 means not*/
	params["H_tree_clock "] = 1; /*1 means calculate H_tree_clock power, 0 means not calculate H_tree_clock*/
	params["router_diagonal"] = 626; /*router diagonal in micro-meter */

	/* RF module parameters */
	params["read_port"] = 1;
	params["write_port"] = 1;
	params["n_regs"] = 64;
	params["reg_width"] = 32;

	params["ndwl"] = 1;
	params["ndbl"] = 1;
	params["nspd"] = 1;

	params["POWER_STATS"] = 1;

	/****************************** from technology.h ******************************/

	params["AF"] = (5.000000e-01);
	params["MAXN"] = (8);
	params["MAXSUBARRAYS"] = (8);
	params["MAXSPD"] = (8);
	params["VTHOUTDRNOR"] = (4.310000e-01);
	params["VTHCOMPINV "] = (4.370000e-01);
	params["BITOUT"] = (64);
	params["ruu_issue_width"] = (4);
	params["amp_Idsat"] = (5.000000e-04);
	params["VSINV"] = (4.560000e-01);
	params["GEN_POWER_FACTOR"] = (1.310000e+00);
	params["VTHNAND60x90"] = (5.610000e-01);
	params["FUDGEFACTOR"] = (1.000000e+00);
	params["VTHOUTDRIVE"] = (4.250000e-01);
	params["VTHMUXDRV1"] = (4.370000e-01);
	params["VTHMUXDRV2"] = (4.860000e-01);
	params["NORMALIZE_SCALE"] = (6.488730e-10);
	params["VTHMUXDRV3"] = (4.370000e-01);
	params["ADDRESS_BITS"] = (64);
	params["RUU_size"] = (16);
	params["VTHNOR12x4x1"] = (5.030000e-01);
	params["VTHNOR12x4x2"] = (4.520000e-01);
	params["VTHOUTDRINV"] = (4.370000e-01);
	params["VTHNOR12x4x3"] = (4.170000e-01);
	params["VTHEVALINV"] = (2.670000e-01);
	params["VTHNOR12x4x4"] = (3.900000e-01);
	params["res_ialu"] = (4);
	params["VTHOUTDRNAND"] = (4.410000e-01);
	params["VTHINV100x60"] = (4.380000e-01);

	if (technology >= 110) {
		params["Cgatepass"] = (1.450000e-15);
		params["Cpdiffarea"] = (6.060000e-16);
		params["Cpdiffside"] = (2.400000e-16);
		params["Cndiffside"] = (2.400000e-16);
		params["Cndiffarea"] = (6.600000e-16);
		params["Cnoverlap"] = (1.320000e-16);
		params["Cpoverlap"] = (1.210000e-16);
		params["Cgate"] = (9.040000e-15);
		params["Cpdiffovlp"] = (1.380000e-16);
		params["Cndiffovlp"] = (1.380000e-16);
		params["Cnoxideovlp"] = (2.230000e-16);
		params["Cpoxideovlp"] = (3.380000e-16);
	} else if (technology <= 90) {
		if (trans_type == LVT) {
			params["Cgatepass"] = (1.5225000e-14);
			params["Cpdiffarea"] = (6.05520000e-15);
			params["Cpdiffside"] = (2.38380000e-15);
			params["Cndiffside"] = (2.8500000e-16);
			params["Cndiffarea"] = (5.7420000e-15);
			params["Cnoverlap"] = (1.320000e-16);
			params["Cpoverlap"] = (1.210000e-16);
			params["Cgate"] = (7.8648000e-14);
			params["Cpdiffovlp"] = (1.420000e-16);
			params["Cndiffovlp"] = (1.420000e-16);
			params["Cnoxideovlp"] = (2.580000e-16);
			params["Cpoxideovlp"] = (3.460000e-16);
		} else if (trans_type == NVT) {
			params["Cgatepass"] = (8.32500e-15);
			params["Cpdiffarea"] = (3.330600e-15);
			params["Cpdiffside"] = (1.29940000e-15);
			params["Cndiffside"] = (2.5500000e-16);
			params["Cndiffarea"] = (2.9535000e-15);
			params["Cnoverlap"] = (1.270000e-16);
			params["Cpoverlap"] = (1.210000e-16);
			params["Cgate"] = (3.9664000e-14);
			params["Cpdiffovlp"] = (1.31000e-16);
			params["Cndiffovlp"] = (1.310000e-16);
			params["Cnoxideovlp"] = (2.410000e-16);
			params["Cpoxideovlp"] = (3.170000e-16);
		} else if (trans_type == HVT) {
			params["Cgatepass"] = (1.45000e-15);
			params["Cpdiffarea"] = (6.06000e-16);
			params["Cpdiffside"] = (2.150000e-16);
			params["Cndiffside"] = (2.25000e-16);
			params["Cndiffarea"] = (1.650000e-16);
			params["Cnoverlap"] = (1.220000e-16);
			params["Cpoverlap"] = (1.210000e-16);
			params["Cgate"] = (6.8000e-16);
			params["Cpdiffovlp"] = (1.20000e-16);
			params["Cndiffovlp"] = (1.20000e-16);
			params["Cnoxideovlp"] = (2.230000e-16);
			params["Cpoxideovlp"] = (2.880000e-16);
		}

	}

	/********************************** technology v2 *******************************/

	/* This file contains parameters for 65nm, 45nm and 32nm */
	if (technology <= 90) {
		params["Vbitpre"] = PARM("Vdd");
		params["Vbitsense"] = (0.08);

		params["SensePowerfactor3"] = (PARM("Freq")) * PARM("Vbitsense")
				* PARM("Vbitsense");
		params["SensePowerfactor2"] = (PARM("Freq")) * PARM("Vbitpre") - PARM(
				"Vbitsense") * (PARM("Vbitpre") - PARM("Vbitsense"));
		params["SensePowerfactor"] = (PARM("Freq")) * PARM("Vdd")
				* (PARM("Vdd") / 2);
		params["SenseEnergyFactor"] = (PARM("Vdd") * PARM("Vdd") / 2);

		/* scaling factors from 65nm to 45nm and 32nm*/
		if (technology == 45) {
			if (trans_type == LVT) {

				params["SCALE_T"] = (0.9123404);
				params["SCALE_M"] = (0.6442105);
				params["SCALE_S"] = (2.3352694);
				params["SCALE_W"] = (0.51);
				params["SCALE_H"] = (0.88);
				params["SCALE_BW"] = (0.73);
				params["SCALE_Crs"] = (0.7);
			} else if (trans_type == NVT) {
				params["SCALE_T"] = (0.8233582);
				params["SCALE_M"] = (0.6442105);
				params["SCALE_S"] = (2.1860558);
				params["SCALE_W"] = (0.51);
				params["SCALE_H"] = (0.88);
				params["SCALE_BW"] = (0.73);
				params["SCALE_Crs"] = (0.7);
			} else if (trans_type == HVT) {
				params["SCALE_T"] = (0.73437604);
				params["SCALE_M"] = (0.6442105);
				params["SCALE_S"] = (2.036842);
				params["SCALE_W"] = (0.51);
				params["SCALE_H"] = (0.88);
				params["SCALE_BW"] = (0.73);
				params["SCALE_Crs"] = (0.7);
			}
		} else if (technology == 32) {
			if (trans_type == LVT) {

				params["SCALE_T"] = (0.7542128);
				params["SCALE_M"] = (0.4863158);
				params["SCALE_S"] = (2.9692334);
				params["SCALE_W"] = (0.26);
				params["SCALE_H"] = (0.77);
				params["SCALE_BW"] = (0.53);
				params["SCALE_Crs"] = (0.49);
			} else if (trans_type == NVT) {
				params["SCALE_T"] = (0.6352095);
				params["SCALE_M"] = (0.4863158);
				params["SCALE_S"] = (3.1319851);
				params["SCALE_W"] = (0.26);
				params["SCALE_H"] = (0.77);
				params["SCALE_BW"] = (0.53);
				params["SCALE_Crs"] = (0.49);
			} else if (trans_type == HVT) {
				params["SCALE_T"] = (0.5162063);
				params["SCALE_M"] = (0.4863158);
				params["SCALE_S"] = (3.294737);
				params["SCALE_W"] = (0.26);
				params["SCALE_H"] = (0.77);
				params["SCALE_BW"] = (0.53);
				params["SCALE_Crs"] = (0.49);
			}
		} else { /* for 65nm and 90nm */
			params["SCALE_T"] = (1);
			params["SCALE_M"] = (1);
			params["SCALE_S"] = (1);
			params["SCALE_W"] = (1);
			params["SCALE_H"] = (1);
			params["SCALE_BW"] = (1);
			params["SCALE_Crs"] = (1);
		}
	}

	/*=============Below are the parameters for 65nm ========================*/

	if (PARM("TECH_POINT") <= 65) {

		params["LSCALE"] = 0.087;
		params["MSCALE"] = (PARM("LSCALE") * .624 / .2250);

		/* bit width of RAM cell in um */
		params["BitWidth"] = (1.4);

		/* bit height of RAM cell in um */
		params["BitHeight"] = (1.4);

		params["Cout"] = (4.35e-14);

		/* Sizing of cells and spacings */
		params["BitlineSpacing"] = (0.8 * PARM("SCALE_BW"));
		params["WordlineSpacing"] = (0.8 * PARM("SCALE_BW"));

		params["RegCellHeight"] = (2.1 * PARM("SCALE_H"));
		params["RegCellWidth"] = (1.4 * PARM("SCALE_W"));

		params["Cwordmetal"] = (1.63e-15 * PARM("SCALE_M"));
		params["Cbitmetal"] = (3.27e-15 * PARM("SCALE_M"));

		params["Cmetal"] = (PARM("Cbitmetal") / 16);
		params["CM2metal"] = (PARM("Cbitmetal") / 16);
		params["CM3metal"] = (PARM("Cbitmetal") / 16);

		// minimum spacing
		params["CCmetal"] = (0.146206e-15);
		params["CCM2metal"] = (0.146206e-15);
		params["CCM3metal"] = (0.146206e-15);
		// 2x minimum spacing
		params["CC2metal"] = (0.09844e-15);
		params["CC2M2metal"] = (0.09844e-15);
		params["CC2M3metal"] = (0.09844e-15);
		// 3x minimum spacing
		params["CC3metal"] = (0.08689e-15);
		params["CC3M2metal"] = (0.08689e-15);
		params["CC3M3metal"] = (0.08689e-15);

		/* corresponds to clock network*/
		params["Clockwire"] = (323.4e-12 * PARM("SCALE_M"));
		params["Reswire"] = (61.11e3 * (1 / PARM("SCALE_M")));
		params["invCap"] = (3.12e-14);
		params["Resout"] = (361.00);

		/* um */
		params["Leff"] = (0.0696);
		/* length unit in um */
		params["Lamda"] = (PARM("Leff") * 0.5);

		/* fF/um */
		params["Cpolywire"] = (1.832e-15);
		/* ohms*um of channel width */
		params["Rnchannelstatic"] = (2244.6);

		/* ohms*um of channel width */
		params["Rpchannelstatic"] = (5324.4);

		if (PARM("TRANSISTOR_TYPE") == LVT) { /* derived from Cacti 5.3 */
			params["Rnchannelon"] = (1370);
			params["Rpchannelon"] = (3301);

			params["Vt"] = 0.195;

			params["Wdecdrivep"] = (8.27);
			params["Wdecdriven"] = (6.70);
			params["Wdec3to8n"] = (2.33);
			params["Wdec3to8p"] = (2.33);
			params["WdecNORn"] = (1.50);
			params["WdecNORp"] = (3.82);
			params["Wdecinvn"] = (8.46);
			params["Wdecinvp"] = (10.93);
			params["Wdff"] = (8.6);

			params["Wworddrivemax"] = (9.27);
			params["Wmemcella"] = (0.2225);
			params["Wmemcellr"] = (0.3708);
			params["Wmemcellw"] = (0.1947);
			params["Wmemcellbscale"] = (1.87);
			params["Wbitpreequ"] = (0.927);

			params["Wbitmuxn"] = (0.927);
			params["WsenseQ1to4"] = (0.371);
			params["Wcompinvp1"] = (0.927);
			params["Wcompinvn1"] = (0.5562);
			params["Wcompinvp2"] = (1.854);
			params["Wcompinvn2"] = (1.1124);
			params["Wcompinvp3"] = (3.708);
			params["Wcompinvn3"] = (2.2248);
			params["Wevalinvp"] = (1.854);
			params["Wevalinvn"] = (7.416);

			params["Wcompn"] = (1.854);
			params["Wcompp"] = (2.781);
			params["Wcomppreequ"] = (3.712);
			params["Wmuxdrv12n"] = (2.785);
			params["Wmuxdrv12p"] = (4.635);
			params["WmuxdrvNANDn"] = (1.860);
			params["WmuxdrvNANDp"] = (7.416);
			params["WmuxdrvNORn"] = (5.562);
			params["WmuxdrvNORp"] = (7.416);
			params["Wmuxdrv3n"] = (18.54);
			params["Wmuxdrv3p"] = (44.496);
			params["Woutdrvseln"] = (1.112);
			params["Woutdrvselp"] = (1.854);
			params["Woutdrvnandn"] = (2.225);
			params["Woutdrvnandp"] = (0.927);
			params["Woutdrvnorn"] = (0.5562);
			params["Woutdrvnorp"] = (3.708);
			params["Woutdrivern"] = (4.450);
			params["Woutdriverp"] = (7.416);
			params["Wbusdrvn"] = (4.450);
			params["Wbusdrvp"] = (7.416);

			params["Wcompcellpd2"] = (0.222);
			params["Wcompdrivern"] = (37.08);
			params["Wcompdriverp"] = (74.20);
			params["Wcomparen2"] = (3.708);
			params["Wcomparen1"] = (1.854);
			params["Wmatchpchg"] = (0.927);
			params["Wmatchinvn"] = (0.930);
			params["Wmatchinvp"] = (1.854);
			params["Wmatchnandn"] = (1.854);
			params["Wmatchnandp"] = (0.927);
			params["Wmatchnorn"] = (1.860);
			params["Wmatchnorp"] = (0.930);

			params["WSelORn"] = (0.930);
			params["WSelORprequ"] = (3.708);
			params["WSelPn"] = (0.927);
			params["WSelPp"] = (1.391);
			params["WSelEnn"] = (0.434);
			params["WSelEnp"] = (0.930);

			params["Wsenseextdrv1p"] = (3.708);
			params["Wsenseextdrv1n"] = (2.225);
			params["Wsenseextdrv2p"] = (18.54);
			params["Wsenseextdrv2n"] = (11.124);
		} else if (PARM("TRANSISTOR_TYPE") == NVT) {
			params["Rnchannelon"] = (2540);
			params["Rpchannelon"] = (5791);

			params["Vt"] = 0.285;

			params["Wdecdrivep"] = (6.7);
			params["Wdecdriven"] = (4.7);
			params["Wdec3to8n"] = (1.33);
			params["Wdec3to8p"] = (1.33);
			params["WdecNORn"] = (1.20);
			params["WdecNORp"] = (2.62);
			params["Wdecinvn"] = (1.46);
			params["Wdecinvp"] = (3.93);
			params["Wdff"] = (4.6);

			params["Wworddrivemax"] = (9.225);
			params["Wmemcella"] = (0.221);
			params["Wmemcellr"] = (0.369);
			params["Wmemcellw"] = (0.194);
			params["Wmemcellbscale"] = 1.87;
			params["Wbitpreequ"] = (0.923);

			params["Wbitmuxn"] = (0.923);
			params["WsenseQ1to4"] = (0.369);
			params["Wcompinvp1"] = (0.924);
			params["Wcompinvn1"] = (0.554);
			params["Wcompinvp2"] = (1.845);
			params["Wcompinvn2"] = (1.107);
			params["Wcompinvp3"] = (3.69);
			params["Wcompinvn3"] = (2.214);
			params["Wevalinvp"] = (1.842);
			params["Wevalinvn"] = (7.368);

			params["Wcompn"] = (1.845);
			params["Wcompp"] = (2.768);
			params["Wcomppreequ"] = (3.692);
			params["Wmuxdrv12n"] = (2.773);
			params["Wmuxdrv12p"] = (4.618);
			params["WmuxdrvNANDn"] = (1.848);
			params["WmuxdrvNANDp"] = (7.38);
			params["WmuxdrvNORn"] = (5.535);
			params["WmuxdrvNORp"] = (7.380);
			params["Wmuxdrv3n"] = (18.45);
			params["Wmuxdrv3p"] = (44.28);
			params["Woutdrvseln"] = (1.105);
			params["Woutdrvselp"] = (1.842);
			params["Woutdrvnandn"] = (2.214);
			params["Woutdrvnandp"] = (0.923);
			params["Woutdrvnorn"] = (0.554);
			params["Woutdrvnorp"] = (3.69);
			params["Woutdrivern"] = (4.428);
			params["Woutdriverp"] = (7.380);
			params["Wbusdrvn"] = (4.421);
			params["Wbusdrvp"] = (7.368);

			params["Wcompcellpd2"] = (0.221);
			params["Wcompdrivern"] = (36.84);
			params["Wcompdriverp"] = (73.77);
			params["Wcomparen2"] = (3.684);
			params["Wcomparen1"] = (1.842);
			params["Wmatchpchg"] = (0.921);
			params["Wmatchinvn"] = (0.923);
			params["Wmatchinvp"] = (1.852);
			params["Wmatchnandn"] = (1.852);
			params["Wmatchnandp"] = (0.921);
			params["Wmatchnorn"] = (1.845);
			params["Wmatchnorp"] = (0.923);

			params["WSelORn"] = (0.923);
			params["WSelORprequ"] = (3.684);
			params["WSelPn"] = (0.921);
			params["WSelPp"] = (1.382);
			params["WSelEnn"] = (0.446);
			params["WSelEnp"] = (0.923);

			params["Wsenseextdrv1p"] = (3.684);
			params["Wsenseextdrv1n"] = (2.211);
			params["Wsenseextdrv2p"] = (18.42);
			params["Wsenseextdrv2n"] = (11.052);
		} else if (PARM("TRANSISTOR_TYPE") == HVT) {
			params["Rnchannelon"] = (4530);
			params["Rpchannelon"] = (10101);

			params["Vt"] = 0.524;

			params["Wdecdrivep"] = (3.11);
			params["Wdecdriven"] = (1.90);
			params["Wdec3to8n"] = (1.33);
			params["Wdec3to8p"] = (1.33);
			params["WdecNORn"] = (0.90);
			params["WdecNORp"] = (1.82);
			params["Wdecinvn"] = (0.46);
			params["Wdecinvp"] = (0.93);
			params["Wdff"] = (3.8);

			params["Wworddrivemax"] = (9.18);
			params["Wmemcella"] = (0.220);
			params["Wmemcellr"] = (0.367);
			params["Wmemcellw"] = (0.193);
			params["Wmemcellbscale"] = 1.87;
			params["Wbitpreequ"] = (0.918);

			params["Wbitmuxn"] = (0.918);
			params["WsenseQ1to4"] = (0.366);
			params["Wcompinvp1"] = (0.920);
			params["Wcompinvn1"] = (0.551);
			params["Wcompinvp2"] = (1.836);
			params["Wcompinvn2"] = (1.102);
			params["Wcompinvp3"] = (3.672);
			params["Wcompinvn3"] = (2.203);
			params["Wevalinvp"] = (1.83);
			params["Wevalinvn"] = (7.32);

			params["Wcompn"] = (1.836);
			params["Wcompp"] = (2.754);
			params["Wcomppreequ"] = (3.672);
			params["Wmuxdrv12n"] = (2.760);
			params["Wmuxdrv12p"] = (4.60);
			params["WmuxdrvNANDn"] = (1.836);
			params["WmuxdrvNANDp"] = (7.344);
			params["WmuxdrvNORn"] = (5.508);
			params["WmuxdrvNORp"] = (7.344);
			params["Wmuxdrv3n"] = (18.36);
			params["Wmuxdrv3p"] = (44.064);
			params["Woutdrvseln"] = (1.098);
			params["Woutdrvselp"] = (1.83);
			params["Woutdrvnandn"] = (2.203);
			params["Woutdrvnandp"] = (0.918);
			params["Woutdrvnorn"] = (0.551);
			params["Woutdrvnorp"] = (3.672);
			params["Woutdrivern"] = (4.406);
			params["Woutdriverp"] = (7.344);
			params["Wbusdrvn"] = (4.392);
			params["Wbusdrvp"] = (7.32);

			params["Wcompcellpd2"] = (0.220);
			params["Wcompdrivern"] = (36.6);
			params["Wcompdriverp"] = (73.33);
			params["Wcomparen2"] = (3.66);
			params["Wcomparen1"] = (1.83);
			params["Wmatchpchg"] = (0.915);
			params["Wmatchinvn"] = (0.915);
			params["Wmatchinvp"] = (1.85);
			params["Wmatchnandn"] = (1.85);
			params["Wmatchnandp"] = (0.915);
			params["Wmatchnorn"] = (1.83);
			params["Wmatchnorp"] = (0.915);

			params["WSelORn"] = (0.915);
			params["WSelORprequ"] = (3.66);
			params["WSelPn"] = (0.915);
			params["WSelPp"] = (1.373);
			params["WSelEnn"] = (0.458);
			params["WSelEnp"] = (0.915);

			params["Wsenseextdrv1p"] = (3.66);
			params["Wsenseextdrv1n"] = (2.196);
			params["Wsenseextdrv2p"] = (18.3);
			params["Wsenseextdrv2n"] = (10.98);
		}

		params["Rbitmetal"] = (1.92644); /* derived from Cacti 5.3 */
		params["Rwordmetal"] = (1.31948); /* derived from Cacti 5.3 */

		params["CamCellHeight"] = (2.9575); /* derived from Cacti 5.3 */
		params["CamCellWidth"] = (2.535); /* derived from Cacti 5.3 */

		params["MatchlineSpacing"] = (0.522);
		params["TaglineSpacing"] = (0.522);

		params["CrsbarCellHeight"] = (2.06 * PARM("SCALE_Crs"));
		params["CrsbarCellWidth"] = (2.06 * PARM("SCALE_Crs"));

		params["krise"] = (0.348e-10);
		params["tsensedata"] = (0.5046e-10);
		params["tsensetag"] = (0.2262e-10);
		params["tfalldata"] = (0.609e-10);
		params["tfalltag"] = (0.6609e-10);

	}

	/*=======================PARAMETERS for Link===========================*/

	if (PARM("TECH_POINT") == 65) { /* PARAMETERS for Link at 65nm */
		if (PARM("WIRE_LAYER_TYPE") == LOCAL) {
			params["WireMinWidth"] = 136e-9;
			params["WireMinSpacing"] = 136e-9;
			params["WireMetalThickness"] = 231.2e-9;
			params["WireBarrierThickness"] = 4.8e-9;
			params["WireDielectricThickness"] = 231.2e-9;
			params["WireDielectricConstant"] = 2.85;
		} else if (PARM("WIRE_LAYER_TYPE") == INTERMEDIATE) {
			params["WireMinWidth"] = 140e-9;
			params["WireMinSpacing"] = 140e-9;
			params["WireMetalThickness"] = 252e-9;
			params["WireBarrierThickness"] = 5.2e-9;
			params["WireDielectricThickness"] = 224e-9;
			params["WireDielectricConstant"] = 2.85;

		} else if (PARM("WIRE_LAYER_TYPE") == GLOBAL) {
			params["WireMinWidth"] = 400e-9;
			params["WireMinSpacing"] = 400e-9;
			params["WireMetalThickness"] = 400e-9;
			params["WireBarrierThickness"] = 5.2e-9;
			params["WireDielectricThickness"] = 790e-9;
			params["WireDielectricConstant"] = 2.9;

		} /*WIRE_LAYER_TYPE for 65nm*/

	} else if (PARM("TECH_POINT") == 45) { /* PARAMETERS for Link at 45nm */
		if (PARM("WIRE_LAYER_TYPE") == LOCAL) {
			params["WireMinWidth"] = 45e-9;
			params["WireMinSpacing"] = 45e-9;
			params["WireMetalThickness"] = 129.6e-9;
			params["WireBarrierThickness"] = 3.3e-9;
			params["WireDielectricThickness"] = 162e-9;
			params["WireDielectricConstant"] = 2.0;

		} else if (PARM("WIRE_LAYER_TYPE") == INTERMEDIATE) {
			params["WireMinWidth"] = 45e-9;
			params["WireMinSpacing"] = 45e-9;
			params["WireMetalThickness"] = 129.6e-9;
			params["WireBarrierThickness"] = 3.3e-9;
			params["WireDielectricThickness"] = 72e-9;
			params["WireDielectricConstant"] = 2.0;

		} else if (PARM("WIRE_LAYER_TYPE") == GLOBAL) {
			params["WireMinWidth"] = 67.5e-9;
			params["WireMinSpacing"] = 67.5e-9;
			params["WireMetalThickness"] = 155.25e-9;
			params["WireBarrierThickness"] = 3.3e-9;
			params["WireDielectricThickness"] = 141.75e-9;
			params["WireDielectricConstant"] = 2.0;

		} /*WIRE_LAYER_TYPE for 45nm*/

	} else if (PARM("TECH_POINT") == 32) { /* PARAMETERS for Link at 32nm */
		if (PARM("WIRE_LAYER_TYPE") == LOCAL) {
			params["WireMinWidth"] = 32e-9;
			params["WireMinSpacing"] = 32e-9;
			params["WireMetalThickness"] = 60.8e-9;
			params["WireBarrierThickness"] = 2.4e-9;
			params["WireDielectricThickness"] = 60.8e-9;
			params["WireDielectricConstant"] = 1.9;

		} else if (PARM("WIRE_LAYER_TYPE") == INTERMEDIATE) {
			params["WireMinWidth"] = 32e-9;
			params["WireMinSpacing"] = 32e-9;
			params["WireMetalThickness"] = 60.8e-9;
			params["WireBarrierThickness"] = 2.4e-9;
			params["WireDielectricThickness"] = 54.4e-9;
			params["WireDielectricConstant"] = 1.9;

		} else if (PARM("WIRE_LAYER_TYPE") == GLOBAL) {
			params["WireMinWidth"] = 48e-9;
			params["WireMinSpacing"] = 48e-9;
			params["WireMetalThickness"] = 120e-9;
			params["WireBarrierThickness"] = 2.4e-9;
			params["WireDielectricThickness"] = 110.4e-9;
			params["WireDielectricConstant"] = 1.9;

		} /* WIRE_LAYER_TYPE for 32nm*/

	} /* PARM(TECH_POINT) */

	/*===================================================================*/
	/*parameters for insertion buffer for links at 65nm*/
	if (PARM("TECH_POINT") == 65) {
		params["BufferDriveResistance"] = 6.77182e+03;
		params["BufferIntrinsicDelay"] = 3.31822e-11;
		if (PARM("TRANSISTOR_TYPE") == LVT) {
			params["BufferPMOSOffCurrent"] = 317.2e-09;
			params["BufferNMOSOffCurrent"] = 109.7e-09;
			params["BufferInputCapacitance"] = 1.3e-15;
			params["ClockCap"] = 2.6e-14;
		} else if (PARM("TRANSISTOR_TYPE") == NVT) {
			params["BufferPMOSOffCurrent"] = 113.1e-09;
			params["BufferNMOSOffCurrent"] = 67.3e-09;
			params["BufferInputCapacitance"] = 2.6e-15;
			params["ClockCap"] = 1.56e-14;
		} else if (PARM("TRANSISTOR_TYPE") == HVT) {
			params["BufferPMOSOffCurrent"] = 35.2e-09;
			params["BufferNMOSOffCurrent"] = 18.4e-09;
			params["BufferInputCapacitance"] = 7.8e-15;
			params["ClockCap "] = 0.9e-15;
		}

		/*parameters for insertion buffer for links at 45nm*/
	} else if (PARM("TECH_POINT") == 45) {
		params["BufferDriveResistance"] = 7.3228e+03;
		params["BufferIntrinsicDelay"] = 4.6e-11;
		if (PARM("TRANSISTOR_TYPE") == LVT) {
			params["BufferInputCapacitance"] = 1.25e-15;
			params["BufferPMOSOffCurrent"] = 1086.75e-09;
			params["BufferNMOSOffCurrent"] = 375.84e-09;
			params["ClockCap"] = 2.5e-14;
		} else if (PARM("TRANSISTOR_TYPE") == NVT) {
			params["BufferInputCapacitance"] = 2.5e-15;
			params["BufferPMOSOffCurrent"] = 382.3e-09;
			params["BufferNMOSOffCurrent"] = 195.5e-09;
			params["ClockCap"] = 1.5e-14;
		} else if (PARM("TRANSISTOR_TYPE") == HVT) {
			params["BufferInputCapacitance"] = 7.5e-15;
			params["BufferPMOSOffCurrent"] = 76.4e-09;
			params["BufferNMOSOffCurrent"] = 39.1e-09;
			params["ClockCap"] = 0.84e-15;
		}
		/*parameters for insertion buffer for links at 32nm*/
	} else if (PARM("TECH_POINT") == 32) {
		params["BufferDriveResistance"] = 10.4611e+03;
		params["BufferIntrinsicDelay"] = 4.0e-11;
		if (PARM("TRANSISTOR_TYPE") == LVT) {
			params["BufferPMOSOffCurrent"] = 1630.08e-09;
			params["BufferNMOSOffCurrent"] = 563.74e-09;
			params["BufferInputCapacitance"] = 1.2e-15;
			params["ClockCap"] = 2.2e-14;
		} else if (PARM("TRANSISTOR_TYPE") == NVT) {
			params["BufferPMOSOffCurrent"] = 792.4e-09;
			params["BufferNMOSOffCurrent"] = 405.1e-09;
			params["BufferInputCapacitance"] = 2.4e-15;
			params["ClockCap"] = 1.44e-14;
		} else if (PARM("TRANSISTOR_TYPE") == HVT) {
			params["BufferPMOSOffCurrent"] = 129.9e-09;
			params["BufferNMOSOffCurrent"] = 66.4e-09;
			params["BufferInputCapacitance"] = 7.2e-15;
			params["ClockCap"] = 0.53e-15;
		}
	}/*PARM(TECH_POINT)*/

	/*======================Parameters for Area===========================*/

	if (PARM("TECH_POINT") <= 65) {
		params["AreaNOR"] = (2.52 * PARM("SCALE_T"));
		params["AreaINV"] = (1.44 * PARM("SCALE_T"));
		params["AreaDFF"] = (8.28 * PARM("SCALE_T"));
		params["AreaAND"] = (2.52 * PARM("SCALE_T"));
		params["AreaMUX2"] = (6.12 * PARM("SCALE_T"));
		params["AreaMUX3"] = (9.36 * PARM("SCALE_T"));
		params["AreaMUX4"] = (12.6 * PARM("SCALE_T"));
	}

}
