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
#ifndef C_CmdUnit_HPP
#define C_CmdUnit_HPP

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
#include "c_DeviceController.hpp"
#include "c_Controller.hpp"
#include "c_CtrlSubComponent.hpp"


typedef unsigned long ulong;

namespace SST {
namespace n_Bank{

	class c_BankCommand;
	class c_Controller;
	enum class e_BankCommandType;
  
class c_CmdUnit: public c_CtrlSubComponent <c_BankCommand*,c_BankCommand*> {
public:
	c_CmdUnit(Component *comp, Params& x_params);
	~c_CmdUnit();

	void finish() {
		printf("Refresh's sent out: %" PRIu32 "\n", m_refsSent);
	}

	void print() const; // print internals
	bool clockTic(SST::Cycle_t);

private:

    void run();

	std::vector<c_BankCommand*> m_cmdReqQ;			//input queue
	std::vector<c_BankCommand*> m_cmdResQ;

	c_Controller *m_Owner;

	// params for internal architecture
	int k_cmdReqQEntries;
	int k_cmdResQEntries;

	c_CmdUnit(); // for serialization only

	void sendRequest();
	void sendRefresh();
	void issueCommand(c_BankCommand* cmd);

	std::set<unsigned> m_inflightWrites; // track inflight write commands
	//std::vector<bool> m_blockBank;bool m_issuedDataCmd;
	std::deque<unsigned> m_blockRowCmd; //command bus occupancy info
	std::deque<unsigned> m_blockColCmd; //command bus occupancy info

	void sendReqCloseBankPolicy(
			std::deque<c_BankCommand*>::iterator x_startItr); // send request function that models close bank policy
	void sendReqOpenRowPolicy(); // send request function that models open row policy
	void sendReqOpenBankPolicy(); // send request function that models open bank policy
	void sendReqPseudoOpenRowPolicy(); // send request function that models pseudo open row policy

	bool sendCommand(c_BankCommand* x_bankCommandPtr, c_BankInfo* x_bank); // helper method to sendRequest

	/// helper methods to check if channel (command bus) is available
	bool isCommandBusAvailable(c_BankCommand* x_BankCommandPtr);
	///Set the occupancy of command bus
	bool occupyCommandBus(c_BankCommand *l_cmdPtr);
	///Release the occupancy of command bus
	void releaseCommandBus();

	// FIXME: Remove. For testing purposes
	void printQueues();



	// params for bank structure
	int k_numBytesPerTransaction;
	int k_numChannelsPerDimm;
	int k_numPseudoChannels;
	int k_numRanksPerChannel;
	int k_numBankGroupsPerRank;
	int k_numBanksPerBankGroup;
	int k_numColsPerBank;
	int k_numRowsPerBank;

	int m_numRanks;
	int m_numBankGroups;
	int m_numBanks;

	bool k_allocateCmdResQACT;bool k_allocateCmdResQREAD;bool k_allocateCmdResQREADA;bool k_allocateCmdResQWRITE;bool k_allocateCmdResQWRITEA;bool k_allocateCmdResQPRE;

	bool k_useRefresh;
	bool k_cmdQueueFindAnyIssuable;
	int k_bankPolicy;
	bool k_IsHBM;

	int k_relCommandWidth;
	int m_cmdResQTokens;

	int k_useDualCommandBus;
	int k_multiCycleACT;
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



	std::map<std::string, unsigned> m_bankParams;

	// FIXME: Delete. Used for debugging queue size issues
	unsigned* m_statsReqQ;
	unsigned* m_statsResQ;
	unsigned m_refsSent;

	SimTime_t m_lastDataCmdIssueCycle;
	e_BankCommandType m_lastDataCmdType;
	unsigned m_lastPseudoChannel;
	std::list<unsigned> m_cmdACTFAWtracker; // FIXME: change this to a circular buffer for speed. Could also implement as shift register.
	bool m_issuedACT;


};
}
}

#endif // C_CMDUNIT_HPP
