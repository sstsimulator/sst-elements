// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include "sst/core/element.h"

// local includes
#include "c_MemhBridge.hpp"
#include "c_AddressHasher.hpp"
#include "c_TxnGenSeq.hpp"
#include "c_TxnGenRand.hpp"
#include "c_TracefileReader.hpp"
#include "c_DramSimTraceReader.hpp"
#include "c_USimmTraceReader.hpp"
#include "c_TxnDriver.hpp"
#include "c_TxnUnit.hpp"
#include "c_CmdDriver.hpp"
#include "c_CmdUnit.hpp"
#include "c_BankReceiver.hpp"
#include "c_Dimm.hpp"


// namespaces
using namespace SST;
using namespace SST::n_Bank;
using namespace SST::n_CmdDriver;
using namespace SST::n_TxnDriver;
using namespace SST::n_BankReceiver;

/*----ALLOCATORS FOR COMPONENTS----*/

static Component*
create_c_AddressHasher(SST::ComponentId_t id, SST::Params& params) {
	return new c_AddressHasher(id, params);
}

// c_MemhBridge
static Component*
create_c_MemhBridge(SST::ComponentId_t id, SST::Params& params) {
	return new c_MemhBridge(id, params);
}

// c_TxnGenSeq
static Component*
create_c_TxnGenSeq(SST::ComponentId_t id, SST::Params& params) {
	return new c_TxnGenSeq(id, params);
}

// c_TxnGenRand
static Component*
create_c_TxnGenRand(SST::ComponentId_t id, SST::Params& params){
	return new c_TxnGenRand(id, params);
}

// c_TracefileReader
static Component*
create_c_TracefileReader(SST::ComponentId_t id, SST::Params& params){
	return new c_TracefileReader(id, params);
}

// c_DramSimTraceReader
static Component*
create_c_DramSimTraceReader(SST::ComponentId_t id, SST::Params& params){
	return new c_DramSimTraceReader(id, params);
}

// c_USimmTraceReader
static Component*
create_c_USimmTraceReader(SST::ComponentId_t id, SST::Params& params){
	return new c_USimmTraceReader(id, params);
}

// c_TxnDriver
static Component*
create_c_TxnDriver(SST::ComponentId_t id, SST::Params& params){
	return new c_TxnDriver(id, params);
}

// c_TxnUnit
static Component*
create_c_TxnUnit(SST::ComponentId_t id, SST::Params& params) {
	return new c_TxnUnit(id, params);
}

// c_CmdDriver
static Component*
create_c_CmdDriver(SST::ComponentId_t id, SST::Params& params){
	return new c_CmdDriver(id, params);
}

// c_CmdUnit
static Component*
create_c_CmdUnit(SST::ComponentId_t id, SST::Params& params){
	return new c_CmdUnit(id, params);
}

// c_BankReceiver
static Component*
create_c_BankReceiver(SST::ComponentId_t id, SST::Params& params){
	return new c_BankReceiver(id, params);
}

// c_Dimm
static Component*
create_c_Dimm(SST::ComponentId_t id, SST::Params& params) {
	return new c_Dimm(id, params);
}

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

static const ElementInfoPort c_AddressHasher_ports[] = {
		{ NULL, NULL, NULL } };

