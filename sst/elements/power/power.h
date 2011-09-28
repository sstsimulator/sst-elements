// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SST_POWER_H
#define _SST_POWER_H


#include "interface.h"
#include "HotSpot-interface.h"
////#include "IntSim-interface.h"


/*********Sim-Panalyzer************/
//#define PANALYZER_H
////#define LV1_PANALYZER_H
//#define LV2_PANALYZER_H

/* added for sim-panalyzer power analysis */
extern "C"{
#ifdef PANALYZER_H

#ifdef LV1_PANALYZER_H
#include "../sst/core/techModels/libsim-panalyzer/lv1_opts.h"
#include "../sst/core/techModels/libsim-panalyzer/lv1_panalyzer.h"
#include "../sst/core/techModels/libsim-panalyzer/lv1_cache_panalyzer.h"
#include "../sst/core/techModels/libsim-panalyzer/io_panalyzer.h"
#endif

#ifdef LV2_PANALYZER_H
#include "../sst/core/techModels/libsim-panalyzer/technology.h"
#include "../sst/core/techModels/libsim-panalyzer/alu_panalyzer.h"
#include "../sst/core/techModels/libsim-panalyzer/mult_panalyzer.h"
#include "../sst/core/techModels/libsim-panalyzer/fpu_panalyzer.h"
#include "../sst/core/techModels/libsim-panalyzer/uarch_panalyzer.h"
#endif //lv2_panalyzer_h

#endif //panalyzer_h
}


/***********McPAT05****************/
//#define McPAT05_H

/*added for McPAT05 power analysis */
#ifdef McPAT05_H
#include "../sst/core/techModels/libMcPAT/io.h"
#include "../sst/core/techModels/libMcPAT/logic.h"
#include "../sst/core/techModels/libMcPAT/full_decoder.h"
#include "../sst/core/techModels/libMcPAT/crossbarswitch.h"
#include "../sst/core/techModels/libMcPAT/basic_circuit.h"
#include "../sst/core/techModels/libMcPAT/processor.h"
#endif //mcpat05_h


/***********McPAT06****************/
//#define McPAT06_H

/*added for McPAT06 power analysis */
#ifdef McPAT06_H
#include "../sst/core/techModels/libMcPATbeta06/io.h"
#include "../sst/core/techModels/libMcPATbeta06/logic.h"
#include "../sst/core/techModels/libMcPATbeta06/basic_circuit.h"
#include "../sst/core/techModels/libMcPATbeta06/processor.h"
#endif //mcpat06_h

/***********McPAT07****************/
#define McPAT07_H

/*added for McPAT07 power analysis */
#ifdef McPAT07_H
#include "../sst/core/techModels/libMcPATbeta/io.h"
#include "../sst/core/techModels/libMcPATbeta/logic.h"
#include "../sst/core/techModels/libMcPATbeta/basic_circuit.h"
#include "../sst/core/techModels/libMcPATbeta/processor.h"
#include "../sst/core/techModels/libMcPATbeta/globalvar.h"
#endif //mcpat07_h

/*********ORION******************/
////#define ORION_H

/*added for ORION power analysis */
extern "C"{
#ifdef ORION_H
#include "../sst/core/techModels/libORION/SIM_parameter.h"
#include "../sst/core/techModels/libORION/SIM_router.h"
#include "../sst/core/techModels/libORION/SIM_link.h"
#endif //orion_h
}



#define MAX_NUM_SUBCOMP 20
////#define MESMTHI_H  //mesmthi is a 4x4 multicore system model and need special mapping of power 
		   //to corresponding floorplan blocks 


namespace SST {

        typedef int nm;
        typedef int ns;

	enum logic_style {STATIC, DYNAMIC};
	enum clock_style{NORM_H, BALANCED_H};
	enum io_style{IN, OUT, BI};
	enum topology_style{TWODMESH, RING, CROSSBAR}; 

	//TODO consider to add "ALL" to ptype, so users don't have to list/decouple power params one by one.
	enum ptype {CACHE_IL1, CACHE_IL2, CACHE_DL1, CACHE_DL2, CACHE_ITLB, CACHE_DTLB, CLOCK, BPRED, RF, IO, LOGIC, EXEU_ALU, EXEU_FPU, MULT, IB, ISSUE_Q, INST_DECODER, BYPASS, EXEU, PIPELINE, LSQ, RAT, ROB, BTB, CACHE_L2, MEM_CTRL, ROUTER, LOAD_Q, RENAME_U, SCHEDULER_U, CACHE_L3, CACHE_L1DIR, CACHE_L2DIR, UARCH}; //RS is renamed by ISSUE_Q because of name collision	in s-p
	enum pmodel{McPAT, SimPanalyzer, McPAT05, IntSim, ORION}; //McPAT05 is an older version
	enum tmodel {HOTSPOT};
	enum tlayer{SILICON, INTERFACE, SPREADER, HEATSINK, NUM_THERMAL_LAYERS};
	
	

typedef struct 
{
	watts il1_read, il1_write;
	watts il2_read, il2_write;
	watts dl1_read, dl1_write;
	watts dl2_read, dl2_write;
	watts itlb_read, itlb_write;
	watts dtlb_read, dtlb_write;
	watts aio, dio;
	watts clock;
	watts logic;
	watts rf, bpred;
	watts alu, fpu, mult, exeu;
	watts ib, issueQ, decoder, bypass, pipeline, lsq;
	watts rat, rob, btb, l2, mc, router, loadQ, rename, scheduler, l3, l1dir, l2dir;
	watts uarch;
}Punit_t;

typedef struct  //this takes over the core params in each subcomponent_params_t
{
	unsigned core_physical_address_width, core_temperature, core_tech_node;
	unsigned core_virtual_address_width, core_virtual_memory_page_size, core_number_hardware_threads;		
	unsigned machine_bits, archi_Regs_IRF_size, archi_Regs_FRF_size, core_phy_Regs_IRF_size, core_phy_Regs_FRF_size;
	unsigned core_register_windows_size, core_opcode_width;	
	unsigned core_instruction_window_size;
	unsigned core_issue_width, core_decode_width, core_fetch_width, core_commit_width;	
	unsigned core_instruction_length;
	unsigned core_instruction_buffer_size;
	unsigned ALU_per_core, FPU_per_core, MUL_per_core;	
	unsigned core_store_buffer_size, core_memory_ports;
	unsigned core_int_pipeline_depth, core_RAS_size, core_ROB_size, core_load_buffer_size, core_number_of_NoCs;
	unsigned core_number_instruction_fetch_ports, core_fp_issue_width, core_fp_instruction_window_size, core_peak_issue_width, core_micro_opcode_width;	
	unsigned core_long_channel;
} core_params_t;

typedef struct
{
	std::vector<farads> unit_scap;   //switching
	//farads unit_scap;  
        farads unit_icap;  //internal
        farads unit_lcap;  //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_physical_address_width;
	unsigned core_virtual_address_width, core_virtual_memory_page_size, core_number_hardware_threads;
	unsigned core_temperature, core_tech_node;  

	//for cache components
	int num_sets;
	std::vector<int> line_size;
	int num_bitlines;
	int num_wordlines;
	std::vector<int> assoc;
	unsigned num_rwports, num_rports, num_wports;
	std::vector<unsigned> num_banks;
	std::vector<double> throughput;
	std::vector<double> latency;
	std::vector<double> output_width;
	std::vector<int> cache_policy;
	std::vector<unsigned> miss_buf_size, fill_buf_size, prefetch_buf_size, wbb_buf_size;
	unsigned number_entries;
	unsigned device_type, directory_type;
	double area, num_transistors;
} cache_params_t;



struct clock_params_t
{
	farads unit_scap;  //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_temperature, core_tech_node;	

	//for clock components
	clock_style clk_style;
	double skew;
	nm chip_area;
	farads node_cap;
	int opt_clock_buffer_num;
	double area, num_transistors;
	clock_option_type_t clock_option;
};

struct bpred_params_t
{
	farads unit_icap; //internal
	farads unit_ecap;  //effective capacitance-for lv1
	farads unit_scap;  //switching
	volts vss;
	double op_freq;

	unsigned global_predictor_bits, global_predictor_entries, prediction_width, local_predictor_size, local_predictor_entries;
	unsigned chooser_predictor_bits, chooser_predictor_entries;
	unsigned nrows, ncols;
	unsigned num_rwports, num_rports, num_wports;	
	unsigned long bpred_access;
	double area, num_transistors;
};

struct rf_params_t
{
	farads unit_icap; //internal
	farads unit_ecap;  //effective capacitance-for lv1
	farads unit_scap;  //switching
	volts vss;
	unsigned machine_bits, archi_Regs_IRF_size, archi_Regs_FRF_size;
	unsigned core_issue_width, core_register_windows_size, core_number_hardware_threads, core_opcode_width, core_virtual_address_width;
	unsigned core_temperature, core_tech_node;

	double op_freq;
	unsigned nrows, ncols;
	unsigned num_rwports, num_rports, num_wports;   
	unsigned long rf_access;
	double area, num_transistors;
};

struct io_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	
	//for I/O components
	io_style i_o_style;
	unsigned opt_io_buffer_num;
	double ustrip_len;
	unsigned bus_width;   //bus width
	unsigned bus_size;    //io transaction size = buffer size = bsize in io_panalyzer
	unsigned io_access_time;  //in cycles
	unsigned io_cycle_time;  // in cycles
	double area, num_transistors;
};

