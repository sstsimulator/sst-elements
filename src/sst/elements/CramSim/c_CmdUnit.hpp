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
#ifndef C_CMDUNIT_HPP
#define C_CMDUNIT_HPP

#include <vector>
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

typedef unsigned long ulong;

namespace SST {
namespace n_Bank {

  class c_BankCommand;
  enum class e_BankCommandType;
  
class c_CmdUnit: public SST::Component {
public:

	c_CmdUnit(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_CmdUnit();

	void finish() {
		// printf("CmdUnit Queues:\n");
		// printf("\t CmdUnit Req Q:\n");
		// for (unsigned l_i = 0; l_i != k_cmdReqQEntries+1; ++l_i)
		// 	printf("%u\n", m_statsReqQ[l_i]);
		//
		// printf("\n\t CmdUnit Res Q:\n");
		// for (unsigned l_i = 0; l_i != k_cmdResQEntries+1; ++l_i)
		// 	printf("%u\n", m_statsResQ[l_i]);
		printf("Refresh's sent out: %" PRIu32 "\n", m_refsSent);

	}

	void print() const; // print internals

private:
	c_CmdUnit(); // for serialization only
	c_CmdUnit(const c_CmdUnit&); // do not implement
	void operator=(const c_CmdUnit&); // do not implement

	virtual bool clockTic(SST::Cycle_t); // called every cycle

	// CmdUnit <-> TxnUnit Handlers
	void handleInTxnUnitReqPtrEvent(SST::Event *ev); // receive a cmd req pkg from TxnUnit
	void handleOutTxnUnitReqQTokenChgEvent(SST::Event *ev); // we do not this function for functionality
	void handleOutTxnUnitResPtrEvent(SST::Event *ev); // we do not need this function for functionality

	// CmdUnit <-> Bank handlers
	void handleOutBankReqPtrEvent(SST::Event *ev); // we do not need this function for functionality
	void handleInBankResPtrEvent(SST::Event *ev); // receive a cmd res from Bank

	// CmdUnit <-> TxnUnit Links
	SST::Link* m_inTxnUnitReqPtrLink; // incoming txnunit req ptr
	SST::Link* m_outTxnUnitReqQTokenChgLink; // outgoing txnunit req q token
	SST::Link* m_outTxnUnitResPtrLink; // outgoing txnunit res ptr

	// CmdUnit <-> Bank Links
	SST::Link* m_outBankReqPtrLink; // outgoing bank req ptr
	SST::Link* m_inBankResPtrLink; // incoming bank res ptr

	void sendTokenChg(); // should happen at the end of every cycle
	void sendResponse();
	void sendRequest();
	void sendRefresh();

	std::set<unsigned> m_inflightWrites; // track inflight write commands
	std::vector<bool> m_blockBank;bool m_issuedDataCmd;

	void sendReqCloseBankPolicy(
			std::vector<c_BankCommand*>::iterator x_startItr); // send request function that models close bank policy
	void sendReqOpenRowPolicy(); // send request function that models open row policy
	void sendReqOpenBankPolicy(); // send request function that models open bank policy
	void sendReqPseudoOpenRowPolicy(); // send request function that models pseudo open row policy

	bool sendCommand(c_BankCommand* x_bankCommandPtr, c_BankInfo* x_bank); // helper method to sendRequest

	// FIXME: Remove. For testing purposes
	void printQueues();

	// params for internal arcitecture
	int k_cmdReqQEntries;
	int k_cmdResQEntries;

	// params for bank structure
	int k_numBytesPerTransaction;
	int k_numChannelsPerDimm;
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int k_numColsPerBank;
	int k_numRowsPerBank;

	int m_numBankGroups;
	int m_numBanks;

	bool k_allocateCmdResQACT;bool k_allocateCmdResQREAD;bool k_allocateCmdResQREADA;bool k_allocateCmdResQWRITE;bool k_allocateCmdResQWRITEA;bool k_allocateCmdResQPRE;

	bool k_useRefresh;bool k_cmdQueueFindAnyIssuable;
	int k_bankPolicy;

	int k_relCommandWidth;
	int m_cmdResQTokens;

	std::vector<c_BankInfo*> m_banks;
	std::vector<c_BankGroup*> m_bankGroups;
	std::vector<c_Rank*> m_ranks;
	std::vector<c_Channel*> m_channel;

        bool k_printCmdTrace;
        std::string k_cmdTraceFileName;
        std::filebuf m_cmdTraceFileBuf;
        std::streambuf *m_cmdTraceStreamBuf;
        std::ofstream m_cmdTraceOFStream;
        std::ostream *m_cmdTraceStream;

	// token change in this unit this cycle
	// beginning of every cycle these variables are reset
	// sendTokenChg() function sends the contents of these variables
	int m_thisCycleReqQTknChg;
	int m_thisCycleResReceived;

	std::vector<c_BankCommand*> m_cmdReqQ;
	std::vector<c_BankCommand*> m_cmdResQ;

	std::map<std::string, unsigned> m_bankParams;

	// FIXME: Delete. Used for debugging queue size issues
	unsigned* m_statsReqQ;
	unsigned* m_statsResQ;
	unsigned m_refsSent;

	SimTime_t m_lastDataCmdIssueCycle;
        e_BankCommandType m_lastDataCmdType;
	std::list<unsigned> m_cmdACTFAWtracker; // FIXME: change this to a circular buffer for speed. Could also implement as shift register.
	bool m_issuedACT;

  
  // Statistics
  //  Statistic<uint64_t>* s_rowHits;

};
}
}

#endif // C_CMDUNIT_HPP
