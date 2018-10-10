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

// Copyright 2015 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef C_DeviceDriver_HPP
#define C_DeviceDriver_HPP

#include <vector>
#include <queue>
#include <list>
#include <set>
#include <iostream>
#include <fstream>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_BankInfo.hpp"
#include "c_BankGroup.hpp"
#include "c_Channel.hpp"
#include "c_Rank.hpp"
#include "c_BankCommand.hpp"
#include "c_DeviceDriver.hpp"
#include "c_Controller.hpp"


typedef unsigned long ulong;

namespace SST {
namespace n_Bank{

	class c_Controller;
	enum class e_BankCommandType;
  
class c_DeviceDriver: public SubComponent{
public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        c_DeviceDriver,
        "CramSim",
        "c_DeviceDriver",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Dram Control Unit",
        "SST::CramSim::Controller::DeviceDriver"
    )

    SST_ELI_DOCUMENT_PARAMS(
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
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

	c_DeviceDriver(Component *comp, Params& x_params);
	~c_DeviceDriver();

	virtual void run();
	virtual bool push(c_BankCommand* x_cmd);
	virtual bool isCmdAllowed(c_BankCommand* x_bankCommandPtr);
	virtual c_BankInfo* getBankInfo(unsigned x_bankId);
	void update();

	unsigned getNumChannel(){return k_numChannels;}
	unsigned getNumPChPerChannel(){return k_numPChannelsPerChannel;}
	unsigned getNumRanksPerChannel(){return k_numRanksPerChannel;}
	unsigned getNumBankGroupsPerRank(){return k_numBankGroupsPerRank;}
	unsigned getNumBanksPerBankGroup(){return k_numBanksPerBankGroup;}
	unsigned getNumRowsPerBank(){return k_numRowsPerBank;}
	unsigned getNumColPerBank(){return k_numColsPerBank;}
	unsigned getTotalNumBank() {return m_numBanks;}

private:

	c_DeviceDriver(); // for serialization only
	bool sendRefresh(unsigned rank);

	void sendRequest(); // send request function that models close bank policy
	bool sendCommand(c_BankCommand* x_bankCommandPtr, c_BankInfo* x_bank); // helper method to sendRequest

	/// helper methods to check if channel (command bus) is available
	bool isCommandBusAvailable(c_BankCommand* x_BankCommandPtr);
	///Set the occupancy of command bus
	bool occupyCommandBus(c_BankCommand *x_cmdPtr);
	///Release the occupancy of command bus
	void releaseCommandBus();

	void initACTFAWTracker();
	void initRefresh();
	unsigned getNumIssuedACTinFAW(unsigned x_rankid);
	void createRefreshCmds(unsigned x_rank);
    bool isRefreshing(const c_HashedAddress *x_addr);

	c_Controller *m_Owner;

	std::deque<c_BankCommand*> m_inputQ;
	std::deque<c_BankCommand*> m_outputQ;
	std::vector<bool> m_blockBank;
	std::set<unsigned> m_inflightWrites; // track inflight write commands
	std::deque<unsigned> m_blockRowCmd; //command bus occupancy info
	std::deque<unsigned> m_blockColCmd; //command bus occupancy info

	std::vector<unsigned> m_currentREFICount; //per rank REFICounter
	std::vector<std::vector<c_BankCommand*>> m_refreshCmdQ; //per rank refresh commandQ
	std::vector<unsigned> m_nextBankToRefresh; //for per-bank refresh

	SimTime_t m_lastDataCmdIssueCycle;
	e_BankCommandType m_lastDataCmdType;
	unsigned m_lastChannel;
	unsigned m_lastPseudoChannel;
	std::vector<std::list<unsigned>> m_cmdACTFAWtrackers; // FIXME: change this to a circular buffer for speed. Could also implement as shift register.
	std::vector<bool> m_isACTIssued;
	bool m_issuedACT;


	// params
	int k_numChannels;
	int k_numPChannelsPerChannel;
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int k_numColsPerBank;
	int k_numRowsPerBank;

	bool k_useDualCommandBus;
	bool k_multiCycleACT;
	bool k_useRefresh;
	bool k_useSBRefresh;

	std::vector<c_BankInfo*> m_banks;
	std::vector<c_BankGroup*> m_bankGroups;
	std::vector<c_Rank*> m_ranks;
	std::vector<c_Channel*> m_channel;
    std::map<std::string, unsigned> m_bankParams;


	int m_numChannels;
	int m_numPseudoChannels;
	int m_numRanks;
	int m_numBankGroups;
	int m_numBanks;

	bool k_printCmdTrace;
	std::string k_cmdTraceFileName;
	std::filebuf m_cmdTraceFileBuf;
	std::streambuf *m_cmdTraceStreamBuf;
	std::ofstream m_cmdTraceOFStream;
	std::ostream *m_cmdTraceStream;

	//debug
	Output *output;
	uint64_t m_issued_cmd;
};
}
}

#endif // C_DeviceDriver_HPP