struct logic_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_instruction_window_size, core_issue_width, core_number_hardware_threads;
	unsigned core_decode_width, archi_Regs_IRF_size, archi_Regs_FRF_size;
	unsigned core_temperature, core_tech_node;

	//for logic components
	logic_style lgc_style; //Dynamic/Static Logic	
	unsigned num_gates;
	unsigned num_functions;   // Does this mean and, or, xor??
	unsigned num_fan_in;
	unsigned num_fan_out;
	double area, num_transistors;
};

struct other_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	double area, num_transistors;
};


struct ib_params_t
{
	unsigned core_instruction_length, core_issue_width, core_number_hardware_threads;
	unsigned core_instruction_buffer_size, num_rwports;
	unsigned core_temperature, core_tech_node, core_virtual_address_width, core_virtual_memory_page_size;
	double area, num_transistors;
};

struct irs_params_t
{
	unsigned core_number_hardware_threads, core_instruction_length, core_instruction_window_size;
	unsigned core_issue_width;
	unsigned core_temperature, core_tech_node;
	double area, num_transistors;
};

struct decoder_params_t
{
	unsigned core_opcode_width;
	unsigned core_temperature, core_tech_node;
	double area, num_transistors;
};

struct bypass_params_t
{
	unsigned core_number_hardware_threads, ALU_per_core, machine_bits, FPU_per_core;
	unsigned core_opcode_width, core_virtual_address_width;
	unsigned core_store_buffer_size, core_memory_ports;
	unsigned core_temperature, core_tech_node;
	double area, num_transistors;
};

struct pipeline_params_t
{
	unsigned core_number_hardware_threads, core_fetch_width, core_decode_width;
	unsigned core_issue_width, core_commit_width, core_instruction_length, core_virtual_address_width;
	unsigned core_opcode_width, core_int_pipeline_depth;
	unsigned machine_bits, archi_Regs_IRF_size;
	unsigned core_temperature, core_tech_node;
	double area, num_transistors;
};

typedef struct
{
	farads unit_scap;  //switching
	volts vss;
	double op_freq;

	//for btb components
	
	int line_size, assoc;
	unsigned num_banks;
	double throughput, latency;
	double area, num_transistors;
} btb_params_t;

typedef struct
{
	double mc_clock;
	unsigned llc_line_length, databus_width, addressbus_width, req_window_size_per_channel;
	unsigned memory_channels_per_mc, IO_buffer_size_per_channel;
	unsigned memory_number_ranks, memory_peak_transfer_rate;
	double area, num_transistors;
} mc_params_t;

typedef struct
{
	double clockrate, vdd;
	unsigned has_global_link, flit_bits, input_buffer_entries_per_vc, virtual_channel_per_port, input_ports;
	unsigned output_ports, link_throughput, link_latency, horizontal_nodes, vertical_nodes;
	topology_style topology;
	double area, num_transistors;
	double link_length;
} router_params_t;


typedef struct
{
	/*McPAT*/
	double branch_read[MAX_NUM_SUBCOMP], branch_write[MAX_NUM_SUBCOMP], RAS_read[MAX_NUM_SUBCOMP], RAS_write[MAX_NUM_SUBCOMP];
	double il1_read[MAX_NUM_SUBCOMP], il1_readmiss[MAX_NUM_SUBCOMP], IB_read[MAX_NUM_SUBCOMP], IB_write[MAX_NUM_SUBCOMP], BTB_read[MAX_NUM_SUBCOMP], BTB_write[MAX_NUM_SUBCOMP], ID_inst_read[MAX_NUM_SUBCOMP], ID_operand_read[MAX_NUM_SUBCOMP], ID_misc_read[MAX_NUM_SUBCOMP];
	double int_win_read[MAX_NUM_SUBCOMP], int_win_write[MAX_NUM_SUBCOMP], int_win_search[MAX_NUM_SUBCOMP], fp_win_read[MAX_NUM_SUBCOMP], fp_win_write[MAX_NUM_SUBCOMP], fp_win_search[MAX_NUM_SUBCOMP], ROB_read[MAX_NUM_SUBCOMP], ROB_write[MAX_NUM_SUBCOMP];
	double iFRAT_read[MAX_NUM_SUBCOMP], iFRAT_write[MAX_NUM_SUBCOMP], iFRAT_search[MAX_NUM_SUBCOMP], fFRAT_read[MAX_NUM_SUBCOMP], fFRAT_write[MAX_NUM_SUBCOMP], fFRAT_search[MAX_NUM_SUBCOMP], iRRAT_read[MAX_NUM_SUBCOMP], fRRAT_read[MAX_NUM_SUBCOMP], iRRAT_write[MAX_NUM_SUBCOMP], fRRAT_write[MAX_NUM_SUBCOMP];
	double ifreeL_read[MAX_NUM_SUBCOMP], ifreeL_write[MAX_NUM_SUBCOMP], ffreeL_read[MAX_NUM_SUBCOMP], ffreeL_write[MAX_NUM_SUBCOMP], idcl_read[MAX_NUM_SUBCOMP], fdcl_read[MAX_NUM_SUBCOMP];
	double dl1_read[MAX_NUM_SUBCOMP], dl1_readmiss[MAX_NUM_SUBCOMP], dl1_write[MAX_NUM_SUBCOMP], dl1_writemiss[MAX_NUM_SUBCOMP], LSQ_read[MAX_NUM_SUBCOMP], LSQ_write[MAX_NUM_SUBCOMP], loadQ_read[MAX_NUM_SUBCOMP], loadQ_write[MAX_NUM_SUBCOMP];
	double itlb_read[MAX_NUM_SUBCOMP], itlb_readmiss[MAX_NUM_SUBCOMP], dtlb_read[MAX_NUM_SUBCOMP], dtlb_readmiss[MAX_NUM_SUBCOMP];
	double int_regfile_reads[MAX_NUM_SUBCOMP], int_regfile_writes[MAX_NUM_SUBCOMP], float_regfile_reads[MAX_NUM_SUBCOMP], float_regfile_writes[MAX_NUM_SUBCOMP], RFWIN_read[MAX_NUM_SUBCOMP], RFWIN_write[MAX_NUM_SUBCOMP];
	double bypass_access;
	double router_access;
	double L2_read[MAX_NUM_SUBCOMP], L2_readmiss[MAX_NUM_SUBCOMP], L2_write[MAX_NUM_SUBCOMP], L2_writemiss[MAX_NUM_SUBCOMP], L3_read[MAX_NUM_SUBCOMP], L3_readmiss[MAX_NUM_SUBCOMP], L3_write[MAX_NUM_SUBCOMP], L3_writemiss[MAX_NUM_SUBCOMP];
	double homeL2_read[MAX_NUM_SUBCOMP], homeL2_readmiss[MAX_NUM_SUBCOMP], homeL2_write[MAX_NUM_SUBCOMP], homeL2_writemiss[MAX_NUM_SUBCOMP], homeL3_read[MAX_NUM_SUBCOMP], homeL3_readmiss[MAX_NUM_SUBCOMP], homeL3_write[MAX_NUM_SUBCOMP], homeL3_writemiss[MAX_NUM_SUBCOMP];
	double L1Dir_read[MAX_NUM_SUBCOMP], L1Dir_readmiss[MAX_NUM_SUBCOMP], L1Dir_write[MAX_NUM_SUBCOMP], L1Dir_writemiss[MAX_NUM_SUBCOMP], L2Dir_read[MAX_NUM_SUBCOMP], L2Dir_readmiss[MAX_NUM_SUBCOMP], L2Dir_write[MAX_NUM_SUBCOMP], L2Dir_writemiss[MAX_NUM_SUBCOMP];
	double memctrl_read, memctrl_write;
	/*S-P*/
	double il1_ReadorWrite,  il1_datablock,  il1_access;
	unsigned il1_accessaddress,il1_latency;
	double il2_ReadorWrite, il2_datablock, il2_access;
	unsigned il2_accessaddress, il2_latency;
	double dl1_ReadorWrite, dl1_datablock, dl1_access;
	unsigned dl1_accessaddress, dl1_latency;
	double dl2_ReadorWrite, dl2_datablock, dl2_access;
	unsigned dl2_accessaddress, dl2_latency;
	double itlb_ReadorWrite, itlb_datablock, itlb_access;
	unsigned itlb_accessaddress, itlb_latency;
	double dtlb_ReadorWrite, dtlb_datablock, dtlb_access;
	unsigned dtlb_accessaddress, dtlb_latency;
	double bpred_access, rf_access, alu_access[MAX_NUM_SUBCOMP], fpu_access[MAX_NUM_SUBCOMP], mult_access[MAX_NUM_SUBCOMP], exeu_access, logic_access, clock_access;
	double io_ReadorWrite, io_datablock, io_access;
	unsigned io_accessaddress, io_latency;
	/*IntSim*/
	double ib_access, issueQ_access, decoder_access, pipeline_access, lsq_access;
	double rat_access, rob_access, btb_access, l2_access, mc_access;
	double loadQ_access, rename_access, scheduler_access, l3_access, l1dir_access, l2dir_access;
} usagecounts_t;

struct powerModel_t
{
	pmodel il1, il2, dl1, dl2, itlb, dtlb;
	pmodel clock, bpred, rf, io, logic, alu, fpu, mult;
	pmodel ib, issueQ, decoder, bypass, exeu, pipeline;
	pmodel lsq, rat, rob, btb, L2, mc, router, loadQ;
	pmodel rename, scheduler, L3, L1dir, L2dir;

