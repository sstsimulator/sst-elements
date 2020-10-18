// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
#ifndef C_DIMM_HPP
#define C_DIMM_HPP

#include <vector>
#include <queue>
#include <stdlib.h>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
//#include "c_BankCommand.hpp"
#include "c_Bank.hpp"

namespace SST {
namespace CramSim {

  class c_BankCommand;

  // holds all the statistics pointers for the banks
  class c_BankStatistics {
  public:
    Statistic<uint64_t>* s_bankACTsRecvd;
    Statistic<uint64_t>* s_bankREADsRecvd;
    Statistic<uint64_t>* s_bankWRITEsRecvd;
    Statistic<uint64_t>* s_bankPREsRecvd;

    Statistic<uint64_t>* s_bankRowHits;
    Statistic<uint64_t>* s_totalRowHits;
  };

class c_Dimm: public SST::Component {
public:

    SST_ELI_REGISTER_COMPONENT(
        c_Dimm,
        "CramSim",
        "c_Dimm",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Test DIMM",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"numRanksPerChannel", "Total number of ranks per channel", NULL},
        {"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
        {"numBanksPerBankGroup", "Total number of banks per group", NULL},
        {"boolAllocateCmdResACT", "Allocate space in Controller Res Q for ACT Cmds", NULL},
        {"boolAllocateCmdResREAD", "Allocate space in Controller Res Q for READ Cmds", NULL},
        {"boolAllocateCmdResREADA", "Allocate space in Controller Res Q for READA Cmds", NULL},
        {"boolAllocateCmdResWRITE", "Allocate space in Controller Res Q for WRITE Cmds", NULL},
        {"boolAllocateCmdResWRITEA", "Allocate space in Controller Res Q for WRITEA Cmds", NULL},
        {"boolAllocateCmdResPRE", "Allocate space in Controller Res Q for PRE Cmds", NULL},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"ctrlLink", "link to controller", {"c_CmdReqEvent"} },
    )

    SST_ELI_DOCUMENT_STATISTICS(
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
    )

	c_Dimm(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_Dimm();

	void finish();
private:
	c_Dimm(); // for serialization only
	c_Dimm(const c_Dimm&); // do not implement
	void operator=(const c_Dimm&); // do not implement

	virtual bool clockTic(SST::Cycle_t); // called every cycle

	// BankReceiver <-> CmdUnit Handlers
	void handleInCmdUnitReqPtrEvent(SST::Event *ev); // receive a cmd req from CmdUnit
	void printQueues();

	void sendResponse();
	void sendToBank(c_BankCommand* x_bankCommandPtr);
	void updateDynamicEnergy(c_BankCommand* x_bankCommandPtr);
	void updateBackgroundEnergy();

	// Links
	SST::Link* m_ctrlLink;

	// Clock Handler
	Clock::HandlerBase *m_clockHandler;

	// params
	int k_numChannels;
	int k_numPChannelsPerChannel;
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int k_numDevices;

	bool k_boolPowerCalc;
	int k_IDD0;
	int k_IDD2P;
	int k_IDD2N;
	int k_IDD3N;
	int k_IDD4W;
	int k_IDD4R;
	int k_IDD5;
	int k_nRAS;
	int k_nRP;
	int k_nRFC;
	int k_nBL;
	float k_VDD;

	int m_numBanks;
	int m_numRanks;

        SimTime_t m_simCycle;
	std::vector<c_Bank*> m_banks;

	std::vector<c_BankCommand*> m_cmdResQ;

        Output* output;

	// Statistics
	Statistic<uint64_t>* s_actCmdsRecvd;
	Statistic<uint64_t>* s_readCmdsRecvd;
        Statistic<uint64_t>* s_readACmdsRecvd;
	Statistic<uint64_t>* s_writeCmdsRecvd;
	Statistic<uint64_t>* s_writeACmdsRecvd;
	Statistic<uint64_t>* s_preCmdsRecvd;
	Statistic<uint64_t>* s_refCmdsRecvd;

	std::vector<uint64_t> m_actCmdsRecvd;
	std::vector<uint64_t> m_readCmdsRecvd;
        std::vector<uint64_t> m_readACmdsRecvd;
	std::vector<uint64_t> m_writeCmdsRecvd;
	std::vector<uint64_t> m_writeACmdsRecvd;
	std::vector<uint64_t> m_preCmdsRecvd;
	std::vector<uint64_t> m_refCmdsRecvd;
	std::vector<double> m_backgroundEnergy;
	std::vector<double> m_readEnergy;
	std::vector<double> m_writeEnergy;
	std::vector<double> m_refreshEnergy;
	std::vector<double> m_actpreEnergy;

        std::vector<c_BankStatistics*> m_bankStatsVec;
};
}
}

#endif // C_DIMM_HPP
