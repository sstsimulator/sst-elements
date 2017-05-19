// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
using namespace SST::Statistics;

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

static const ElementInfoStatistic c_TxnGenRand_stats[] = {
  {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
  {NULL, NULL, NULL, 0}
};

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

static const ElementInfoStatistic c_DramSimTraceReader_stats[] = {
  {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
  {NULL, NULL, NULL, 0}
};

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

static const ElementInfoStatistic c_TxnUnit_stats[] = {
  {"readTxnsRecvd", "Number of read transactions received", "reads", 1}, // Name, Desc, Units, Enable Level
  {"writeTxnsRecvd", "Number of write transactions received", "writes", 1},
  {"totalTxnsRecvd", "Number of write transactions received", "transactions", 1},
  {"reqQueueSize", "Total size of the request queue over time", "transactions", 1},
  {"resQueueSize", "Total size of the response queue over time", "transactions", 1},
  {NULL, NULL, NULL, 0}
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

static const ElementInfoStatistic c_CmdUnit_stats[] = {
  //{"rowHits", "Number of DRAM page buffer hits", "hits", 1}, // Name, Desc, Units, Enable Level
  {NULL,NULL,NULL,0}
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

static const ElementInfoStatistic c_Dimm_stats[] = {
  {"actCmdsRecvd", "Number of activate commands received", "activates", 1}, // Name, Desc, Units, Enable Level
  {"readCmdsRecvd", "Number of read commands received", "reads", 1}, 
  {"readACmdsRecvd", "Number of read with autoprecharge commands received", "readAs", 1},
  {"writeCmdsRecvd", "Number of write commands received", "writes", 1}, 
  {"writeACmdsRecvd", "Number of write with autoprecharge commands received", "writeAs", 1},
  {"preCmdsRecvd", "Number of precharge commands received", "precharges", 1},
  {"refCmdsRecvd", "Number of refresh commands received", "refreshes", 1},
  {"totalRowHits", "Number of total row hits", "hits", 1},
  // begin autogenerated section
  {"bankACTsRecvd_0", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_0", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_0", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_0", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_0", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_1", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_1", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_1", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_1", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_1", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_2", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_2", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_2", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_2", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_2", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_3", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_3", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_3", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_3", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_3", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_4", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_4", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_4", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_4", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_4", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_5", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_5", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_5", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_5", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_5", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_6", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_6", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_6", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_6", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_6", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_7", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_7", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_7", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_7", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_7", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_8", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_8", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_8", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_8", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_8", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_9", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_9", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_9", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_9", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_9", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_10", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_10", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_10", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_10", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_10", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_11", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_11", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_11", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_11", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_11", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_12", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_12", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_12", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_12", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_12", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_13", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_13", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_13", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_13", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_13", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_14", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_14", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_14", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_14", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_14", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_15", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_15", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_15", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_15", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_15", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_16", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_16", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_16", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_16", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_16", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_17", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_17", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_17", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_17", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_17", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_18", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_18", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_18", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_18", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_18", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_19", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_19", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_19", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_19", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_19", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_20", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_20", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_20", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_20", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_20", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_21", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_21", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_21", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_21", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_21", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_22", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_22", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_22", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_22", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_22", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_23", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_23", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_23", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_23", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_23", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_24", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_24", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_24", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_24", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_24", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_25", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_25", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_25", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_25", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_25", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_26", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_26", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_26", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_26", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_26", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_27", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_27", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_27", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_27", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_27", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_28", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_28", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_28", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_28", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_28", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_29", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_29", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_29", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_29", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_29", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_30", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_30", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_30", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_30", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_30", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_31", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_31", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_31", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_31", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_31", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_32", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_32", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_32", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_32", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_32", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_33", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_33", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_33", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_33", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_33", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_34", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_34", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_34", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_34", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_34", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_35", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_35", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_35", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_35", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_35", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_36", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_36", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_36", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_36", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_36", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_37", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_37", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_37", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_37", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_37", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_38", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_38", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_38", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_38", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_38", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_39", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_39", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_39", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_39", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_39", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_40", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_40", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_40", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_40", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_40", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_41", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_41", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_41", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_41", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_41", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_42", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_42", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_42", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_42", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_42", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_43", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_43", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_43", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_43", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_43", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_44", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_44", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_44", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_44", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_44", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_45", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_45", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_45", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_45", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_45", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_46", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_46", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_46", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_46", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_46", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_47", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_47", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_47", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_47", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_47", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_48", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_48", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_48", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_48", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_48", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_49", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_49", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_49", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_49", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_49", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_50", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_50", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_50", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_50", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_50", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_51", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_51", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_51", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_51", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_51", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_52", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_52", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_52", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_52", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_52", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_53", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_53", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_53", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_53", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_53", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_54", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_54", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_54", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_54", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_54", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_55", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_55", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_55", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_55", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_55", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_56", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_56", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_56", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_56", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_56", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_57", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_57", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_57", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_57", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_57", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_58", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_58", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_58", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_58", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_58", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_59", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_59", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_59", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_59", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_59", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_60", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_60", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_60", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_60", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_60", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_61", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_61", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_61", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_61", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_61", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_62", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_62", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_62", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_62", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_62", "Number of row hits at bank x", "commands", 5},
  {"bankACTsRecvd_63", "Number of activate commands received at bank x", "commands", 5},
  {"bankREADsRecvd_63", "Number of read commands received at bank x", "commands", 5},
  {"bankWRITEsRecvd_63", "Number of write commands received at bank x", "commands", 5},
  {"bankPREsRecvd_63", "Number of write commands received at bank x", "commands", 5},
  {"bankRowHits_63", "Number of row hits at bank x", "commands", 5},
  {NULL, NULL, NULL, 0}
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
		c_TxnGenRand_stats								// Statistics
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
		c_DramSimTraceReader_stats							// Statistics
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
		c_TxnUnit_stats									// Statistics
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
		c_CmdUnit_stats									// Statistics
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
		c_Dimm_stats									// Statistics
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