	powerModel_t() : il1(McPAT), il2(McPAT), dl1(McPAT), dl2(McPAT), itlb(McPAT), dtlb(McPAT),
	clock(McPAT), bpred(McPAT), rf(McPAT), io(McPAT), logic(McPAT), alu(McPAT), fpu(McPAT), mult(McPAT),
	ib(McPAT), issueQ(McPAT), decoder(McPAT), bypass(McPAT), exeu(McPAT), pipeline(McPAT),
	lsq(McPAT), rat(McPAT), rob(McPAT), btb(McPAT), L2(McPAT), mc(McPAT), router(McPAT), loadQ(McPAT),
	rename(McPAT), scheduler(McPAT), L3(McPAT), L1dir(McPAT), L2dir(McPAT) {}
};

struct device_params_t
{
	unsigned number_core, number_il1, number_dl1, number_itlb, number_dtlb, number_L1dir, number_L2dir, number_L2, number_L3;
	unsigned machineType; //1: inorder, 0:ooo
	float clockRate; //frequency McPAT
};

struct floorplan_id_t
{
	std::vector<int> il1, il2, dl1, dl2, itlb, dtlb;
	std::vector<int> clock, bpred, rf, io, logic, alu, fpu, mult;
	std::vector<int> ib, issueQ, decoder, bypass, exeu, pipeline;
	std::vector<int> lsq, rat, rob, btb, L2, mc, router, loadQ;
	std::vector<int> rename, scheduler, L3, L1dir, L2dir;
};

class Reliability;

class Power{
   public:
 	std::vector<Pdissipation_t> p_usage_cache_il1;
	Pdissipation_t p_usage_cache_il2;
	std::vector<Pdissipation_t> p_usage_cache_dl1;
	Pdissipation_t p_usage_cache_dl2;
	Pdissipation_t p_usage_cache_itlb;
	Pdissipation_t p_usage_cache_dtlb;
	Pdissipation_t p_usage_clock;
	Pdissipation_t p_usage_io;
	Pdissipation_t p_usage_logic;
	std::vector<Pdissipation_t> p_usage_alu;
	Pdissipation_t p_usage_fpu;
	Pdissipation_t p_usage_mult;	
	std::vector<Pdissipation_t> p_usage_rf;
	std::vector<Pdissipation_t> p_usage_bpred;
	std::vector<Pdissipation_t> p_usage_ib;
	Pdissipation_t p_usage_rs;
	std::vector<Pdissipation_t> p_usage_decoder;
	Pdissipation_t p_usage_bypass;
	Pdissipation_t p_usage_exeu;
	Pdissipation_t p_usage_pipeline;
	std::vector<Pdissipation_t> p_usage_lsq;
	Pdissipation_t p_usage_rat;
	Pdissipation_t p_usage_rob;
	std::vector<Pdissipation_t> p_usage_btb;
	std::vector<Pdissipation_t> p_usage_cache_l2;
	Pdissipation_t p_usage_mc;
	Pdissipation_t p_usage_router;
	std::vector<Pdissipation_t> p_usage_loadQ;
	std::vector<Pdissipation_t> p_usage_renameU;
	std::vector<Pdissipation_t> p_usage_schedulerU;
	std::vector<Pdissipation_t> p_usage_cache_l3;
	std::vector<Pdissipation_t> p_usage_cache_l1dir;
	std::vector<Pdissipation_t> p_usage_cache_l2dir;
	Pdissipation_t p_usage_uarch; //"ALL"

	device_params_t device_tech;
	cache_params_t cache_il1_tech;
	cache_params_t cache_il2_tech;
	cache_params_t cache_dl1_tech;
	cache_params_t cache_dl2_tech;
	cache_params_t cache_itlb_tech;
	cache_params_t cache_dtlb_tech;
	cache_params_t cache_l2_tech;
	cache_params_t cache_l3_tech;
	cache_params_t cache_l1dir_tech;
	cache_params_t cache_l2dir_tech;
	clock_params_t clock_tech;
	bpred_params_t bpred_tech;
	rf_params_t rf_tech;
	io_params_t io_tech;
	logic_params_t logic_tech;
	other_params_t alu_tech;
	other_params_t fpu_tech;
	other_params_t mult_tech;
	other_params_t uarch_tech;
	ib_params_t ib_tech;
	irs_params_t irs_tech;
	bypass_params_t bypass_tech;
	decoder_params_t decoder_tech;
	pipeline_params_t pipeline_tech;
	core_params_t core_tech;	
	btb_params_t btb_tech;
	mc_params_t mc_tech;
	router_params_t router_tech;
	floorplan_id_t floorplan_id;

	
	ComponentId_t p_compID;
	int p_powerLevel; //level 1: v, f, sC, iC, lC; level 2: v, f, sC, and other params
	bool p_powerMonitor; // if a component want to have power modeled
	bool p_tempMonitor; // if a component want to have temperature modeled
	powerModel_t p_powerModel;
	Punit_t p_unitPower; // stores unit power per sub-component access
	I p_meanPeak, p_meanPeakAll; // for manual error bar on mean peak power
	double p_areaMcPAT;
	char *p_McPATxmlpath;
	unsigned p_maxNumSubComp;  //max number of sub-comp of the same type
	bool p_ifReadEntireXML, p_ifGetMcPATUnitP;
	static std::multimap<ptype,int> subcompList; //stores subcomp types and the floorplan they reside on
	
	// floorplan and thermal tiles parameters
	static parameters_chip_t chip;
	static chip_t p_chip; 
	static int p_NumCompNeedPower;  //number of subcomp of the same chip per rank that needs power 
	static int p_SumNumCompNeedPower; //number of subcomp per chip that needs power 
	static int p_TempSumNumCompNeedPower;
	static bool p_hasUpdatedTemp;
	static double p_TotalFailureRate;
	static unsigned int p_NumSamples;  // average failure rate = total failure rate / number of samples
	static std::vector<double> p_minTemperature;
	static std::vector<double> p_maxTemperature;
	static std::vector<std::vector<double> > p_thermalFreq; //i: block id; j: time instance

	//double value, maxvalue;

	// sim-panalyzer parameters
	#ifdef LV2_PANALYZER_H
	#ifdef CACHE_PANALYZER_H
	fu_cache_pspec_t *il1_pspec;
	fu_cache_pspec_t *il2_pspec;
	fu_cache_pspec_t *dl1_pspec;
	fu_cache_pspec_t *dl2_pspec;
	fu_cache_pspec_t *itlb_pspec;
	fu_cache_pspec_t *dtlb_pspec;
	#endif /* CACHE_PANALYZER_H */
	#ifdef CLOCK_PANALYZER_H
	fu_clock_pspec_t *clock_pspec;
	#endif /* CLOCK_PANALYZER_H */
	#ifdef MEMORY_PANALYZER_H
	fu_sbank_pspec_t *rf_pspec;
	fu_sbank_pspec_t *bpred_pspec;
	#endif /* MEMORY_PANALYZER_H */
	#ifdef LOGIC_PANALYZER_H 
	fu_logic_pspec_t *logic_pspec;
	#endif  /*LOGIC_PANALYZER_H*/	
	fu_alu_pspec_t *alu_pspec;
	fu_mult_pspec_t *mult_pspec;
	fu_fpu_pspec_t *fpu_pspec;
	#endif /*lv2_panalyzer*/
	
	#ifdef IO_PANALYZER_H
	/* address io panalyzer */
	fu_io_pspec_t *aio_pspec;
	/* data io panalyzer */
	fu_io_pspec_t *dio_pspec;
	#endif /* IO_PANALYZER_H */

	// McPAT05 parameters
	int perThreadState;
	double C_EXEU; //exeu capacitance

	#ifdef McPAT05_H
	#ifdef  XML_PARSE_H_
	ParseXML *p_Mp1;
	#endif /*  XML_PARSE_H_ */
	#ifdef  PROCESSOR_H_
	Processor p_Mproc;
	InputParameter interface_ip;
	#endif /* PROCESSOR_H_*/
	#ifdef  CORE_H_
	tlb_core itlb, dtlb;
	cache_processor icache, dcache;
	BTB_core BTB;
	RF_core	 IRF, FRF, RFWIN, phyIRF, phyFRF;
	IB_core  IB;
	RS_core	 iRS, iISQ,fRS, fISQ;
	LSQ_core LSQ, loadQ;
	resultbus int_bypass,intTagBypass, fp_bypass, fpTagBypass;
	selection_logic instruction_selection;
	dep_resource_conflict_check idcl,fdcl;
	full_decoder inst_decoder;
	core_pipeline corepipe;
	UndifferentiatedCore undifferentiatedCore;
	MCclock_network clockNetwork;
	ROB_core ROB;	
	predictor_core predictor;
	RAT_core iRRAT,iFRAT,iFRATCG, fRRAT,fFRAT,fFRATCG;
	#endif /* CORE_H_*/
	#ifdef  SHAREDCACHE_H_
	cache_processor llCache,directory;
        pipeline pipeLogicCache, pipeLogicDirectory;
        MCclock_network L2clockNetwork;
	#endif /*  SHAREDCACHE_H_*/
	#ifdef MEMORYCTRL_H_
	selection_logic MC_arb;
        cache_processor frontendBuffer,readBuffer, writeBuffer;
	pipeline MCpipeLogic;
        MCclock_network MCclockNetwork;
        MCBackend transecEngine;
        MCPHY	  PHY;
	#endif /*MEMORYCTRL_H_*/
	#ifdef ROUTER_H_
	cache_processor inputBuffer, routingTable;
        crossbarswitch xbar;
        Arbiter vcAllocatorStage1,vcAllocatorStage2, switchAllocatorStage1, switchAllocatorStage2;
        wire globalInterconnect;
        pipeline RTpipeLogic;
        MCclock_network RTclockNetwork;
	#endif /*ROUTER_H_*/
	#endif /*McPAT05_H*/