/*----SETUP c_MemhBridge STRUCTURES----*/
static const ElementInfoParam c_MemhBridge_params[] = {
		{"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
		{"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
		{ NULL, NULL, NULL } };

static const char* c_MemhBridge_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_MemhBridge_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_MemhBridge_token_port_events[] = {"c_TokenChgEvent", NULL};
static const char* c_MemhBridge_CPU_events[] = {"c_CPUevent", NULL};

static const ElementInfoPort c_MemhBridge_ports[] = {
		{ "linkCPU", "link to/from CPU",c_MemhBridge_CPU_events},
		{ "outTxnGenReqPtr", "link to c_MemhBridge for outgoing req txn", c_MemhBridge_req_port_events },
		{ "inTxnUnitReqQTokenChg", "link to c_MemhBridge for incoming req token", c_MemhBridge_token_port_events },
		{ "inTxnUnitResPtr", "link to c_MemhBrdige for incoming res txn", c_MemhBridge_res_port_events },
		{ "outTxnGenResQTokenChg", "link to c_MemhBridge for outgoing res token",c_MemhBridge_token_port_events },
		{ NULL, NULL, NULL } };

/*----SETUP c_TxnGenSeq STRUCTURES----*/
static const ElementInfoParam c_TxnGenSeq_params[] = {
		{"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
		{"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
		{"readWriteRatio", "Ratio of read txn's to generate : write txn's to generate", NULL},
		{ NULL, NULL, NULL } };

static const char* c_TxnGenSeq_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_TxnGenSeq_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_TxnGenSeq_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_TxnGenSeq_ports[] = {
		{ "outTxnGenReqPtr", "link to c_TxnGen for outgoing req txn", c_TxnGenSeq_req_port_events },
		{ "inTxnUnitReqQTokenChg", "link to c_TxnGen for incoming req token", c_TxnGenSeq_token_port_events },
		{ "inTxnUnitResPtr", "link to c_TxnGen for incoming res txn", c_TxnGenSeq_res_port_events },
		{ "outTxnGenResQTokenChg", "link to c_TxnGen for outgoing res token",c_TxnGenSeq_token_port_events },
		{ NULL, NULL, NULL } };


/*----SETUP c_TxnGenRand STRUCTURES----*/
static const ElementInfoParam c_TxnGenRand_params[] = {
		{"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
		{"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
		{"readWriteRatio", "Ratio of read txn's to generate : write txn's to generate", NULL},
		{ NULL, NULL, NULL } };

static const char* c_TxnGenRand_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_TxnGenRand_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_TxnGenRand_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_TxnGenRand_ports[] = {
		{ "outTxnGenReqPtr", "link to c_TxnGen for outgoing req txn", c_TxnGenRand_req_port_events },
		{ "inTxnUnitReqQTokenChg", "link to c_TxnGen for incoming req token", c_TxnGenRand_token_port_events },
		{ "inTxnUnitResPtr", "link to c_TxnGen for incoming res txn", c_TxnGenRand_res_port_events },
		{ "outTxnGenResQTokenChg", "link to c_TxnGen for outgoing res token",c_TxnGenRand_token_port_events },
		{ NULL, NULL, NULL } };

/*----SETUP c_TracefileReader STRUCTURES----*/
static const ElementInfoParam c_TracefileReader_params[] = {
		{"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
		{"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
		{"traceFile", "Location of trace file to read", NULL},
		{ NULL, NULL, NULL } };

static const char* c_TracefileReader_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_TracefileReader_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_TracefileReader_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_TracefileReader_ports[] = {
		{ "outTxnGenReqPtr", "link to c_TxnGen for outgoing req txn", c_TxnGenRand_req_port_events },
		{ "inTxnUnitReqQTokenChg", "link to c_TxnGen for incoming req token", c_TxnGenRand_token_port_events },
		{ "inTxnUnitResPtr", "link to c_TxnGen for incoming res txn", c_TxnGenRand_res_port_events },
		{ "outTxnGenResQTokenChg", "link to c_TxnGen for outgoing res token",c_TxnGenRand_token_port_events },
		{ NULL, NULL, NULL } };


/*----SETUP c_DramSimTraceReader STRUCTURES----*/
static const ElementInfoParam c_DramSimTraceReader_params[] = {
		{"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
		{"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
		{"traceFile", "Location of trace file to read", NULL},
		{ NULL, NULL, NULL } };

static const char* c_DramSimTraceReader_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_DramSimTraceReader_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_DramSimTraceReader_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_DramSimTraceReader_ports[] = {
		{ "outTxnGenReqPtr", "link to c_TxnGen for outgoing req txn", c_TxnGenRand_req_port_events },
		{ "inTxnUnitReqQTokenChg", "link to c_TxnGen for incoming req token", c_TxnGenRand_token_port_events },
		{ "inTxnUnitResPtr", "link to c_TxnGen for incoming res txn", c_TxnGenRand_res_port_events },
		{ "outTxnGenResQTokenChg", "link to c_TxnGen for outgoing res token",c_TxnGenRand_token_port_events },
		{ NULL, NULL, NULL } };


/*----SETUP c_USimmTraceReader STRUCTURES----*/
static const ElementInfoParam c_USimmTraceReader_params[] = {
  {"numTxnGenReqQEntries", "Total entries allowed in the Req queue", NULL},
  {"numTxnGenResQEntries", "Total entries allowed in the Res queue", NULL},
  {"numTxnUnitReqQEntries", "Total entries in the neighbor TxnUnit's Req queue", NULL},
  {"traceFile", "Location of trace file to read", NULL},
  { NULL, NULL, NULL } };

static const char* c_USimmTraceReader_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_USimmTraceReader_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_USimmTraceReader_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_USimmTraceReader_ports[] = {
  { "outTxnGenReqPtr", "link to c_TxnGen for outgoing req txn", c_TxnGenRand_req_port_events },
  { "inTxnUnitReqQTokenChg", "link to c_TxnGen for incoming req token", c_TxnGenRand_token_port_events },
  { "inTxnUnitResPtr", "link to c_TxnGen for incoming res txn", c_TxnGenRand_res_port_events },
  { "outTxnGenResQTokenChg", "link to c_TxnGen for outgoing res token",c_TxnGenRand_token_port_events },
  { NULL, NULL, NULL } };



/*----SETUP c_TxnDriver STRUCTURES----*/
static const ElementInfoParam c_TxnDriver_params[] = {
		{"numTxnDrvBufferQEntries", "Total entries allowed in the buffer", NULL},
		{"numTxnGenResQEntries", "Total entries allowed in the Res queue of the Txn Driver", NULL},
		{ NULL, NULL, NULL } };


static const char* c_TxnDriver_req_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_TxnDriver_res_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_TxnDriver_token_port_events[] = {"c_TokenChgEvent", NULL};
static const char* c_TxnDriver_txnreqtoken_port_events[] = {"c_TxnReqTokenChgEvent", NULL};

static const ElementInfoPort c_TxnDriver_ports[] = {
		{ "outTxnDrvReqQTokenChg", "link to c_TxnDriver for outgoing req token", c_TxnDriver_txnreqtoken_port_events },
		{ "inTxnGenReqPtr", "link to c_TxnGen for incoming req txn", c_TxnGenRand_req_port_events },
		{ "inTxnGenResQTokenChg", "link to c_TxnGen for incoming res token", c_TxnGenRand_token_port_events },
		{ "outTxnDrvResPtr", "link to c_TxnGen for outgoing res txn",c_TxnGenRand_res_port_events },
		{ NULL, NULL, NULL } };


/*----SETUP c_TxnUnit STRUCTURES----*/
static const ElementInfoParam c_TxnUnit_params[] = {
		{"numTxnUnitReqQEntries", "Total number of entries in TxnUnit's Req queue", NULL},
		{"numTxnUnitResQEntries", "Total number of entries in TxnUnit's Res queue", NULL},
		{"numTxnGenResQEntries", "Total number of entries in neighbor TxnGen's Res queue", NULL},
		{"numCmdReqQEntries", "Total number of entries in neighbor CmdUnit's Req queue", NULL},
		{"numChannelsPerDimm", "Total number of channels per DIMM", NULL},
		{"numRanksPerChannel", "Total number of ranks per channel", NULL},
		{"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
		{"numBanksPerBankGroup", "Total number of banks per group", NULL},
		{"numRowsPerBank" "Number of rows in every bank", NULL},
		{"numColsPerBank", "Number of cols in every bank", NULL},
		{"numBytesPerTransaction", "Number of bytes retrieved for every transaction", NULL},
		{"relCommandWidth", "Relative width of each command", NULL},
		{"bankPolicy", "Select which bank policy to model", NULL},
		{"nREFI", "Bank param for how often refreshes happen", NULL},
		{"boolUseRefresh", "Whether to use REF or not", NULL},
		{"boolUseReadA", "Whether to use READ or READA Cmds", NULL},
		{"boolUseWriteA", "Whether to use WRITE or WRITEA Cmds", NULL},
		{NULL, NULL, NULL } };

static const char* c_TxnUnit_txnReq_port_events[] = { "c_TxnReqEvent", NULL };
static const char* c_TxnUnit_txnRes_port_events[] = { "c_TxnResEvent", NULL };
static const char* c_TxnUnit_cmdReq_port_events[] = { "c_CmdPtrPkgEvent", NULL };
static const char* c_TxnUnit_token_port_events[] = {"c_TokenChgEvent", NULL};
static const char* c_TxnUnit_cmdRes_port_events[] = {"c_CmdResEvent", NULL};


static const ElementInfoPort c_TxnUnit_ports[] = {
		{"inTxnGenReqPtr", "link to c_TxnUnit for incoming req txn", c_TxnUnit_txnReq_port_events},
		{"outCmdUnitReqPtrPkg", "link to c_TxnUnit for outgoing req cmds", c_TxnUnit_cmdReq_port_events},
		{"outTxnGenReqQTokenChg", "link to c_TxnUnit for outgoing req txn tokens", c_TxnUnit_token_port_events},
		{"inCmdUnitReqQTokenChg", "link to c_TxnUnit for incoming req cmd tokens", c_TxnUnit_token_port_events},
		{"inCmdUnitResPtr", "link to c_TxnUnit for incoming res txns from CmdUnit", c_TxnUnit_cmdRes_port_events},
		{"outTxnGenResPtr", "link to c_TxnUnit for outgoing res txns", c_TxnUnit_txnRes_port_events},
		{"inTxnGenResQTokenChg", "link to c_TxnUnit for incoming res txn tokens", c_TxnUnit_token_port_events},
		{NULL, NULL, NULL}
};




/*----SETUP c_CmdDriver STRUCTURES----*/
static const ElementInfoParam c_CmdDriver_params[] = {
		{"numCmdReqQEntries", "Total number of entries in Driver's buffer", NULL},
		{"numTxnResQEntries", "Total number of entries in neighbor TxnUnit's Res queue", NULL},
		{NULL, NULL, NULL } };

static const char* c_CmdDriver_cmdRes_port_events[] = { "c_CmdResEvent", NULL };
static const char* c_CmdDriver_cmdReq_port_events[] = { "c_CmdPtrPkgEvent", NULL };
static const char* c_CmdDriver_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_CmdDriver_ports[] = {
		{"outCmdDrvReqQTokenChg", "link to c_CmdDriver for outgoing req txn token", c_CmdDriver_token_port_events},
		{"inTxnUnitReqPtr", "link to c_CmdDriver for incoming req cmds", c_CmdDriver_cmdReq_port_events},
		{"outCmdDrvResPtr", "link to c_CmdDriver for outgoing res txn", c_CmdDriver_cmdRes_port_events},
		{NULL, NULL, NULL}
};


/*----SETUP c_CmdUnit STRUCTURES----*/
static const ElementInfoParam c_CmdUnit_params[] = {
		{"numCmdReqQEntries", "Total number of entries in CmdUnit's Req queue", NULL},
		{"numCmdResQEntries", "Total number of entries in CmdUnit's Res queue", NULL},
		{"numChannelsPerDimm", "Total number of channels per DIMM", NULL},
		{"numRanksPerChannel", "Total number of ranks per channel", NULL},
		{"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
		{"numBanksPerBankGroup", "Total number of banks per group", NULL},
		{"numRowsPerBank" "Number of rows in every bank", NULL},
		{"numColsPerBank", "Number of cols in every bank", NULL},
		{"numBytesPerTransaction", "Number of bytes retrieved for every transaction", NULL},
		{"boolPrintCmdTrace", "Print a command trace", NULL},
		{"strCmdTraceFile", "Filename to print the command trace, or - for stdout", NULL},
		{"strAddressMapStr", "String describing the address map", NULL},
		{"relCommandWidth", "Relative width of each command", NULL},
		{"boolAllocateCmdResACT", "Allocate space in CmdUnit Res Q for ACT Cmds", NULL},
		{"boolAllocateCmdResREAD", "Allocate space in CmdUnit Res Q for READ Cmds", NULL},
		{"boolAllocateCmdResREADA", "Allocate space in CmdUnit Res Q for READA Cmds", NULL},
		{"boolAllocateCmdResWRITE", "Allocate space in CmdUnit Res Q for WRITE Cmds", NULL},
		{"boolAllocateCmdResWRITEA", "Allocate space in CmdUnit Res Q for WRITEA Cmds", NULL},
		{"boolAllocateCmdResPRE", "Allocate space in CmdUnit Res Q for PRE Cmds", NULL},
		{"boolCmdQueueFindAnyIssuable", "Search through Req Q for any cmd to send", NULL},
		{"bankPolicy", "Select which bank policy to model", NULL},
		{"boolUseRefresh", "Whether to use REF or not", NULL},
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

static const char* c_CmdUnit_cmdReq_port_events[] = { "c_CmdReqEvent", NULL };
static const char* c_CmdUnit_cmdRes_port_events[] = { "c_CmdResEvent", NULL };
static const char* c_CmdUnit_cmdReqPkg_port_events[] = { "c_CmdPtrPkgEvent", NULL };
static const char* c_CmdUnit_token_port_events[] = {"c_TokenChgEvent", NULL};

static const ElementInfoPort c_CmdUnit_ports[] = {
		{"inTxnUnitReqPtr", "link to c_CmdDriver for incoming req cmds", c_CmdUnit_cmdReqPkg_port_events},
		{"outTxnUnitReqQTokenChg", "link to c_CmdUnit for outgoing req cmd token", c_CmdUnit_token_port_events},
		{"outTxnUnitResPtr", "link to c_CmdUnit for outgoing res cmd", c_CmdUnit_cmdRes_port_events},
		{"outBankReqPtr", "link to c_CmdUnit for outgoing req cmd", c_CmdUnit_cmdReq_port_events},
		{"inBankResPtr", "link to c_CmdUnit for incoming res cmd", c_CmdUnit_cmdRes_port_events},
		{NULL, NULL, NULL}
};

/*----SETUP c_BankReceiver STRUCTURES----*/
static const ElementInfoParam c_BankReceiver_params[] = {
		{NULL, NULL, NULL } };

static const char* c_BankReceiver_cmdReq_port_events[] = { "c_CmdReqEvent", NULL };
static const char* c_BankReceiver_cmdRes_port_events[] = { "c_CmdResEvent", NULL };

static const ElementInfoPort c_BankReceiver_ports[] = {
		{"inCmdUnitReqPtr", "link to c_BankReceiver for incoming req cmds", c_BankReceiver_cmdReq_port_events},
		{"outCmdUnitResPtr", "link to c_BankReceiver for outgoing res cmds", c_BankReceiver_cmdRes_port_events},
		{NULL, NULL, NULL}
};

/*----SETUP c_BankReceiver STRUCTURES----*/
static const ElementInfoParam c_Dimm_params[] = {
		{"numRanksPerChannel", "Total number of ranks per channel", NULL},
		{"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
		{"numBanksPerBankGroup", "Total number of banks per group", NULL},
		{"boolAllocateCmdResACT", "Allocate space in CmdUnit Res Q for ACT Cmds", NULL},
		{"boolAllocateCmdResREAD", "Allocate space in CmdUnit Res Q for READ Cmds", NULL},
		{"boolAllocateCmdResREADA", "Allocate space in CmdUnit Res Q for READA Cmds", NULL},
		{"boolAllocateCmdResWRITE", "Allocate space in CmdUnit Res Q for WRITE Cmds", NULL},
		{"boolAllocateCmdResWRITEA", "Allocate space in CmdUnit Res Q for WRITEA Cmds", NULL},
		{"boolAllocateCmdResPRE", "Allocate space in CmdUnit Res Q for PRE Cmds", NULL},
		{NULL, NULL, NULL } };

static const char* c_Dimm_cmdReq_port_events[] = { "c_CmdReqEvent", NULL };
static const char* c_Dimm_cmdRes_port_events[] = { "c_CmdResEvent", NULL };

static const ElementInfoPort c_Dimm_ports[] = {
		{"inCmdUnitReqPtr", "link to c_Dimm for incoming req cmds", c_Dimm_cmdReq_port_events},
		{"outCmdUnitResPtr", "link to c_Dimm for outgoing res cmds", c_Dimm_cmdRes_port_events},
		{NULL, NULL, NULL}
};


static const ElementInfoComponent CramSimComponents[] = {
		{ "c_AddressHasher", 							// Name
		"Hashes addresses based on config parameters",			// Description
		NULL, 										// PrintHelp
		create_c_AddressHasher, 						// Allocator
		c_AddressHasher_params, 						// Parameters
		c_AddressHasher_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_TxnGenSeq", 							// Name
		"Test Txn Sequential Generator",			// Description
		NULL, 										// PrintHelp
		create_c_TxnGenSeq, 						// Allocator
		c_TxnGenSeq_params, 						// Parameters
		c_TxnGenSeq_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_TxnGenRand", 							// Name
		"Test Txn Random Generator",			 	// Description
		NULL, 										// PrintHelp
		create_c_TxnGenRand, 						// Allocator
		c_TxnGenRand_params, 						// Parameters
		c_TxnGenRand_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_TracefileReader", 							// Name
		"Test Trace file Generator",			 	// Description
		NULL, 										// PrintHelp
		create_c_TracefileReader, 						// Allocator
		c_TracefileReader_params, 						// Parameters
		c_TracefileReader_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_DramSimTraceReader", 							// Name
		"Test DRAMSim2 Trace file Generator",			 	// Description
		NULL, 										// PrintHelp
		create_c_DramSimTraceReader, 						// Allocator
		c_DramSimTraceReader_params, 						// Parameters
		c_DramSimTraceReader_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_USimmTraceReader", 							// Name
		"Test USimm Trace file Generator",			 	// Description
		NULL, 										// PrintHelp
		create_c_USimmTraceReader, 						// Allocator
		c_USimmTraceReader_params, 						// Parameters
		c_USimmTraceReader_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_TxnDriver", 							// Name
		"Test Txn Driver",						 	// Description
		NULL, 										// PrintHelp
		create_c_TxnDriver, 						// Allocator
		c_TxnDriver_params, 						// Parameters
		c_TxnDriver_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_TxnUnit",	 							// Name
		"Test Txn Unit",				 			// Description
		NULL, 										// PrintHelp
		create_c_TxnUnit, 							// Allocator
		c_TxnUnit_params, 							// Parameters
		c_TxnUnit_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_CmdDriver",	 						// Name
		"Test Cmd Driver",				 			// Description
		NULL, 										// PrintHelp
		create_c_CmdDriver, 						// Allocator
		c_CmdDriver_params, 						// Parameters
		c_CmdDriver_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_CmdUnit",	 							// Name
		"Test Cmd Unit",				 			// Description
		NULL, 										// PrintHelp
		create_c_CmdUnit, 							// Allocator
		c_CmdUnit_params, 							// Parameters
		c_CmdUnit_ports, 							// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_BankReceiver",	 						// Name
		"Test Bank Receiver",				 		// Description
		NULL, 										// PrintHelp
		create_c_BankReceiver, 						// Allocator
		c_BankReceiver_params, 						// Parameters
		c_BankReceiver_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_Dimm",			 						// Name
		"Test DIMM",				 				// Description
		NULL, 										// PrintHelp
		create_c_Dimm, 						// Allocator
		c_Dimm_params, 						// Parameters
		c_Dimm_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ "c_MemhBridge",			 						// Name
		"Bridge to communicate with MemoryHierarchy",				 				// Description
		NULL, 										// PrintHelp
		create_c_MemhBridge, 						// Allocator
		c_MemhBridge_params, 						// Parameters
		c_MemhBridge_ports, 						// Ports
		COMPONENT_CATEGORY_UNCATEGORIZED, 			// Category
		NULL 										// Statistics
		},
		{ NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL } };

extern "C" {
ElementLibraryInfo CramSim_eli = { "CramSim", // Name
		"Library with transaction generation components", // Description
		CramSimComponents, // Components
		NULL, // Events
		NULL, // Introspectors
		NULL, // Modules
		NULL, // Subcomponents
		NULL, // Partitioners
		NULL, // Python Module Generator
		NULL // Generators
		};
}
