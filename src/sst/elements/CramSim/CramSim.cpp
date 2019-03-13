// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SST includes
#include "sst_config.h"

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"

// local includes
#include "c_MemhBridge.hpp"
#include "c_AddressHasher.hpp"
#include "c_TxnConverter.hpp"
#include "c_TxnScheduler.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_DeviceDriver.hpp"
#include "c_Dimm.hpp"
#include "c_Controller.hpp"
#include "c_TxnDispatcher.hpp"
#include "c_TxnGen.hpp"
#include "c_TraceFileReader.hpp"


// namespaces
using namespace SST;
using namespace SST::n_Bank;
using namespace SST::Statistics;

/*----ALLOCATORS FOR COMPONENTS----*/

// c_MemhBridge
static Component*
create_c_MemhBridge(SST::ComponentId_t id, SST::Params& params) {
	return new c_MemhBridge(id, params);
}

// c_TxnGen
static Component*
create_c_TxnGen(SST::ComponentId_t id, SST::Params& params) {
	return new c_TxnGen(id, params);
}

// c_TraceFileReader
static Component*
create_c_TraceFileReader(SST::ComponentId_t id, SST::Params& params){
	return new c_TraceFileReader(id, params);
}


// c_TxnDispatcher
static Component*
create_c_TxnDispatcher(SST::ComponentId_t id, SST::Params& params) {
	return new c_TxnDispatcher(id, params);
}
// c_Dimm
static Component*
create_c_Dimm(SST::ComponentId_t id, SST::Params& params) {
	return new c_Dimm(id, params);
}

// c_Controller
static Component*
create_c_Controller(SST::ComponentId_t id, SST::Params& params) {
	return new c_Controller(id, params);
}
// Address Mapper
static SubComponent*
create_c_AddressHasher(Component * owner, Params& params) {
	return new c_AddressHasher(owner, params);
}

// Transaction Converter
static SubComponent*
create_c_TxnScheduler(Component * owner, Params& params) {
	return new c_TxnScheduler(owner, params);
}

// Transaction Converter
static SubComponent*
create_c_TxnConverter(Component * owner, Params& params) {
	return new c_TxnConverter(owner, params);
}

// Command Scheduler
static SubComponent*
create_c_DeviceDriver(Component * owner, Params& params) {
	return new c_DeviceDriver(owner, params);
}

// Device Controller
static SubComponent*
create_c_CmdScheduler(Component * owner, Params& params) {
	return new c_CmdScheduler(owner, params);
}

static const char* c_TxnDispatcher_port_events[] = { "MemEvent", NULL };

/*----SETUP c_AddressHasher  STRUCTURES----*/
static const ElementInfoParam c_TxnDispatcher_params[] = {
		{"numLanes", "Total number of lanes", NULL},
		{"laneIdxPosition", "Bit posiiton of the lane index in the address.. [End:Start]", NULL},
		{ NULL, NULL, NULL } };


static const ElementInfoPort c_TxnDispatcher_ports[] = {
		{ "txnGen", "link to/from a transaction generator",c_TxnDispatcher_port_events},
		{ "lane_%(lanes)d", "link to/from lanes", c_TxnDispatcher_port_events},
		{ NULL, NULL, NULL } };