	//McPAT06 parameters
	#ifdef McPAT06_H
	#ifdef  XML_PARSE_H_
	ParseXML *p_Mp1;
	#endif /*  XML_PARSE_H_ */
	#ifdef  PROCESSOR_H_
	Processor p_Mproc;
	Core *p_Mcore;
	SharedCache* l2array;
        SharedCache* l3array;
        SharedCache* l1dirarray;
        SharedCache* l2dirarray;
        NoC* nocs;
        MemoryController* mc;
	#endif /* PROCESSOR_H_*/
	#ifdef  CORE_H_
	InstFetchU * ifu;
	LoadStoreU * lsu;
	MemManU    * mmu;
	EXECU      * exu;
	RENAMINGU  * rnu;
        Pipeline   * corepipe;
        UndiffCore * undiffCore;
        CoreDynParam  coredynp;
	
	ArrayST * globalBPT;
	ArrayST * L1_localBPT;
	ArrayST * L2_localBPT;
	ArrayST * chooser;
	ArrayST * RAS;
	
	InstCache icache;
	ArrayST * IB;
	ArrayST * BTB;
	BranchPredictor * BPT;

	ArrayST         * int_inst_window;
	ArrayST         * fp_inst_window;
	ArrayST         * ROB;
        selection_logic * instruction_selection;

	ArrayST * iFRAT;
	ArrayST * fFRAT;
	ArrayST * iRRAT;
	ArrayST * fRRAT;
	ArrayST * ifreeL;
	ArrayST * ffreeL;
	dep_resource_conflict_check * idcl;
	dep_resource_conflict_check * fdcl;

	DataCache dcache;
	ArrayST * LSQ;//it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
	ArrayST * LoadQ;

	ArrayST * itlb;
	ArrayST * dtlb;

	ArrayST * IRF;
	ArrayST * FRF;
	ArrayST * RFWIN;

	RegFU          * rfu;
	SchedulerU     * scheu;
        FunctionalUnit * fp_u;
        FunctionalUnit * exeu;
	McPATComponent  bypass;
	interconnect * int_bypass;
	interconnect * intTagBypass;
	interconnect * fp_bypass;
	interconnect * fpTagBypass;
	#endif /* CORE_H_*/
	#endif /*McPAT06_H*/

	//McPAT07 parameters
	#ifdef McPAT07_H
	#ifdef  XML_PARSE_H_
	ParseXML *p_Mp1;
	#endif /*  XML_PARSE_H_ */
	#ifdef  PROCESSOR_H_
	Processor p_Mproc;
	Core *p_Mcore;
	SharedCache* l2array;
        SharedCache* l3array;
        SharedCache* l1dirarray;
        SharedCache* l2dirarray;
        NoC* nocs;
        MemoryController* mc;
	#endif /* PROCESSOR_H_*/
	#ifdef  CORE_H_
	InstFetchU * ifu;
	LoadStoreU * lsu;
	MemManU    * mmu;
	EXECU      * exu;
	RENAMINGU  * rnu;
        Pipeline   * corepipe;
        UndiffCore * undiffCore;	
	SharedCache * l2cache;//mcpat08
        CoreDynParam  coredynp;
	
	ArrayST * globalBPT;
	ArrayST * localBPT; //
	ArrayST * L1_localBPT;
	ArrayST * L2_localBPT;
	ArrayST * chooser;
	ArrayST * RAS;
	
	InstCache icache;
	ArrayST * IB;
	ArrayST * BTB;
	BranchPredictor * BPT;
	inst_decoder * ID_inst;
	inst_decoder * ID_operand;
	inst_decoder * ID_misc;

	ArrayST         * int_inst_window;
	ArrayST         * fp_inst_window;
	ArrayST         * ROB;
        selection_logic * instruction_selection;

	ArrayST * iFRAT;
	ArrayST * fFRAT;
	ArrayST * iRRAT;
	ArrayST * fRRAT;
	ArrayST * ifreeL;
	ArrayST * ffreeL;
	dep_resource_conflict_check * idcl;
	dep_resource_conflict_check * fdcl;
	ArrayST * RAHT;//mcpat08, register alias history table Used to store GC


	DataCache dcache;
	ArrayST * LSQ;//it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
	ArrayST * LoadQ;

	ArrayST * itlb;
	ArrayST * dtlb;

	ArrayST * IRF;
	ArrayST * FRF;
	ArrayST * RFWIN;

	RegFU          * rfu;
	SchedulerU     * scheu;
        FunctionalUnit * fp_u;
        FunctionalUnit * exeu;
	FunctionalUnit * mul; //
	McPATComponent  bypass;
	interconnect * int_bypass;
	interconnect * intTagBypass;
	interconnect * int_mul_bypass;//
	interconnect * intTag_mul_Bypass;//
	interconnect * fp_bypass;
	interconnect * fpTagBypass;
	#endif /* CORE_H_*/
	#endif /*McPAT07_H*/

	#ifdef INTSIM_H
	IntSim_library *intsim_il1, *intsim_il2, *intsim_dl1, *intsim_dl2, *intsim_itlb, *intsim_dtlb;
	IntSim_library *intsim_clock, *intsim_bpred, *intsim_rf, *intsim_io, *intsim_logic, *intsim_alu, *intsim_fpu, *intsim_mult;
	IntSim_library *intsim_ib, *intsim_issueQ, *intsim_decoder, *intsim_bypass, *intsim_exeu, *intsim_pipeline;
	IntSim_library *intsim_lsq, *intsim_rat, *intsim_rob, *intsim_btb, *intsim_L2, *intsim_mc, *intsim_router, *intsim_loadQ;
	IntSim_library *intsim_rename, *intsim_scheduler, *intsim_L3, *intsim_L1dir, *intsim_L2dir;
	#endif


   public:
       	//Default constructor
	Power();
        Power(ComponentId_t compID) {
            PowerInit(compID);}
        void PowerInit(ComponentId_t compID) {
            p_compID = compID;
	    p_powerLevel = 1; p_powerMonitor = false; p_tempMonitor = false; //p_powerModel = McPAT;  // This setting is important since params are not read in order as they apprears
	    p_meanPeak = p_meanPeakAll = 0.0;
	    p_areaMcPAT = 0.0; p_maxNumSubComp = 0;
	    p_ifReadEntireXML = p_ifGetMcPATUnitP = false;
	    p_hasUpdatedTemp = false;
	    p_TotalFailureRate = 0;
	    p_NumSamples = 0;
	    //p_McPATonPipe = p_McPATonIRS = p_McPATonRF = false;  //if xxx has power estimated by McPAT already
	    #ifdef McPAT05_H
	    McPAT05initBasic(); //initialize basic InputParameter interface_ip
	    #endif

            // power usage initialization
	    memset(&p_usage_cache_il1,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_il2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dl1,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dl2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_itlb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dtlb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_clock,0,sizeof(Pdissipation_t));
	    memset(&p_usage_io,0,sizeof(Pdissipation_t));
	    memset(&p_usage_logic,0,sizeof(Pdissipation_t));
	    memset(&p_usage_alu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_fpu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_mult,0,sizeof(Pdissipation_t));
	    memset(&p_usage_uarch,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rf,0,sizeof(Pdissipation_t));
	    memset(&p_usage_bpred,0,sizeof(Pdissipation_t));
	    memset(&p_usage_ib,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rs,0,sizeof(Pdissipation_t));
	    memset(&p_usage_decoder,0,sizeof(Pdissipation_t));
	    memset(&p_usage_bypass,0,sizeof(Pdissipation_t));
	    memset(&p_usage_exeu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_pipeline,0,sizeof(Pdissipation_t));
	    memset(&p_usage_lsq,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rat,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rob,0,sizeof(Pdissipation_t));
	    memset(&p_usage_btb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_mc,0,sizeof(Pdissipation_t));
	    memset(&p_usage_router,0,sizeof(Pdissipation_t));
	    memset(&p_usage_loadQ,0,sizeof(Pdissipation_t));
	    memset(&p_usage_renameU,0,sizeof(Pdissipation_t));
	    memset(&p_usage_schedulerU,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l3,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l1dir,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l2dir,0,sizeof(Pdissipation_t));
	    memset(&p_unitPower,0,sizeof(Punit_t));
	    memset(&floorplan_id,0,sizeof(floorplan_id_t));
	    // device params initilaization
	    device_tech.clockRate = 1200000000.0; device_tech.machineType = 1; 
	    device_tech.number_L1dir=1; device_tech.number_L2dir=1;
	    device_tech.number_L2=1; device_tech.number_L3=1;
	    device_tech.number_il1=1; device_tech.number_dl1=1;
	    device_tech.number_itlb=1; device_tech.number_dtlb=1;
	    device_tech.number_core=1;
            // cache params initilaization
            /* cache_il1 */
            cache_il1_tech.unit_scap.push_back(16384.0); cache_il1_tech.vss = 0.0; cache_il1_tech.op_freq = 0; cache_il1_tech.num_sets = 0;	    
            cache_il1_tech.line_size.push_back(32); cache_il1_tech.num_bitlines = 0; cache_il1_tech.num_wordlines = 0; cache_il1_tech.assoc.push_back(4);
	    cache_il1_tech.unit_icap = 0.0; cache_il1_tech.unit_lcap = 0.0; cache_il1_tech.unit_ecap = 0;
	    cache_il1_tech.num_rwports = cache_il1_tech.num_rports = cache_il1_tech.num_wports = 0; cache_il1_tech.num_banks.push_back(1);
	    cache_il1_tech.throughput.push_back(1.0); cache_il1_tech.latency.push_back(3.0);
cache_il1_tech.output_width.push_back(8.0); cache_il1_tech.cache_policy.push_back(0);
 cache_il1_tech.core_physical_address_width = 52;
	    cache_il1_tech.miss_buf_size.push_back(16); cache_il1_tech.fill_buf_size.push_back(16); cache_il1_tech.prefetch_buf_size.push_back(16); cache_il1_tech.wbb_buf_size.push_back(0);
	    cache_il1_tech.core_virtual_address_width = 64; cache_il1_tech.core_virtual_memory_page_size = 4096; cache_il1_tech.core_number_hardware_threads = 4;	
	    cache_il1_tech.number_entries = 0; cache_il1_tech.core_temperature=380; cache_il1_tech.core_tech_node=90;  cache_il1_tech.device_type = 0; cache_il1_tech.area = 0.41391e-6; cache_il1_tech.num_transistors = 0.584e6;
	    cache_il1_tech.directory_type = 1;  
            /* cache_il2 */
            cache_il2_tech.unit_scap.push_back(0.0); cache_il2_tech.vss = 0.0; cache_il2_tech.op_freq = 0; cache_il2_tech.num_sets = 0;
            cache_il2_tech.line_size.push_back(0); cache_il2_tech.num_bitlines = 0; cache_il2_tech.num_wordlines = 0; cache_il2_tech.assoc.push_back(0);
	    cache_il2_tech.unit_icap = 0.0; cache_il2_tech.unit_lcap = 0.0; cache_il2_tech.unit_ecap = 0;
	    cache_il2_tech.num_rwports = cache_il2_tech.num_rports = cache_il2_tech.num_wports = 0; cache_il2_tech.num_banks.push_back(0);
	    cache_il2_tech.throughput.push_back(0.0); cache_il2_tech.latency.push_back(0.0);
cache_il2_tech.output_width.push_back(0); cache_il2_tech.cache_policy.push_back(0);
cache_il2_tech.core_physical_address_width = 0;
	    cache_il2_tech.miss_buf_size.push_back(0); cache_il2_tech.fill_buf_size.push_back(0); cache_il2_tech.prefetch_buf_size.push_back(0); cache_il2_tech.wbb_buf_size.push_back(0);
	    cache_il2_tech.core_virtual_address_width = cache_il2_tech.core_virtual_memory_page_size = cache_il2_tech.core_number_hardware_threads = 0;	
	    cache_il2_tech.number_entries = 0; cache_il2_tech.core_temperature=380; cache_il2_tech.core_tech_node=90; cache_il2_tech.device_type = 0;  	  
	    cache_il2_tech.directory_type = 1;  cache_il2_tech.area = 0.41391e-6; cache_il2_tech.num_transistors = 0.584e6;
            /* cache_dl1 */
            cache_dl1_tech.unit_scap.push_back(8192.0); cache_dl1_tech.vss = 0.0; cache_dl1_tech.op_freq = 0; cache_dl1_tech.num_sets = 0;
            cache_dl1_tech.line_size.push_back(16); cache_dl1_tech.num_bitlines = 0; cache_dl1_tech.num_wordlines = 0; cache_dl1_tech.assoc.push_back(4);
	    cache_dl1_tech.unit_icap = 0.0; cache_dl1_tech.unit_lcap = 0.0; cache_dl1_tech.unit_ecap = 0;
	    cache_dl1_tech.num_rwports = cache_dl1_tech.num_rports = cache_dl1_tech.num_wports = 1; cache_dl1_tech.num_banks.push_back(1);
	    cache_dl1_tech.throughput.push_back(1.0); cache_dl1_tech.latency.push_back(3.0);
cache_dl1_tech.output_width.push_back(16.0); cache_dl1_tech.cache_policy.push_back(0); cache_dl1_tech.core_physical_address_width = 52;
	    cache_dl1_tech.miss_buf_size.push_back(16); cache_dl1_tech.fill_buf_size.push_back(16); cache_dl1_tech.prefetch_buf_size.push_back(16); cache_dl1_tech.wbb_buf_size.push_back(16);
	    cache_dl1_tech.core_virtual_address_width = cache_dl1_tech.core_virtual_memory_page_size = cache_dl1_tech.core_number_hardware_threads = 4;	
	    cache_dl1_tech.number_entries = 0; cache_dl1_tech.core_temperature=380; cache_dl1_tech.core_tech_node=90; cache_dl1_tech.device_type = 0;
	    cache_dl1_tech.directory_type = 1; cache_dl1_tech.area = 0.41391e-6; cache_dl1_tech.num_transistors = 0.584e6;	    
            /* cache_dl2 */
            cache_dl2_tech.unit_scap.push_back(0.0); cache_dl2_tech.vss = 0.0; cache_dl2_tech.op_freq = 0; cache_dl2_tech.num_sets = 0;
            cache_dl2_tech.line_size.push_back(0); cache_dl2_tech.num_bitlines = 0; cache_dl2_tech.num_wordlines = 0; cache_dl2_tech.assoc.push_back(0);
	    cache_dl2_tech.unit_icap = 0.0; cache_dl2_tech.unit_lcap = 0.0; cache_dl2_tech.unit_ecap = 0;
	    cache_dl2_tech.num_rwports = cache_dl2_tech.num_rports = cache_dl2_tech.num_wports = 0; cache_dl2_tech.num_banks.push_back(0);
	    cache_dl2_tech.throughput.push_back(0.0); cache_dl2_tech.latency.push_back(0.0);
cache_dl2_tech.output_width.push_back(0); cache_dl2_tech.cache_policy.push_back(0); cache_dl2_tech.core_physical_address_width = 0;
	    cache_dl2_tech.miss_buf_size.push_back(0); cache_dl2_tech.fill_buf_size.push_back(0); cache_dl2_tech.prefetch_buf_size.push_back(0); cache_dl2_tech.wbb_buf_size.push_back(0);
	    cache_dl2_tech.core_virtual_address_width = cache_dl2_tech.core_virtual_memory_page_size = cache_dl2_tech.core_number_hardware_threads = 0;	
	    cache_dl2_tech.number_entries = 0; cache_dl2_tech.core_temperature=380; cache_dl2_tech.core_tech_node=90; cache_dl2_tech.device_type = 0;
	    cache_dl2_tech.directory_type = 1; cache_dl2_tech.area = 0.41391e-6; cache_dl2_tech.num_transistors = 0.584e6; 	    
            /* cache_itlb */
            cache_itlb_tech.unit_scap.push_back(0.0); cache_itlb_tech.vss = 0.0; cache_itlb_tech.op_freq = 0; cache_itlb_tech.num_sets = 0;
            cache_itlb_tech.line_size.push_back(0); cache_itlb_tech.num_bitlines = 0; cache_itlb_tech.num_wordlines = 0; cache_itlb_tech.assoc.push_back(0);
	    cache_itlb_tech.unit_icap = 0.0; cache_itlb_tech.unit_lcap = 0.0; cache_itlb_tech.unit_ecap = 0;
	    cache_itlb_tech.num_rwports = cache_itlb_tech.num_rports = cache_itlb_tech.num_wports = 0; cache_itlb_tech.num_banks.push_back(0);
	    cache_itlb_tech.throughput.push_back(0); cache_itlb_tech.latency.push_back(0.0);
cache_itlb_tech.output_width.push_back(0); cache_itlb_tech.cache_policy.push_back(0); cache_itlb_tech.core_physical_address_width = 0;
	    cache_itlb_tech.miss_buf_size.push_back(0); cache_itlb_tech.fill_buf_size.push_back(0); cache_itlb_tech.prefetch_buf_size.push_back(0); cache_itlb_tech.wbb_buf_size.push_back(0);	
	    cache_itlb_tech.core_virtual_address_width = 64; cache_itlb_tech.core_virtual_memory_page_size = 4096; 
	    cache_itlb_tech.core_number_hardware_threads = 4; cache_itlb_tech.core_physical_address_width = 52; cache_itlb_tech.number_entries = 64;   
	    cache_itlb_tech.core_temperature=380; cache_itlb_tech.core_tech_node=90;  cache_itlb_tech.device_type = 0;
	    cache_itlb_tech.directory_type = 1; cache_itlb_tech.area = 0.41391e-6; cache_itlb_tech.num_transistors = 0.584e6;
            /* cache_dtlb */
            cache_dtlb_tech.unit_scap.push_back(0.0); cache_dtlb_tech.vss = 0.0; cache_dtlb_tech.op_freq = 0; cache_dtlb_tech.num_sets = 0;
            cache_dtlb_tech.line_size.push_back(0); cache_dtlb_tech.num_bitlines = 0; cache_dtlb_tech.num_wordlines = 0; cache_dtlb_tech.assoc.push_back(0);
	    cache_dtlb_tech.unit_icap = 0.0; cache_dtlb_tech.unit_lcap = 0.0; cache_dtlb_tech.unit_ecap = 0;
	    cache_dtlb_tech.num_rwports = cache_dtlb_tech.num_rports = cache_dtlb_tech.num_wports = 0; cache_dtlb_tech.num_banks.push_back(0);
	    cache_dtlb_tech.throughput.push_back(0.0); cache_dtlb_tech.latency.push_back(0.0);
cache_dtlb_tech.output_width.push_back(0); cache_dtlb_tech.cache_policy.push_back(0); cache_dtlb_tech.core_physical_address_width = 0;
	    cache_dtlb_tech.miss_buf_size.push_back(0); cache_dtlb_tech.fill_buf_size.push_back(0); cache_dtlb_tech.prefetch_buf_size.push_back(0); cache_dtlb_tech.wbb_buf_size.push_back(0);	  
	    cache_dtlb_tech.core_virtual_address_width = 64; cache_dtlb_tech.core_virtual_memory_page_size = 4096; 
	    cache_dtlb_tech.core_number_hardware_threads = 4; cache_dtlb_tech.core_physical_address_width = 52; cache_dtlb_tech.number_entries = 64; 
	    cache_dtlb_tech.core_temperature=380; cache_dtlb_tech.core_tech_node=90; cache_dtlb_tech.device_type = 0; 
	    cache_dtlb_tech.directory_type = 1; cache_dtlb_tech.area = 0.41391e-6; cache_dtlb_tech.num_transistors = 0.584e6;
            /*clock*/
	    clock_tech.unit_scap=0.0; clock_tech.unit_icap=0.0; clock_tech.unit_lcap=0.0; clock_tech.vss=0.0; 
	    clock_tech.op_freq=0; clock_tech.clk_style=NORM_H; clock_tech.skew=0.0; clock_tech.chip_area=0; 
	    clock_tech.node_cap=0.0; clock_tech.opt_clock_buffer_num=0; clock_tech.unit_ecap=0.0;
	    clock_tech.core_temperature=380; clock_tech.core_tech_node=90; clock_tech.area = 0.41391e-6; clock_tech.num_transistors = 0.584e6; clock_tech.clock_option = GLOBAL_CLOCK;
	    /*bpred*/
	    bpred_tech.unit_icap=0.0; bpred_tech.unit_ecap=0.0; bpred_tech.vss=0.0;
	    bpred_tech.op_freq=0; bpred_tech.unit_scap=0.0; bpred_tech.bpred_access=0; bpred_tech.nrows=0; bpred_tech.ncols=0;
	    bpred_tech.num_rwports = bpred_tech.num_rports = bpred_tech.num_wports = 0;	  
	    bpred_tech.global_predictor_bits=2; bpred_tech.global_predictor_entries=4096; bpred_tech.prediction_width=0; bpred_tech.local_predictor_size=10;
	    bpred_tech.local_predictor_entries=1024; bpred_tech.chooser_predictor_bits=2; bpred_tech.chooser_predictor_entries=4096;  bpred_tech.area = 0.41391e-6; bpred_tech.num_transistors = 0.584e6; 
	    /*rf*/
	    rf_tech.unit_scap=0.0; rf_tech.unit_icap=0.0; rf_tech.unit_ecap=0.0; rf_tech.vss=0.0;
	    rf_tech.op_freq=0; rf_tech.rf_access=0; rf_tech.nrows=0; rf_tech.ncols=0;
	    rf_tech.num_rwports = rf_tech.num_rports = rf_tech.num_wports = 0;	
	    rf_tech.machine_bits = 64; rf_tech.archi_Regs_IRF_size = 32; rf_tech.archi_Regs_FRF_size = 32; rf_tech.core_issue_width = 1;  
	    rf_tech.core_register_windows_size = 8;  rf_tech.core_number_hardware_threads = 4;	    
	    rf_tech.core_temperature=380; rf_tech.core_tech_node=90; rf_tech.core_opcode_width =8;  rf_tech.core_virtual_address_width = 64; rf_tech.area = 0.41391e-6; rf_tech.num_transistors = 0.584e6;
 	    /*io*/
	    io_tech.unit_scap=0.0; io_tech.unit_icap=0.0; io_tech.unit_lcap=0.0; io_tech.vss=0.0; io_tech.op_freq=0;
	    io_tech.i_o_style=OUT; io_tech.opt_io_buffer_num=0; io_tech.ustrip_len=0.0; io_tech.bus_width=0;	
	    io_tech.bus_size=0; io_tech.io_access_time=0; io_tech.io_cycle_time=0; io_tech.unit_ecap=0.0;
	    io_tech.area = 0.41391e-6; io_tech.num_transistors = 0.584e6;	    		
	    /*logic*/
	    logic_tech.unit_scap=0.0; logic_tech.unit_icap=0.0; logic_tech.unit_lcap=0.0; logic_tech.vss=0.0; 
	    logic_tech.op_freq=0; logic_tech.lgc_style=STATIC; logic_tech.num_gates=0; logic_tech.num_functions=0;
	    logic_tech.num_fan_in=0; logic_tech.num_fan_out=0; logic_tech.unit_ecap=0.0;
	    logic_tech.core_instruction_window_size = 64; logic_tech.core_issue_width = 1; logic_tech.core_number_hardware_threads = 4;
	    logic_tech.core_decode_width = 1;  logic_tech.archi_Regs_IRF_size = 32; logic_tech.archi_Regs_FRF_size = 32;	
	    logic_tech.core_temperature=380; logic_tech.core_tech_node=90;   
	    logic_tech.area = 0.41391e-6; logic_tech.num_transistors = 0.584e6; 
            /*alu*/
	    alu_tech.unit_scap=50.0; alu_tech.unit_icap=0.0; alu_tech.unit_lcap=0.0; alu_tech.vss=0.0;
	    alu_tech.op_freq=0; alu_tech.unit_ecap=0.0; alu_tech.area = 0.41391e-6; alu_tech.num_transistors = 0.584e6;	  
	    /*fpu*/
	    fpu_tech.unit_scap=350.0; fpu_tech.unit_icap=0.0; fpu_tech.unit_lcap=0.0; fpu_tech.vss=0.0;
	    fpu_tech.op_freq=0; fpu_tech.unit_ecap=0.0; fpu_tech.area = 0.41391e-6; fpu_tech.num_transistors = 0.584e6;      
	    /*mult*/
	    mult_tech.unit_scap=0.0; mult_tech.unit_icap=0.0; mult_tech.unit_lcap=0.0; mult_tech.vss=0.0;
	    mult_tech.op_freq=0; mult_tech.unit_ecap=0.0; mult_tech.area = 0.41391e-6; mult_tech.num_transistors = 0.584e6;
	    /*IB*/
	    ib_tech.core_instruction_length = 32; ib_tech.core_issue_width = 1; ib_tech.core_number_hardware_threads = 4;
	    ib_tech.core_instruction_buffer_size = 20; ib_tech.num_rwports = 1; ib_tech.core_temperature=380; ib_tech.core_tech_node=90;
	    ib_tech.core_virtual_address_width = 64; ib_tech.core_virtual_memory_page_size = 4096; ib_tech.area = 0.41391e-6; ib_tech.num_transistors = 0.584e6;
	    /*IRS*/
	    irs_tech.core_number_hardware_threads = 4;  irs_tech.core_instruction_length = 32;  irs_tech.core_instruction_window_size = 64;
	    irs_tech.core_issue_width = 1;   
	    irs_tech.core_temperature=380; irs_tech.core_tech_node=90; irs_tech.area = 0.41391e-6; irs_tech.num_transistors = 0.584e6;
	    #ifdef McPAT05_H 
		perThreadState = 4; //from McPAT
	    #endif
	    /*INST_DECODER*/
	    decoder_tech.core_opcode_width = 8; decoder_tech.core_temperature=380; decoder_tech.core_tech_node=90;
	    decoder_tech.area = 0.41391e-6; decoder_tech.num_transistors = 0.584e6;
	    /*BYPASS*/
	    bypass_tech.core_number_hardware_threads = 4;  bypass_tech.ALU_per_core = 3;   bypass_tech.machine_bits = 64; 
	    bypass_tech.FPU_per_core = 1; bypass_tech.core_opcode_width = 8; bypass_tech.core_virtual_address_width =64; bypass_tech.machine_bits = 64;
	    bypass_tech.core_store_buffer_size =32; bypass_tech.core_memory_ports = 1; bypass_tech.core_temperature=380; bypass_tech.core_tech_node=90; bypass_tech.area = 0.41391e-6; bypass_tech.num_transistors = 0.584e6;
	    /*EXEU*/
	    #ifdef McPAT05_H
	    C_EXEU = 100.0; //pF
	    #endif
	    /*PIPELINE*/
	    pipeline_tech.core_number_hardware_threads = 4;  pipeline_tech.core_fetch_width = 1; pipeline_tech.core_decode_width = 1;
	    pipeline_tech.core_issue_width = 1; pipeline_tech.core_commit_width = 1; pipeline_tech.core_instruction_length = 32;
	    pipeline_tech.core_virtual_address_width = 64;  pipeline_tech.core_opcode_width = 8; pipeline_tech.core_int_pipeline_depth = 12;
	    pipeline_tech.machine_bits = 64;  pipeline_tech.archi_Regs_IRF_size = 32; 	pipeline_tech.core_temperature=380; pipeline_tech.core_tech_node=90; pipeline_tech.area = 0.41391e-6; pipeline_tech.num_transistors = 0.584e6;
	    /*schedulerU*/
	    #ifdef McPAT06_H 
		perThreadState = 8; //from McPAT
	    #endif   
	    #ifdef McPAT07_H 
		perThreadState = 8; //from McPAT
	    #endif  
	    /*uarch*/
	    uarch_tech.unit_scap=0.0; uarch_tech.unit_icap=0.0; uarch_tech.unit_lcap=0.0; uarch_tech.vss=0.0;
	    uarch_tech.op_freq=0; uarch_tech.unit_ecap=0.0;
	    /*btb*/
            btb_tech.unit_scap = 8192.0; btb_tech.vss = 0.0; btb_tech.op_freq = 0; 
            btb_tech.line_size = 4; btb_tech.assoc = 2; btb_tech.num_banks = 1;
	    btb_tech.throughput =1.0; btb_tech.latency = 3.0; btb_tech.area = 0.41391e-6; btb_tech.num_transistors = 0.584e6;
	    /*core--McPAT07*/
	    core_tech.core_physical_address_width=52; core_tech.core_temperature=380; core_tech.core_tech_node=90;
	    core_tech.core_virtual_address_width =64; core_tech.core_virtual_memory_page_size=4096; core_tech.core_number_hardware_threads=4;		
	    core_tech.machine_bits=64; core_tech.archi_Regs_IRF_size=32; core_tech.archi_Regs_FRF_size=32;
	    core_tech.core_issue_width=1; core_tech.core_register_windows_size=8; core_tech.core_opcode_width=9;	
	    core_tech.core_instruction_window_size=16; core_tech.core_decode_width=1; core_tech.core_instruction_length=32;
   	    core_tech.core_instruction_buffer_size=16; core_tech.ALU_per_core=1; core_tech.FPU_per_core=1; core_tech.MUL_per_core=1; core_tech.core_ROB_size = 80;	
	    core_tech.core_store_buffer_size=32; core_tech.core_load_buffer_size=32; core_tech.core_memory_ports=1; core_tech.core_fetch_width=1;
	    core_tech.core_commit_width=1; core_tech.core_int_pipeline_depth=6; core_tech.core_phy_Regs_IRF_size=80; core_tech.core_phy_Regs_FRF_size=80; core_tech.core_RAS_size=32;	
	    core_tech.core_number_of_NoCs = 1; 	core_tech.core_number_instruction_fetch_ports = 1; core_tech.core_fp_issue_width = 1; core_tech.core_fp_instruction_window_size =16;
	    core_tech.core_long_channel = 1; core_tech.core_peak_issue_width=4; core_tech.core_micro_opcode_width=11;
	    /*core--McPAT06*/
	    /*core_tech.core_physical_address_width=52; core_tech.core_temperature=360; core_tech.core_tech_node=32;
	    core_tech.core_virtual_address_width =64; core_tech.core_virtual_memory_page_size=4096; core_tech.core_number_hardware_threads=2;		
	    core_tech.machine_bits=64; core_tech.archi_Regs_IRF_size=32; core_tech.archi_Regs_FRF_size=32;
	    core_tech.core_issue_width=4; core_tech.core_register_windows_size=8; core_tech.core_opcode_width=11;	
	    core_tech.core_instruction_window_size=64; core_tech.core_decode_width=4; core_tech.core_instruction_length=32;
   	    core_tech.core_instruction_buffer_size=20; core_tech.ALU_per_core=1; core_tech.FPU_per_core=1; core_tech.core_ROB_size = 80;	
	    core_tech.core_store_buffer_size=8; core_tech.core_load_buffer_size=32; core_tech.core_memory_ports=1; core_tech.core_fetch_width=4;
	    core_tech.core_commit_width=4; core_tech.core_int_pipeline_depth=12; core_tech.core_phy_Regs_IRF_size=80; core_tech.core_phy_Regs_FRF_size=80; core_tech.core_RAS_size=32;	
	    core_tech.core_number_of_NoCs = 1; 	core_tech.core_number_instruction_fetch_ports = 1; core_tech.core_fp_issue_width = 1; core_tech.core_fp_instruction_window_size =64;*/
	    /*core--McPAT05*/
	    /*core_tech.core_physical_address_width=52; core_tech.core_temperature=360; core_tech.core_tech_node=65;
	    core_tech.core_virtual_address_width =64; core_tech.core_virtual_memory_page_size=4096; core_tech.core_number_hardware_threads=4;		
	    core_tech.machine_bits=64; core_tech.archi_Regs_IRF_size=32; core_tech.archi_Regs_FRF_size=32;
	    core_tech.core_issue_width=1; core_tech.core_register_windows_size=8; core_tech.core_opcode_width=8;	
	    core_tech.core_instruction_window_size=64; core_tech.core_decode_width=1; core_tech.core_instruction_length=32;
   	    core_tech.core_instruction_buffer_size=20; core_tech.ALU_per_core=3; core_tech.FPU_per_core=1; core_tech.core_ROB_size = 80;	
	    core_tech.core_store_buffer_size=32; core_tech.core_load_buffer_size=32; core_tech.core_memory_ports=1; core_tech.core_fetch_width=1;
	    core_tech.core_commit_width=1; core_tech.core_int_pipeline_depth=12; core_tech.core_phy_Regs_IRF_size=80; core_tech.core_phy_Regs_FRF_size=80; core_tech.core_RAS_size=32;	
	    core_tech.core_number_of_NoCs = 1;*/
	    /*cache_l2*/
	    cache_l2_tech.unit_scap.push_back(786432.0); cache_l2_tech.vss = 0.0; cache_l2_tech.op_freq = 3500000000.0; cache_l2_tech.num_sets = 0;	    
            cache_l2_tech.line_size.push_back(64); cache_l2_tech.num_bitlines = 0; cache_l2_tech.num_wordlines = 0; cache_l2_tech.assoc.push_back(16);
	    cache_l2_tech.unit_icap = 0.0; cache_l2_tech.unit_lcap = 0.0; cache_l2_tech.unit_ecap = 0;
	    cache_l2_tech.num_rwports = cache_l2_tech.num_rports = cache_l2_tech.num_wports = 1; cache_l2_tech.num_banks.push_back(1);
	    cache_l2_tech.throughput.push_back(4.0); cache_l2_tech.latency.push_back(23.0);
cache_l2_tech.output_width.push_back(64.0); cache_l2_tech.cache_policy.push_back(1); cache_l2_tech.core_physical_address_width = 52;
	    cache_l2_tech.miss_buf_size.push_back(16); cache_l2_tech.fill_buf_size.push_back(16); cache_l2_tech.prefetch_buf_size.push_back(16); cache_l2_tech.wbb_buf_size.push_back(16);
	    cache_l2_tech.core_virtual_address_width = cache_l2_tech.core_virtual_memory_page_size = cache_l2_tech.core_number_hardware_threads = 4;	
	    cache_l2_tech.number_entries = 0; cache_l2_tech.core_temperature=380; cache_l2_tech.core_tech_node=90; cache_l2_tech.device_type = 1; 
	    cache_l2_tech.directory_type = 1; cache_l2_tech.area = 0.41391e-6; cache_l2_tech.num_transistors = 0.584e6;
	    /*cache_l3*/
	    cache_l3_tech.unit_scap.push_back(1048576.0); cache_l3_tech.vss = 0.0; cache_l3_tech.op_freq = 3500000000.0; cache_l3_tech.num_sets = 0;	    
            cache_l3_tech.line_size.push_back(64); cache_l3_tech.num_bitlines = 0; cache_l3_tech.num_wordlines = 0; cache_l3_tech.assoc.push_back(16);
	    cache_l3_tech.unit_icap = 0.0; cache_l3_tech.unit_lcap = 0.0; cache_l3_tech.unit_ecap = 0;
	    cache_l3_tech.num_rwports = cache_l3_tech.num_rports = cache_l3_tech.num_wports = 1; cache_l3_tech.num_banks.push_back(1);
	    cache_l3_tech.throughput.push_back(2.0); cache_l3_tech.latency.push_back(100.0);
cache_l3_tech.output_width.push_back(64.0); cache_l3_tech.cache_policy.push_back(1); cache_l3_tech.core_physical_address_width = 52;
	    cache_l3_tech.miss_buf_size.push_back(16); cache_l3_tech.fill_buf_size.push_back(16); cache_l3_tech.prefetch_buf_size.push_back(16); cache_l3_tech.wbb_buf_size.push_back(16);
	    cache_l3_tech.core_virtual_address_width = cache_l3_tech.core_virtual_memory_page_size = cache_l3_tech.core_number_hardware_threads = 4;	
	    cache_l3_tech.number_entries = 0; cache_l3_tech.core_temperature=380; cache_l3_tech.core_tech_node=90; cache_l3_tech.device_type = 0;
	    cache_l3_tech.directory_type = 1; cache_l3_tech.area = 0.41391e-6; cache_l3_tech.num_transistors = 0.584e6;
	    /*cache_l1dir*/
	    cache_l1dir_tech.unit_scap.push_back(2048.0); cache_l1dir_tech.vss = 0.0; cache_l1dir_tech.op_freq = 3500000000.0; cache_l1dir_tech.num_sets = 0;	    
            cache_l1dir_tech.line_size.push_back(1); cache_l1dir_tech.num_bitlines = 0; cache_l1dir_tech.num_wordlines = 0; cache_l1dir_tech.assoc.push_back(0);
	    cache_l1dir_tech.unit_icap = 0.0; cache_l1dir_tech.unit_lcap = 0.0; cache_l1dir_tech.unit_ecap = 0;
	    cache_l1dir_tech.num_rwports = cache_l1dir_tech.num_rports = cache_l1dir_tech.num_wports =1; cache_l1dir_tech.num_banks.push_back(1);
	    cache_l1dir_tech.throughput.push_back(4.0); cache_l1dir_tech.latency.push_back(4.0);
cache_l1dir_tech.output_width.push_back(1.0); cache_l1dir_tech.cache_policy.push_back(1); cache_l1dir_tech.core_physical_address_width = 52;
	    cache_l1dir_tech.miss_buf_size.push_back(8); cache_l1dir_tech.fill_buf_size.push_back(8); cache_l1dir_tech.prefetch_buf_size.push_back(8); cache_l1dir_tech.wbb_buf_size.push_back(8);
	    cache_l1dir_tech.core_virtual_address_width = cache_l1dir_tech.core_virtual_memory_page_size = cache_l1dir_tech.core_number_hardware_threads = 4;	
	    cache_l1dir_tech.number_entries = 0; cache_l1dir_tech.core_temperature=380; cache_l1dir_tech.core_tech_node=90; cache_l1dir_tech.device_type = 0; 
	    cache_l1dir_tech.directory_type = 0; cache_l1dir_tech.area = 0.41391e-6; cache_l1dir_tech.num_transistors = 0.584e6;
	    /*cache_l2dir*/
	    cache_l2dir_tech.unit_scap.push_back(1048576.0); cache_l2dir_tech.vss = 0.0; cache_l2dir_tech.op_freq = 3500000000.0; cache_l2dir_tech.num_sets = 0;	    
            cache_l2dir_tech.line_size.push_back(16); cache_l2dir_tech.num_bitlines = 0; cache_l2dir_tech.num_wordlines = 0; cache_l2dir_tech.assoc.push_back(16);
	    cache_l2dir_tech.unit_icap = 0.0; cache_l2dir_tech.unit_lcap = 0.0; cache_l2dir_tech.unit_ecap = 0;
	    cache_l2dir_tech.num_rwports = cache_l2dir_tech.num_rports = cache_l2dir_tech.num_wports = 1; cache_l2dir_tech.num_banks.push_back(1);
	    cache_l2dir_tech.throughput.push_back(2.0); cache_l2dir_tech.latency.push_back(100.0);
cache_l2dir_tech.output_width.push_back(0.0); cache_l2dir_tech.cache_policy.push_back(0); cache_l2dir_tech.core_physical_address_width = 52;
	    cache_l2dir_tech.miss_buf_size.push_back(8); cache_l2dir_tech.fill_buf_size.push_back(8); cache_l2dir_tech.prefetch_buf_size.push_back(8); cache_l2dir_tech.wbb_buf_size.push_back(8);
	    cache_l2dir_tech.core_virtual_address_width = cache_l2dir_tech.core_virtual_memory_page_size = cache_l2dir_tech.core_number_hardware_threads = 4;	
	    cache_l2dir_tech.number_entries = 0; cache_l2dir_tech.core_temperature=380; cache_l2dir_tech.core_tech_node=90; cache_l2dir_tech.device_type = 0;
	    cache_l2dir_tech.directory_type = 1; cache_l2dir_tech.area = 0.41391e-6; cache_l2dir_tech.num_transistors = 0.584e6;
	    /*mc*/
	    mc_tech.mc_clock=200000000.0; mc_tech.llc_line_length=64; mc_tech.databus_width=128; mc_tech.addressbus_width=51; mc_tech.req_window_size_per_channel=32;
	    mc_tech.memory_channels_per_mc=1; mc_tech.IO_buffer_size_per_channel=32; 
	    mc_tech.memory_number_ranks=2; mc_tech.memory_peak_transfer_rate=3200;
	    mc_tech.area = 0.41391e-6; mc_tech.num_transistors = 0.584e6;
	    /*router*/
	    router_tech.clockrate=1200000000.0; router_tech.vdd=1.5; router_tech.has_global_link=1; router_tech.flit_bits=128; router_tech.input_buffer_entries_per_vc=2;
	    router_tech.virtual_channel_per_port=1; router_tech.input_ports=8; router_tech.horizontal_nodes=2; router_tech.vertical_nodes=1;
	    router_tech.output_ports=5; router_tech.link_throughput=1; router_tech.link_latency=1; router_tech.topology = RING; router_tech.area = 0.41391e-6; router_tech.num_transistors = 0.584e6;
	    router_tech.link_length = 15500; //unit is micron
	    /*floorplan*/
	    floorplan_id.il1.push_back(-1); floorplan_id.il2.push_back(-1); floorplan_id.dl1.push_back(-1); 
	    floorplan_id.dl2.push_back(-1); floorplan_id.itlb.push_back(-1); floorplan_id.dtlb.push_back(-1);
	    floorplan_id.clock.push_back(-1); floorplan_id.bpred.push_back(-1); floorplan_id.rf.push_back(-1);
	    floorplan_id.io.push_back(-1); floorplan_id.logic.push_back(-1); floorplan_id.alu.push_back(-1);
	    floorplan_id.fpu.push_back(-1); floorplan_id.mult.push_back(-1); floorplan_id.ib.push_back(-1);
	    floorplan_id.issueQ.push_back(-1); floorplan_id.decoder.push_back(-1); floorplan_id.bypass.push_back(-1);
	    floorplan_id.exeu.push_back(-1); floorplan_id.pipeline.push_back(-1); floorplan_id.lsq.push_back(-1);
	    floorplan_id.rat.push_back(-1); floorplan_id.rob.push_back(-1); floorplan_id.btb.push_back(-1);
	    floorplan_id.L2.push_back(-1); floorplan_id.mc.push_back(-1); floorplan_id.router.push_back(-1);
	    floorplan_id.loadQ.push_back(-1); floorplan_id.rename.push_back(-1); floorplan_id.scheduler.push_back(-1);
	    floorplan_id.L3.push_back(-1); floorplan_id.L1dir.push_back(-1); floorplan_id.L2dir.push_back(-1);

	    #ifdef LV2_PANALYZER_H
	    il1_pspec = NULL; il2_pspec = NULL; dl1_pspec = NULL; dl2_pspec = NULL;  itlb_pspec = NULL;
	    dtlb_pspec = NULL;  clock_pspec = NULL;  logic_pspec = NULL;  mult_pspec = NULL;	
	    bpred_pspec = NULL; rf_pspec = NULL; alu_pspec = NULL;  fpu_pspec = NULL;            
	    #endif  
	    #ifdef IO_PANALYZER_H  
	    aio_pspec = dio_pspec = NULL;
	    #endif
	    #ifdef  XML_PARSE_H_
	    p_Mp1= new ParseXML();
	    #endif /*XML_PARSE_H_*/
        }