/*----SETUP c_AddressHasher  STRUCTURES----*/
static const ElementInfoParam c_AddressHasher_params[] = {
		{"numChannelsPerDimm", "Total number of channels per DIMM", NULL},
		{"numRanksPerChannel", "Total number of ranks per channel", NULL},
		{"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
		{"numBanksPerBankGroup", "Total number of banks per group", NULL},
		{"numRowsPerBank" "Number of rows in every bank", NULL},
		{"numColsPerBank", "Number of cols in every bank", NULL},
		{"numBytesPerTransaction", "Number of bytes retrieved for every transaction", NULL},
		{"strAddressMapStr","String defining the address mapping scheme",NULL},
		{ NULL, NULL, NULL } };

//static const ElementInfoPort c_AddressHasher_ports[] = {
//		{ NULL, NULL, NULL } };

/*----SETUP c_MemhBridge STRUCTURES----*/
static const ElementInfoParam c_MemhBridge_params[] = {
                {"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
		{ NULL, NULL, NULL } };

static const char* c_MemhBridge_mem_port_events[] = { "c_TxnReqEvent","c_TxnResEvent", NULL };
static const char* c_MemhBridge_CPU_events[] = {"c_CPUevent", NULL};

static const ElementInfoPort c_MemhBridge_ports[] = {
		{ "cpuLink", "link to/from CPU",c_MemhBridge_CPU_events},
		{ "memLink", "link to memory-side components (txn dispatcher or controller)", c_MemhBridge_mem_port_events },
		{ NULL, NULL, NULL } };

static const ElementInfoStatistic c_MemhBridge_stats[] = {
  {"readTxnsSent", "Number of read transactions sent", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsSent", "Number of write transactions sent", "writes", 1}, // Name, Desc, Units, Enable Level
  {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
  {"txnsPerCycle", "Transactions Per Cycle", "Txns/Cycle", 1},
  {"readTxnsLatency", "Average latency of read transactions", "cycles", 1},
  {"writeTxnsLatency", "Average latency of write transactions", "cycles", 1},
  {"txnsLatency", "Average latency of (read/write) transactions", "cycles", 1},
  {NULL, NULL, NULL, 0}
};

/*----SETUP c_TxnGen STRUCTURES----*/
static const ElementInfoParam c_TxnGen_params[] = {
		{"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
		{"numTxnPerCycle", "The number of transactions generated per cycle", NULL},
		{"readWriteRatio", "Ratio of read txn's to generate : write txn's to generate", NULL},
		{ NULL, NULL, NULL } };

static const char* c_TxnGen_port_events[] = { "c_TxnReqEvent", "c_TxnResEvent", NULL };

static const ElementInfoPort c_TxnGen_ports[] = {
		{ "memLink", "link to memory-side components (txn dispatcher or controller)", c_TxnGen_port_events },
		{ NULL, NULL, NULL } };

static const ElementInfoStatistic c_TxnGen_stats[] = {
  {"readTxnsSent", "Number of read transactions sent", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsSent", "Number of write transactions sent", "writes", 1}, // Name, Desc, Units, Enable Level
  {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
  {"txnsPerCycle", "Transactions Per Cycle", "Txns/Cycle", 1},
  {"readTxnsLatency", "Average latency of read transactions", "cycles", 1},
  {"writeTxnsLatency", "Average latency of write transactions", "cycles", 1},
  {"txnsLatency", "Average latency of (read/write) transactions", "cycles", 1},
  {NULL, NULL, NULL, 0}
};

/*----SETUP c_TracefileReader STRUCTURES----*/
static const ElementInfoParam c_TraceFileReader_params[] = {
		{"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
		{"numTxnPerCycle", "The number of transactions generated per cycle", NULL},
		{"traceFile", "Location of trace file to read", NULL},
                {"traceFileType", "Trace file type (DEFAULT or USIMM)",NULL},
		{ NULL, NULL, NULL } };

static const char* c_TraceFileReader_port_events[] = { "c_TxnReqEvent", "c_TxnResEvent", NULL };

static const ElementInfoPort c_TraceFileReader_ports[] = {
		{ "memLink", "link to memory-side components (txn dispatcher or controller)", c_TraceFileReader_port_events },
		{ NULL, NULL, NULL } };

static const ElementInfoStatistic c_TraceFileReader_stats[] = {
  {"readTxnsSent", "Number of read transactions sent", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsSent", "Number of write transactions sent", "writes", 1}, // Name, Desc, Units, Enable Level
  {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
  {"txnsPerCycle", "Transactions Per Cycle", "Txns/Cycle", 1},
  {"readTxnsLatency", "Average latency of read transactions", "cycles", 1},
  {"writeTxnsLatency", "Average latency of write transactions", "cycles", 1},
  {"txnsLatency", "Average latency of (read/write) transactions", "cycles", 1},
  {NULL, NULL, NULL, 0}
};

/*----SETUP c_TxnScheduler STRUCTURES----*/
static const ElementInfoParam c_TxnScheduler_params[] = {
		{"txnSchedulingPolicy", "Transaction scheduling policy", NULL},
		{"numTxnQEntries", "The number of transaction queue entries", NULL},
		{NULL, NULL, NULL } };

static const ElementInfoStatistic c_TxnScheduler_stats[] = {
		{NULL, NULL, NULL, 0}
};

/*----SETUP c_TxnConverter STRUCTURES----*/
static const ElementInfoParam c_TxnConverter_params[] = {
		{"relCommandWidth", "Relative width of each command", NULL},
		{"bankPolicy", "Select which bank policy to model", NULL},
		{"boolUseReadA", "Whether to use READ or READA Cmds", NULL},
		{"boolUseWriteA", "Whether to use WRITE or WRITEA Cmds", NULL},
		{NULL, NULL, NULL } };

static const ElementInfoStatistic c_TxnConverter_stats[] = {
  {"readTxnsRecvd", "Number of read transactions received", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsRecvd", "Number of write transactions received", "writes", 1},
  {"totalTxnsRecvd", "Number of write transactions received", "transactions", 1},
  {"reqQueueSize", "Total size of the request queue over time", "transactions", 1},
  {"resQueueSize", "Total size of the response queue over time", "transactions", 1},
  {NULL, NULL, NULL, 0}
};


/*----SETUP c_CmdScheduler STRUCTURES----*/
static const ElementInfoParam c_CmdScheduler_params[] = {
		{"numCmdQEntries", "The number of entries in command scheduler's command queue"},
		{NULL, NULL, NULL } };

static const ElementInfoStatistic c_CmdScheduler_stats[] = {
		{NULL, NULL, NULL, 0}
};



/*----SETUP c_DeviceDriver STRUCTURES----*/
static const ElementInfoParam c_DeviceDriver_params[] = {
		{"numChannels", "Total number of channels per DIMM", NULL},
		{"numPChannelsPerChannel", "Total number of channels per pseudo channel (added to support HBM)", NULL},
		{"numRanksPerChannel", "Total number of ranks per (p)channel", NULL},
		{"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
		{"numBanksPerBankGroup", "Total number of banks per group", NULL},
		{"numRowsPerBank" "Number of rows in every bank", NULL},
		{"numColsPerBank", "Number of cols in every bank", NULL},
		{"boolPrintCmdTrace", "Print a command trace", NULL},
		{"strCmdTraceFile", "Filename to print the command trace, or - for stdout", NULL},
		{"boolAllocateCmdResACT", "Allocate space in DeviceDriver Res Q for ACT Cmds", NULL},
		{"boolAllocateCmdResREAD", "Allocate space in DeviceDriver Res Q for READ Cmds", NULL},
		{"boolAllocateCmdResREADA", "Allocate space in DeviceDriver Res Q for READA Cmds", NULL},
		{"boolAllocateCmdResWRITE", "Allocate space in DeviceDriver Res Q for WRITE Cmds", NULL},
		{"boolAllocateCmdResWRITEA", "Allocate space in DeviceDriver Res Q for WRITEA Cmds", NULL},
		{"boolAllocateCmdResPRE", "Allocate space in DeviceDriver Res Q for PRE Cmds", NULL},
		{"boolUseRefresh", "Whether to use REF or not", NULL},
		{"boolDualCommandBus", "Whether to use dual command bus (added to support HBM)", NULL},
		{"boolMultiCycleACT", "Whether to use multi-cycle (two cycles) active command (added to support HBM)", NULL},
		{"nRC", "Bank Param", NULL},
		{"nRRD", "Bank Param", NULL},
		{"nRRD_L", "Bank Param", NULL},
		{"nRRD_S", "Bank Param", NULL},
		{"nRCD", "Bank Param", NULL},
		{"nCCD", "Bank Param", NULL},
		{"nCCD_L", "Bank Param", NULL},
		{"nCCD_L_WR", "Bank Param", NULL},
		{"nCCD_S", "Bank Param", NULL},
		{"nAL", "Bank Param", NULL},
		{"nCL", "Bank Param", NULL},
		{"nCWL", "Bank Param", NULL},
		{"nWR", "Bank Param", NULL},
		{"nWTR", "Bank Param", NULL},
		{"nWTR_L", "Bank Param", NULL},
		{"nWTR_S", "Bank Param", NULL},
		{"nRTW", "Bank Param", NULL},
		{"nEWTR", "Bank Param", NULL},
		{"nERTW", "Bank Param", NULL},
		{"nEWTW", "Bank Param", NULL},
		{"nERTR", "Bank Param", NULL},
		{"nRAS", "Bank Param", NULL},
		{"nRTP", "Bank Param", NULL},
		{"nRP", "Bank Param", NULL},
		{"nRFC", "Bank Param", NULL},
		{"nREFI", "Bank Param", NULL},
		{"nFAW", "Bank Param", NULL},
		{"nBL", "Bank Param", NULL},
		{NULL, NULL, NULL } };

static const ElementInfoStatistic c_DeviceDriver_stats[] = {
  //{"rowHits", "Number of DRAM page buffer hits", "hits", 1}, // Name, Desc, Units, Enable Level
  {NULL,NULL,NULL,0}
};


/*----SETUP Controller Component STRUCTURES----*/
static const ElementInfoParam c_Controller_params[] = {
		{"AddrMapper", "address hasher", "CramSim.c_AddressHasher"},
		{"TxnScheduler", "Transaction Scheduler", "CramSim.c_TxnScheduler"},
		{"TxnConverter", "Transaction Converter", "CramSim.c_TxnConverter"},
		{"CmdScheduler", "Command Scheduler", "CramSim.c_CmdScheduler"},
		{"DeviceDriver", "device driver", "CramSim.c_DeviceDriver"},
		{NULL, NULL, NULL } };

static const char* c_Controller_TxnGenReq_port_events[] = { "c_txnGenReqEvent", NULL };
static const char* c_Controller_TxnGenRes_port_events[] = { "c_txnGenResEvent", NULL };
static const char* c_Controller_DeviceReq_port_events[] = { "c_DeviceReqEvent", NULL };
static const char* c_Controller_DeviceRes_port_events[] = { "c_DeviceResEvent", NULL };
static const char* c_Controller_TxnGenResToken_port_events[] = { "c_txnGenResTokenEvent", NULL };
static const char* c_Controller_TxnGenReqToken_port_events[] = { "c_txnGenReqTokenEvent", NULL };
static const char* c_Controller_DeviceReqToken_port_events[] = { "c_DeviceReqTokenEvent", NULL };

static const ElementInfoPort c_Controller_ports[] = {
		{"txngenLink", "link to txn generator / txn dispatcher", c_Controller_TxnGenReq_port_events},
		{"memLink", "link to memory", c_Controller_DeviceRes_port_events},
		{NULL, NULL, NULL}
};


/*----SETUP Memory Component STRUCTURES----*/
static const ElementInfoParam c_Dimm_params[] = {
                 {"numRanksPerChannel", "Total number of ranks per channel", NULL},
                 {"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
                 {"numBanksPerBankGroup", "Total number of banks per group", NULL},
                 {"boolAllocateCmdResACT", "Allocate space in Controller Res Q for ACT Cmds", NULL},
                 {"boolAllocateCmdResREAD", "Allocate space in Controller Res Q for READ Cmds", NULL},
                 {"boolAllocateCmdResREADA", "Allocate space in Controller Res Q for READA Cmds", NULL},
                 {"boolAllocateCmdResWRITE", "Allocate space in Controller Res Q for WRITE Cmds", NULL},
                 {"boolAllocateCmdResWRITEA", "Allocate space in Controller Res Q for WRITEA Cmds", NULL},
                 {"boolAllocateCmdResPRE", "Allocate space in Controller Res Q for PRE Cmds", NULL},
                 {NULL, NULL, NULL } };

static const char* c_Dimm_cmdReq_port_events[] = { "c_CmdReqEvent", NULL };
static const char* c_Dimm_cmdRes_port_events[] = { "c_CmdResEvent", NULL };

static const ElementInfoPort c_Dimm_ports[] = {
		{"ctrlLink", "link to controller", c_Dimm_cmdReq_port_events},
		{NULL, NULL, NULL}
};

static const ElementInfoStatistic c_Dimm_stats[] = {
  {"actCmdsRecvd", "Number of activate commands received", "activates", 1}, // Name, Desc, Units, Enable Level
  {"readCmdsRecvd", "Number of read commands received", "reads", 1}, 
  {"readACmdsRecvd", "Number of read with autoprecharge commands received", "readAs", 1},
  {"writeCmdsRecvd", "Number of write commands received", "writes", 1}, 
  {"writeACmdsRecvd", "Number of write with autoprecharge commands received", "writeAs", 1},
  {"preCmdsRecvd", "Number of precharge commands received", "precharges", 1},
  {"refCmdsRecvd", "Number of refresh commands received", "refreshes", 1},
  {"totalRowHits", "Number of total row hits", "hits", 1},
  {"bankACTsRecvd", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits", "Number of row hits at bank x", "commands", 5},
  {NULL, NULL, NULL, 0}
};


static const ElementInfoComponent CramSimComponents[] = {
		{ "c_TxnGen", 							// Name
		"Test Txn Generator",			// Description
		NULL, 										// PrintHelp
		create_c_TxnGen, 						// Allocator
		c_TxnGen_params, 						// Parameters
		c_TxnGen_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		c_TxnGen_stats 										// Statistics
		},
		{ "c_TraceFileReader", 							// Name
		"Test Trace file Generator",			 	// Description
		NULL, 										// PrintHelp
		create_c_TraceFileReader, 						// Allocator
		c_TraceFileReader_params, 						// Parameters
		c_TraceFileReader_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		c_TraceFileReader_stats							// Statistics
		},
		{ "c_Dimm",			 						// Name
		"Test DIMM",				 				// Description
		NULL, 										// PrintHelp
		create_c_Dimm, 						// Allocator
		c_Dimm_params, 						// Parameters
		c_Dimm_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		c_Dimm_stats									// Statistics
		},
		{ "c_MemhBridge",			 						// Name
		"Bridge to communicate with MemoryHierarchy",				 				// Description
		NULL, 										// PrintHelp
		create_c_MemhBridge, 						// Allocator
		c_MemhBridge_params, 						// Parameters
		c_MemhBridge_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		c_MemhBridge_stats 										// Statistics
		},
		{ "c_Controller",			 						// Name
			"Memory Controller",				 				// Description
			NULL, 										// PrintHelp
					create_c_Controller, 						// Allocator
					c_Controller_params, 						// Parameters
					c_Controller_ports, 						// Ports
					COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
					NULL 										// Statistics
		},
		{ "c_TxnDispatcher",			 						// Name
			"Transaction dispatcher",				 				// Description
			NULL, 										// PrintHelp
					create_c_TxnDispatcher, 						// Allocator
					c_TxnDispatcher_params, 						// Parameters
					c_TxnDispatcher_ports, 						// Ports
					COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
					NULL 										// Statistics
		},
		{ NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL } };


static const ElementInfoSubComponent CramSimSubComponents[] = {
		{ "c_AddressHasher", 							// Name
		  "Hashes addresses based on config parameters",			// Description
		  NULL, 										// PrintHelp
		  create_c_AddressHasher, 						// Allocator
		  c_AddressHasher_params, 						// Parameters
		  NULL,										//Interface
		  "SST::CramSim::Controller::AddressHasher" 			// Category
		},
		{ "c_TxnScheduler",	 							// Name
		"Transaction Scheduler",				 			// Description
		NULL, 										// PrintHelp
		create_c_TxnScheduler, 							// Allocator
		c_TxnScheduler_params, 							// Parameters
		c_TxnScheduler_stats,
				"SST::CramSim::Controller::TxnScheduler" 			// Category
		},
		{ "c_TxnConverter",	 							// Name
		"Transaction Converter",				 			// Description
		NULL, 										// PrintHelp
		create_c_TxnConverter, 							// Allocator
		c_TxnConverter_params, 							// Parameters
		c_TxnConverter_stats,
				"SST::CramSim::Controller::TxnConverter" 			// Category
		},
		{"c_DeviceDriver",                                // Name
			"Dram Control Unit",                            // Description
			NULL,                                        // PrintHelp
			create_c_DeviceDriver,                            // Allocator
			c_DeviceDriver_params,                            // Parameters
			c_DeviceDriver_stats,
				"SST::CramSim::Controller::DeviceDriver"            // Category
		},
		{"c_CmdScheduler",                                // Name
				"Command Scheduler",                            // Description
				NULL,                                        // PrintHelp
				create_c_CmdScheduler,                            // Allocator
				c_CmdScheduler_params,                            // Parameters
				c_CmdScheduler_stats,
				"SST::CramSim::Controller::CmdScheduler"            // Category
		},
		{ NULL, NULL, NULL, NULL, NULL, NULL}
};


extern "C" {
ElementLibraryInfo CramSim_eli = { "CramSim", // Name
		"Library with transaction generation components", // Description
		CramSimComponents, // Components
		NULL, // Events
		NULL, // Introspectors
		NULL, // Modules
		CramSimSubComponents, // Subcomponents
		NULL, // Partitioners
		NULL, // Python Module Generator
		NULL // Generators
		};
}