	//Destructor
	virtual ~Power() {}

	void setTech(ComponentId_t compID, Component::Params_t params, ptype power_type, pmodel power_model);
	void getUnitPower(ptype power_type, int user_data, pmodel power_model);
	//Pdissipation_t& getPower(Cycle_t current, ptype power_type, char *user_parms, int total_cycles);
	//Pdissipation_t& getPower(Cycle_t current, ptype power_type, usagecounts_t counts, int total_cycles);
	Pdissipation_t& getPower(IntrospectedComponent* c, ptype power_type, usagecounts_t counts);  //execution time = total cycles/clock rate
	//void updatePowUsage(Pdissipation_t *comp_pusage, const I& totalPowerUsage, const I& dynamicPower, const I& leakage, const I& TDP, Cycle_t current);
	void updatePowUsage(IntrospectedComponent *c, ptype power_type, int fid, Pdissipation_t *comp_pusage, const I& totalPowerUsage, const I& dynamicPower, const I& leakage, const I& TDP);
	double estimateClockDieAreaSimPan();
	double estimateClockNodeCapSimPan();
	double estimateAreaMcPAT(){return p_areaMcPAT*1e-6;};
	void resetCounts(usagecounts_t *counts);
  	I getExecutionTime(IntrospectedComponent *c);
	void setTech(Component::Params_t deviceParams); // called by setTech to set up device params values and store subcomp floorplan id information
	void setChip(Component::Params_t deviceParams);
	void floorParamInitialize();
	void updateFloorplanAreaInfo(int fid, double area);
	void compute_temperature(ComponentId_t compID);
	void leakage_feedback(pmodel power_model, parameters_tech_t device_tech, ptype power_type);
	void printFloorplanAreaInfo();
	void printFloorplanPowerInfo();
	void printFloorplanThermalInfo();
	void compute_MTTF(); //this is done offline at the end of simulation?!
	void getTemperatureStatistics();
	void setupDPM(int block_id, power_state pstate);
	void dynamic_power_management();
	void test();

	// McPAT interface
	#ifdef McPAT05_H
	void McPAT05Setup();
	/*the following are no longer used*/
	void McPAT05initBasic();
	void McPATinitIcache();
	void McPATinitDcache();
	void McPATinitItlb();
	void McPATinitDtlb();
	void McPATinitIB();
	void McPATinitIRS();
	void McPATinitRF();
	void McPATinitBypass();
	void McPATinitLogic();
	void McPATinitDecoder();
	void McPATinitPipeline();
	void McPATinitClock();
	#endif

	#ifdef McPAT06_H
	void McPAT06Setup();
	#endif
	#ifdef McPAT07_H
	void McPATSetup();
	#endif

	friend class boost::serialization::access;
	template<class Archive>
    	void serialize(Archive & ar, const unsigned int version );

	/*
	BOOST_SERIALIZE {
            _AR_DBG(Power,"start\n");
            ar & BOOST_SERIALIZATION_NVP(p_compID);
            ar & BOOST_SERIALIZATION_NVP(p_powerLevel);
            ar & BOOST_SERIALIZATION_NVP(p_powerMonitor);
            ar & BOOST_SERIALIZATION_NVP(p_powerModel);
	    ar & BOOST_SERIALIZATION_NVP(p_usage_cache_il1);
            _AR_DBG(Power,"done\n");
        }

        SAVE_CONSTRUCT_DATA(Power)
        {
            _AR_DBG(Power,"\n");
            ComponentId_t p_compID = t->p_compID;
            ar << BOOST_SERIALIZATION_NVP( p_compID );
        }

        LOAD_CONSTRUCT_DATA(Power)
        {
            _AR_DBG(Power,"\n");
            ComponentId_t p_compID = t->p_compID;
            ar >> BOOST_SERIALIZATION_NVP( p_compID );
            ::new(t)Power( p_compID );
        }
	*/

};
}
#endif // POWER_H




