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

/*
 * c_DeviceDriver.cpp
 *
 *  Created on: June 22, 2016
 *      Author: mcohen
 */

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
// C++ includes
//SST includes
#include "sst_config.h"

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <assert.h>

// CramSim includes
#include "c_DeviceDriver.hpp"
#include "c_Transaction.hpp"
#include "c_AddressHasher.hpp"
#include "c_HashedAddress.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdPtrPkgEvent.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_BankState.hpp"
#include "c_BankStateIdle.hpp"
#include "c_BankCommand.hpp"


using namespace SST;
using namespace SST::n_Bank;



/*!
 *
 * @param owner
 * @param params
 */
c_DeviceDriver::c_DeviceDriver(Component *owner, Params& params) : SubComponent(owner) {

	m_Owner = dynamic_cast<c_Controller *>(owner);
    output = m_Owner->getOutput();

	// read params here
	bool l_found = false;


	k_useDualCommandBus = (uint32_t) params.find<uint32_t>("boolDualCommandBus", 0, l_found);
	if (!l_found) {
		std::cout << "boolDualCommandBus value is missing... disabled" << std::endl;
	}

	k_multiCycleACT = (uint32_t) params.find<uint32_t>("boolMultiCycleACT", 0, l_found);
	if (!l_found) {
		std::cout << "boolMultiCycleACT value is missing... disabled" << std::endl;
	}

	//set system configuration
	k_numChannels = (uint32_t) params.find<uint32_t>("numChannels", 1, l_found);
	if (!l_found) {
		std::cout << "numChannels value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numPChannelsPerChannel= (uint32_t) params.find<uint32_t>("numPChannelsPerChannel", 1, l_found);
	if (!l_found) {
		std::cout << "numPChannelsPerChannel value is missing... disabled" << std::endl;
		//exit(-1);
	}

	k_numRanksPerChannel = (uint32_t) params.find<uint32_t>("numRanksPerChannel", 2, l_found);
	if (!l_found) {
		std::cout << "numRanksPerChannel value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numBankGroupsPerRank = (uint32_t) params.find<uint32_t>("numBankGroupsPerRank", 100, l_found);
	if (!l_found) {
		std::cout << "numBankGroupsPerRank value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numBanksPerBankGroup = (uint32_t) params.find<uint32_t>("numBanksPerBankGroup", 100, l_found);
	if (!l_found) {
		std::cout << "numBanksPerBankGroup value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numColsPerBank = (uint32_t) params.find<uint32_t>("numColsPerBank", 100, l_found);
	if (!l_found) {
		std::cout << "numColsPerBank value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_numRowsPerBank = (uint32_t) params.find<uint32_t>("numRowsPerBank", 100, l_found);
	if (!l_found) {
		std::cout << "numRowsPerBank value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_useRefresh = (uint32_t) params.find<uint32_t>("boolUseRefresh", 0, l_found);
	if (!l_found) {
		std::cout << "boolUseRefresh param value is missing... disabled" << std::endl;
	}

	k_useSBRefresh = (uint32_t) params.find<uint32_t>("boolUseSBRefresh", 0, l_found);
	if (!l_found) {
		std::cout << "boolUseSBRefresh (single bank refresh) param value is missing... disabled" << std::endl;
	}

	/* Device timing parameters*/
    //FIXME: Move this param reading to inside of c_BankInfo
    m_bankParams["nRC"] = (uint32_t) params.find<uint32_t>("nRC", 55, l_found);
    if (!l_found) {
        std::cout << "nRC value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRRD"] = (uint32_t) params.find<uint32_t>("nRRD", 4, l_found);
    if (!l_found) {
        std::cout << "nRRD value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRRD_L"] = (uint32_t) params.find<uint32_t>("nRRD_L", 6, l_found);
    if (!l_found) {
        std::cout << "nRRD_L value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRRD_S"] = (uint32_t) params.find<uint32_t>("nRRD_S", 4, l_found);
    if (!l_found) {
        std::cout << "nRRD_S value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRCD"] = (uint32_t) params.find<uint32_t>("nRCD", 16, l_found);
    if (!l_found) {
        std::cout << "nRRD_L value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCCD"] = (uint32_t) params.find<uint32_t>("nCCD", 4, l_found);
    if (!l_found) {
        std::cout << "nCCD value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCCD_L"] = (uint32_t) params.find<uint32_t>("nCCD_L", 5, l_found);
    if (!l_found) {
        std::cout << "nCCD_L value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCCD_L_WR"] = (uint32_t) params.find<uint32_t>("nCCD_L_WR", 1, l_found);
    if (!l_found) {
        std::cout << "nCCD_L_WR value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCCD_S"] = (uint32_t) params.find<uint32_t>("nCCD_S", 4, l_found);
    if (!l_found) {
        std::cout << "nCCD_S value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nAL"] = (uint32_t) params.find<uint32_t>("nAL", 15, l_found);
    if (!l_found) {
        std::cout << "nAL value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCL"] = (uint32_t) params.find<uint32_t>("nCL", 16, l_found);
    if (!l_found) {
        std::cout << "nCL value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nCWL"] = (uint32_t) params.find<uint32_t>("nCWL", 16, l_found);
    if (!l_found) {
        std::cout << "nCWL value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nWR"] = (uint32_t) params.find<uint32_t>("nWR", 16, l_found);
    if (!l_found) {
        std::cout << "nWR value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nWTR"] = (uint32_t) params.find<uint32_t>("nWTR", 3, l_found);
    if (!l_found) {
        std::cout << "nWTR value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nWTR_L"] = (uint32_t) params.find<uint32_t>("nWTR_L", 9, l_found);
    if (!l_found) {
        std::cout << "nWTR_L value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nWTR_S"] = (uint32_t) params.find<uint32_t>("nWTR_S", 3, l_found);
    if (!l_found) {
        std::cout << "nWTR_S value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRTW"] = (uint32_t) params.find<uint32_t>("nRTW", 4, l_found);
    if (!l_found) {
        std::cout << "nRTW value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nEWTR"] = (uint32_t) params.find<uint32_t>("nEWTR", 6, l_found);
    if (!l_found) {
        std::cout << "nEWTR value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nERTW"] = (uint32_t) params.find<uint32_t>("nERTW", 6, l_found);
    if (!l_found) {
        std::cout << "nERTW value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nEWTW"] = (uint32_t) params.find<uint32_t>("nEWTW", 6, l_found);
    if (!l_found) {
        std::cout << "nEWTW value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nERTR"] = (uint32_t) params.find<uint32_t>("nERTR", 6, l_found);
    if (!l_found) {
        std::cout << "nERTR value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRAS"] = (uint32_t) params.find<uint32_t>("nRAS", 39, l_found);
    if (!l_found) {
        std::cout << "nRAS value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRTP"] = (uint32_t) params.find<uint32_t>("nRTP", 9, l_found);
    if (!l_found) {
        std::cout << "nRTP value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRP"] = (uint32_t) params.find<uint32_t>("nRP", 16, l_found);
    if (!l_found) {
        std::cout << "nRP value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nRFC"] = (uint32_t) params.find<uint32_t>("nRFC", 420, l_found);
    if (!l_found) {
        std::cout << "nRFC value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nREFI"] = (uint32_t) params.find<uint32_t>("nREFI", 9360, l_found);
    if (!l_found) {
        std::cout << "nREFI value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nFAW"] = (uint32_t) params.find<uint32_t>("nFAW", 16, l_found);
    if (!l_found) {
        std::cout << "nFAW value is missing ... exiting" << std::endl;
        exit(-1);
    }

    m_bankParams["nBL"] = (uint32_t) params.find<uint32_t>("nBL", 4, l_found);
    if (!l_found) {
        std::cout << "nBL value is missing ... exiting" << std::endl;
        exit(-1);
    }



	// configure the memory hierarchy
	m_numChannels =  k_numChannels;
	m_numPseudoChannels = m_numChannels * k_numPChannelsPerChannel;
	m_numRanks = m_numPseudoChannels * k_numRanksPerChannel;
	m_numBankGroups = m_numRanks * k_numBankGroupsPerRank;
	m_numBanks = m_numBankGroups * k_numBanksPerBankGroup;

	for (int l_i = 0; l_i != m_numRanks; ++l_i) {
		c_Rank *l_entry = new c_Rank(&m_bankParams);
		m_ranks.push_back(l_entry);
	}

	for (int l_i = 0; l_i != m_numBankGroups; ++l_i) {
		c_BankGroup *l_entry = new c_BankGroup(&m_bankParams, l_i);
		m_bankGroups.push_back(l_entry);
	}

	for (int l_i = 0; l_i != m_numBanks; ++l_i) {
		c_BankInfo *l_entry = new c_BankInfo(&m_bankParams, l_i); // auto init state to IDLE
		m_banks.push_back(l_entry);
	}


	// connect the hierarchy
	unsigned l_rankNum = 0;
	for (unsigned l_i = 0; l_i != m_numPseudoChannels; ++l_i) {
		c_Channel *l_channel = new c_Channel(&m_bankParams, l_i);
		m_channel.push_back(l_channel);
		//   int l_i = 0;
		for (unsigned l_j = 0; l_j != k_numRanksPerChannel; ++l_j) {
			//std::cout << "Attaching Channel" << l_i << " to Rank" << l_rankNum << std::endl;
			m_channel.at(l_i)->acceptRank(m_ranks.at(l_rankNum));
			m_ranks.at(l_rankNum)->acceptChannel(m_channel.at(l_i));
			++l_rankNum;
		}
	}
	assert(l_rankNum == m_numRanks);

	unsigned l_bankGroupNum = 0;
	for (unsigned l_i = 0; l_i != m_numRanks; ++l_i) {
		for (unsigned l_j = 0; l_j != k_numBankGroupsPerRank; ++l_j) {
			//std::cout << "Attaching Rank" << l_i <<
			//	  " to BankGroup" << l_bankGroupNum << std::endl;
			m_ranks.at(l_i)->acceptBankGroup(m_bankGroups.at(l_bankGroupNum));
			m_bankGroups.at(l_bankGroupNum)->acceptRank(m_ranks.at(l_i));
			++l_bankGroupNum;
		}
	}
	assert(l_bankGroupNum == m_numBankGroups);

	unsigned l_bankNum = 0;
	for (int l_i = 0; l_i != m_numBankGroups; ++l_i) {
		for (int l_j = 0; l_j != k_numBanksPerBankGroup; ++l_j) {
			//	std::cout << "Attaching BankGroup" << l_i <<
			//	  " to Bank" << l_bankNum << std::endl;
			m_bankGroups.at(l_i)->acceptBank(m_banks.at(l_bankNum));
			m_banks.at(l_bankNum)->acceptBankGroup(m_bankGroups.at(l_i));
			l_bankNum++;
		}
	}
	assert(l_bankNum == m_numBanks);

	// reset last data cmd issue cycle
	m_lastDataCmdIssueCycle = 0;
	m_lastDataCmdType = e_BankCommandType::READ;
	m_lastPseudoChannel = 0;
	m_lastChannel=0;

	// reset command bus
	m_blockColCmd.resize(k_numChannels);
	m_blockRowCmd.resize(k_numChannels);

	//init per-rank FAW tracker
	initACTFAWTracker();

	//init structures for refresh
	initRefresh();



	// set up cmd trace output
	k_printCmdTrace = (uint32_t) params.find<uint32_t>("boolPrintCmdTrace", 0, l_found);

	k_cmdTraceFileName = (std::string) params.find<std::string>("strCmdTraceFile", "-", l_found);
	//k_cmdTraceFileName.pop_back(); // remove trailing newline (??)
	if (k_printCmdTrace) {
		if (k_cmdTraceFileName.compare("-") == 0) {// set output to std::cout
			std::cout << "Setting cmd trace output to std::cout" << std::endl;
			m_cmdTraceStreamBuf = std::cout.rdbuf();
		} else { // open the file and direct the cmdTraceStream to it
			std::cout << "Setting cmd trace output to " << k_cmdTraceFileName << std::endl;
			m_cmdTraceOFStream.open(k_cmdTraceFileName);
			if (m_cmdTraceOFStream) {
				m_cmdTraceStreamBuf = m_cmdTraceOFStream.rdbuf();
			} else {
				std::cerr << "Failed to open cmd trace output file " << k_cmdTraceFileName << ", redirecting to stdout";
				m_cmdTraceStreamBuf = std::cout.rdbuf();
			}
		}
		m_cmdTraceStream = new std::ostream(m_cmdTraceStreamBuf);
	}

	m_inflightWrites.clear();
	m_blockBank.clear();
	m_blockBank.resize(m_numBanks, false);
	releaseCommandBus();  //update the command bus status
	m_isACTIssued.clear();
	m_isACTIssued.resize(m_numRanks, false);
}

/*!
 *
 */
c_DeviceDriver::~c_DeviceDriver() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	// for (unsigned l_i = 0; l_i != m_numBanks; ++l_i)
	// 	delete m_banks.at(l_i);
	//
	// for (unsigned l_i = 0; l_i != k_numBankGroups; ++l_i)
	// 	delete m_bankGroups.at(l_i);
}


/*!
 *
 */
void c_DeviceDriver::run() {
	uint64_t l_issued_cmd=m_issued_cmd;
	releaseCommandBus();

	if(k_useRefresh) {
		//refresh counter is managed per rank..
		//Todo: For HBM, refresh counter needs to be managed per channel
		for (unsigned l_id = 0; l_id < m_numRanks; l_id++) {
			if (m_currentREFICount[l_id] > 0) {
				--m_currentREFICount[l_id];
			} else {
				createRefreshCmds(l_id);
				m_currentREFICount[l_id] = m_bankParams.at("nREFI");
			}

			if (!m_refreshCmdQ[l_id].empty())
				sendRefresh(l_id);
		}
	}

	if (!m_inputQ.empty())
		sendRequest();

	//send command to c_dimm if the command is ready
	for (auto l_cmdPtrItr = m_outputQ.begin(); l_cmdPtrItr != m_outputQ.end();)  {
		c_BankCommand* l_cmdPtr = (*l_cmdPtrItr);


		if(l_cmdPtr->isResponseReady()) {
			//send only one command for all bank refresh and precharge
			if(l_cmdPtr->getBankIdVec().size()>0)
			{
				if(l_cmdPtr->getSeqNum()==l_cmdPtr->getBankIdVec().front())
				{
					m_Owner->sendCommand(l_cmdPtr);
				} else
					delete l_cmdPtr;
			}
			else
				m_Owner->sendCommand(l_cmdPtr);

			l_cmdPtrItr=m_outputQ.erase(l_cmdPtrItr);
		}
		else
			l_cmdPtrItr++;
	}

}

/*!
 *
 */
void c_DeviceDriver::update() {
	for (int l_i = 0; l_i != m_banks.size(); ++l_i) {

		m_banks.at(l_i)->clockTic(m_Owner->getSimCycle());
		// m_banks.at(l_i)->printState();
	}
	//update ACTFAWTracker info
	for (int l_rankNum = 0; l_rankNum < m_numRanks; l_rankNum++) {
		m_cmdACTFAWtrackers[l_rankNum].push_back(
				m_isACTIssued[l_rankNum] ? static_cast<unsigned>(1) : static_cast<unsigned>(0));
		m_cmdACTFAWtrackers[l_rankNum].pop_front();
	}

	// do the member var setup up before calling any req sending policy function
	if (m_inflightWrites.size() > 0)
		m_inflightWrites.clear();

	m_blockBank.clear();
	m_blockBank.resize(m_numBanks, false);
	releaseCommandBus();  //update the command bus status
	m_isACTIssued.clear();
	m_isACTIssued.resize(m_numRanks, false);
}




/*!
 * check bank and bus status
 * @param x_bankCommandPtr
 * @return
 */
bool c_DeviceDriver::isCmdAllowed(c_BankCommand* x_bankCommandPtr)
{
	SimTime_t l_time = m_Owner->getSimCycle();
	// get count of ACT cmds issued in the FAW
	unsigned l_bankId=x_bankCommandPtr->getHashedAddress()->getBankId();

	if(k_useRefresh)
		if(isRefreshing(x_bankCommandPtr->getHashedAddress()))
			return false;

	//insert active command if the target row is closed (this case can happen due to refresh)
	if(x_bankCommandPtr->isColCommand() &&
			(m_banks.at(l_bankId)->getCurrentState()==e_BankState::PRE || m_banks.at(l_bankId)->getCurrentState()==e_BankState::IDLE))
	{
		//check if ACT command is already inserted to inputQ
		for(auto &l_cmd: m_inputQ) {
			if((l_cmd->getBankId()==x_bankCommandPtr->getBankId())&&(l_cmd->getCommandMnemonic()==e_BankCommandType ::ACT))
				return false;
		}

		//if there is no ACT command for the closed bank, insert an ACT command.
        ulong l_addr = x_bankCommandPtr->getAddress();
        const c_HashedAddress *l_hashedAddr = x_bankCommandPtr->getHashedAddress();
        c_BankCommand *l_newCmd = new c_BankCommand(0, e_BankCommandType::ACT, l_addr, *l_hashedAddr);
        m_inputQ.push_back(l_newCmd);

		return false;
	}

	//check tFAW
    //todo: check tFAW per each rank
	//	if ((e_BankCommandType::ACT == ((x_bankCommandPtr))->getCommandMnemonic()) && (l_cmdACTIssuedInFAW >= 4))
	//		return false;

    //check inflight Write commands to the same address
    // block: READ after WRITE to the same address
    // block: WRITE after WRITE to the same address. Processor should make sure that the older WRITE is annulled but we will block the younger here.
    //todo: remove?
	//if (m_inflightWrites.find((x_bankCommandPtr)->getAddress())
	//	!= m_inflightWrites.end()) {
	//	return false;
	//}
	if(!isCommandBusAvailable(x_bankCommandPtr))
		return false;

    //check timing constraints
	if (!m_banks.at(l_bankId)->isCommandAllowed(x_bankCommandPtr, l_time))
		return false;

	return true;
}


/*!
 *
 * @param x_bankId
 * @return
 */
c_BankInfo* c_DeviceDriver::getBankInfo(unsigned x_bankId)
{
	return m_banks.at(x_bankId);
}



/*!
 *
 */
void c_DeviceDriver::sendRequest() {

	// get count of ACT cmds issued in the FAW
	std::vector<unsigned> l_numACTIssuedInFAW;
	l_numACTIssuedInFAW.clear();
	for(int l_rankId=0;l_rankId<m_numRanks;l_rankId++)
	{
		l_numACTIssuedInFAW.push_back(getNumIssuedACTinFAW(l_rankId));
	}


	for (auto l_cmdPtrItr = m_inputQ.begin(); l_cmdPtrItr != m_inputQ.end();)  {

		bool l_proceed = true;
		c_BankCommand* l_cmdPtr = (*l_cmdPtrItr);
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		unsigned l_rankNum = l_cmdPtr->getHashedAddress()->getRankId();


		if(k_useRefresh)
			if(isRefreshing(l_cmdPtr->getHashedAddress())==true)
            {
                l_cmdPtrItr++;
                continue;
            }

		//requests to same bank should be executed sequentially.
		if(!m_blockBank.at(l_bankNum)) {
			l_proceed = true;
			m_blockBank.at(l_bankNum) = true;
		}
		else {
			l_proceed = false;
		}

		if ((l_cmdPtr)->getCommandMnemonic() == e_BankCommandType::REF)
			break;

		if ((e_BankCommandType::ACT == ((l_cmdPtr))->getCommandMnemonic()) && (l_numACTIssuedInFAW[l_rankNum] >= 4))
		{
			l_proceed = false;
		}

		// block: READ after WRITE to the same address
		// block: WRITE after WRITE to the same address. Processor should make sure that the older WRITE is annulled but we will block the younger here.
		if (m_inflightWrites.find((l_cmdPtr)->getAddress())
			!= m_inflightWrites.end()) {
			l_proceed = false;
		}

		// if this is a WRITE or WRITEA command insert it in the inflight set
		if ((e_BankCommandType::WRITE == (l_cmdPtr)->getCommandMnemonic())
			|| (e_BankCommandType::WRITEA
				== (l_cmdPtr)->getCommandMnemonic())) {

			m_inflightWrites.insert((l_cmdPtr)->getAddress());
		}



		if (l_proceed) {
			c_BankInfo *l_bank = m_banks.at(l_bankNum);

			if ( isCommandBusAvailable(l_cmdPtr)){   //check if the command bus is available
				//skip a precharge command if the bank is already precharged
				if(l_cmdPtr->getCommandMnemonic() == e_BankCommandType::PRE)
                    if(l_bank->getCurrentState()==e_BankState::PRE || l_bank->getCurrentState()==e_BankState::IDLE)
                    {
						l_cmdPtrItr=m_inputQ.erase(l_cmdPtrItr);
						continue;
                    }

				//send command
				if (sendCommand((l_cmdPtr), l_bank)) {
					m_issued_cmd++;

					// remove cmd from ReqQ
					l_cmdPtrItr=m_inputQ.erase(l_cmdPtrItr);

					if (l_cmdPtr->isColCommand()) {
						assert((m_lastDataCmdType != ((l_cmdPtr))->getCommandMnemonic()) ||
							   (m_lastPseudoChannel != (l_cmdPtr->getHashedAddress()->getPChannel())) ||
							   (m_lastChannel !=(l_cmdPtr->getHashedAddress()->getChannel())) ||
							   (m_Owner->getSimCycle() - m_lastDataCmdIssueCycle) >=
							   (std::min(m_bankParams.at("nBL"),
										 std::max(m_bankParams.at("nCCD_L"), m_bankParams.at("nCCD_S")))));

						m_lastChannel = ((l_cmdPtr))->getHashedAddress()->getChannel();
						m_lastDataCmdIssueCycle = m_Owner->getSimCycle();
						m_lastDataCmdType = ((l_cmdPtr))->getCommandMnemonic();
						m_lastPseudoChannel = ((l_cmdPtr))->getHashedAddress()->getPChannel();
					}

					bool l_isACT= (e_BankCommandType::ACT == ((l_cmdPtr))->getCommandMnemonic());
					if(l_isACT)
					{
						assert(m_isACTIssued[l_rankNum]==false);
						m_isACTIssued[l_rankNum] = true;
					}

					if (occupyCommandBus(l_cmdPtr))
						break;// all command buses are occupied, so stop

					continue;
				}
			}
		}
		++l_cmdPtrItr;
	}// for
}


/*!
 *
 * @param rank
 * @return
 */
bool c_DeviceDriver::sendRefresh(unsigned x_requester) {


	std::vector<c_BankCommand*> &cmdQ=m_refreshCmdQ[x_requester];

	c_BankCommand* l_cmdPtr = cmdQ.front();
    SimTime_t l_time = m_Owner->getSimCycle();
	std::vector<unsigned>& l_bankIdVec = l_cmdPtr->getBankIdVec();

	//check if the target banks are ready for the current command
	if(l_bankIdVec.size()>0) {
		for (auto &l_bankid:l_bankIdVec) {
			c_BankInfo *l_bank = m_banks.at(l_bankid);
			if (!l_bank->isCommandAllowed(l_cmdPtr, l_time))
				return false;
		}
	} else{
		c_BankInfo *l_bank = m_banks.at(l_cmdPtr->getBankId());
		if (!l_bank->isCommandAllowed(l_cmdPtr, l_time))
			return false;
	}

	//send the command if the command bus is available
    if ( isCommandBusAvailable(l_cmdPtr)) {   //check if the command bus is available
		if(l_cmdPtr->getBankIdVec().size()>0)
		{
			for (auto &l_bankid:l_cmdPtr->getBankIdVec()) {
				c_BankInfo *l_bank = m_banks.at(l_bankid);
				c_BankCommand *l_cmd = new c_BankCommand(l_bankid,
														 l_cmdPtr->getCommandMnemonic(),
														 l_cmdPtr->getAddress(), *(l_cmdPtr->getHashedAddress()), l_cmdPtr->getBankIdVec());

				sendCommand(l_cmd, l_bank);

			}
		}
		else {
			c_BankInfo *l_bank = m_banks.at(l_cmdPtr->getBankId());
			sendCommand((l_cmdPtr), l_bank);
		}


		cmdQ.erase(cmdQ.begin());
		occupyCommandBus(l_cmdPtr);

		return true;
	}
	else
		return false;
}


/**
 *
 * @param l_cmdPtr
 * @return "true" if the command bus required for the input command is available.
 */
bool c_DeviceDriver::isCommandBusAvailable(c_BankCommand *l_cmdPtr) {
	uint32_t l_ChannelNum = l_cmdPtr->getHashedAddress()->getChannel();
	bool l_isColCmd = l_cmdPtr->isColCommand();


	if(l_isColCmd)
		return (m_blockColCmd.at(l_ChannelNum)==0);
	else
		return (m_blockRowCmd.at(l_ChannelNum)==0);
}


/**
 *
 * @param l_cmdPtr
 * @return "true" if all buses are occupied.
 */
bool c_DeviceDriver::occupyCommandBus(c_BankCommand *l_cmdPtr) {
	uint32_t l_ChannelNum=0;
	uint32_t l_NumAvailableBus=0;

	l_ChannelNum = l_cmdPtr->getHashedAddress()->getChannel();
	uint32_t l_cmdCycle;

	if(l_cmdPtr->getCommandMnemonic()== e_BankCommandType::ACT && k_multiCycleACT)
			l_cmdCycle=2;			//HBM requires two cycles for the active command
	else
			l_cmdCycle=1;

	//Occupy the command bus
	if (k_useDualCommandBus) {
		if (l_cmdPtr->isColCommand())
			m_blockColCmd.at(l_ChannelNum) = l_cmdCycle;
		else
			m_blockRowCmd.at(l_ChannelNum) = l_cmdCycle;
	}
	else {
		m_blockColCmd.at(l_ChannelNum) = 1;
		m_blockRowCmd.at(l_ChannelNum) = 1;
	}

	//Check whether all command buses are occupied
	for(auto & value: m_blockColCmd)
		if(value == 0)
			l_NumAvailableBus++;

	for(auto & value: m_blockRowCmd)
		if(value == 0)
			l_NumAvailableBus++;

	if(l_NumAvailableBus>0) {
		return false;
	}
	else {
		return true;  //all command buses are occupied
	}
}

/**
 *
 */
void c_DeviceDriver::releaseCommandBus() {
	for(auto & value: m_blockColCmd)
	{
		if(value>0) value--;
	}

	for(auto & value: m_blockRowCmd)
	{
		if(value>0) value--;
	}
}


/*!
 *
 * @param x_bankCommandPtr
 * @param x_bank
 * @return
 */
bool c_DeviceDriver::sendCommand(c_BankCommand* x_bankCommandPtr,
		c_BankInfo* x_bank) {
    SimTime_t l_time = m_Owner->getSimCycle();

	if (x_bank->isCommandAllowed(x_bankCommandPtr, l_time)) {
	  if(k_printCmdTrace) {

		  unsigned l_bankId=0;
	    //if(x_bankCommandPtr->getCommandMnemonic() == e_BankCommandType::REF) {
		  if(x_bankCommandPtr->getBankIdVec().size()>0)
			  l_bankId=x_bankCommandPtr->getSeqNum();
		  else
			  l_bankId=x_bankCommandPtr->getBankId();
	    if(x_bankCommandPtr->isRefreshType()) {
				(*m_cmdTraceStream) << "@" << std::dec
									<< m_Owner->getSimCycle()
									<< " " << (x_bankCommandPtr)->getCommandString()
									<< " " << std::dec << (x_bankCommandPtr)->getSeqNum()
									<< " " << std::dec << l_bankId
									<< std::endl;
	    } else {
	      (*m_cmdTraceStream) << "@" << std::dec
				  << m_Owner->getSimCycle()
				  << " " << (x_bankCommandPtr)->getCommandString()
				  << " " << std::dec << (x_bankCommandPtr)->getSeqNum()
				  << " 0x" << std::hex << (x_bankCommandPtr)->getAddress()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getChannel()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getPChannel()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getRank()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getBankGroup()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getBank()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getRow()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getCol()
				  << " " << std::dec << x_bankCommandPtr->getHashedAddress()->getCacheline()
				  << "\t" << std::dec << x_bankCommandPtr->getBankId()
				  << std::endl;
	    }
	  }

		#ifdef __SST_DEBUG_OUTPUT__
				output->verbose(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\n",
				    m_Owner->getSimCycle(),
				     x_bankCommandPtr->getCommandString().c_str(),
				     x_bankCommandPtr->getHashedAddress()->getChannel(),
				     x_bankCommandPtr->getHashedAddress()->getPChannel(),
				     x_bankCommandPtr->getHashedAddress()->getRank(),
				     x_bankCommandPtr->getHashedAddress()->getBankGroup(),
				     x_bankCommandPtr->getHashedAddress()->getBank(),
				     x_bankCommandPtr->getHashedAddress()->getRow(),
				     x_bankCommandPtr->getHashedAddress()->getCol(),
				     x_bankCommandPtr->getHashedAddress()->getBankId(),
				     x_bankCommandPtr->getSeqNum());
		#endif

		x_bank->handleCommand(x_bankCommandPtr, l_time);

		// push the command to output queue
		m_outputQ.push_back(x_bankCommandPtr);

		return true;
	} else
		return false;

}

/*!
 *
 */
void c_DeviceDriver::initACTFAWTracker()
{
	m_cmdACTFAWtrackers.clear();
	for(int i=0; i<m_numRanks;i++)
	{
		std::list<unsigned> l_cmdACTFAWTracker;
		l_cmdACTFAWTracker.clear();
		l_cmdACTFAWTracker.resize(m_bankParams.at("nFAW")-1, 0);
		m_cmdACTFAWtrackers.push_back(l_cmdACTFAWTracker);
	}
}

/*!
 *
 */
void c_DeviceDriver::initRefresh() {

	//init per-rank Refresh
	m_currentREFICount.resize(m_numRanks, 0);
	m_refreshCmdQ.resize(m_numRanks);
	m_nextBankToRefresh.resize(m_numRanks);

	for(int i=0;i<m_numRanks;i++)
	{
		//refresh commands are timely interleaved to ranks.
		m_currentREFICount[i]=m_bankParams.at("nREFI")/(i+1);
		m_nextBankToRefresh[i]=0;
	}
}

/*!
 *
 * @param x_rankid
 * @return
 */
unsigned c_DeviceDriver::getNumIssuedACTinFAW(unsigned x_rankid) {

	assert(x_rankid<m_numRanks);

	// get count of ACT cmds issued in the FAW
	unsigned l_cmdACTIssuedInFAW = 0;
	assert(m_cmdACTFAWtrackers[x_rankid].size() == m_bankParams.at("nFAW")-1);
	for(auto& l_issued : m_cmdACTFAWtrackers[x_rankid])
		l_cmdACTIssuedInFAW += l_issued;
	return l_cmdACTIssuedInFAW;
}

/*!
 *
 * @param x_cmd
 * @return
 */
bool c_DeviceDriver::push(c_BankCommand* x_cmd) {
	unsigned l_ch = x_cmd->getHashedAddress()->getChannel();
	//todo: variable queue size
	if(m_inputQ.size()<32)
	{
		m_inputQ.push_back(x_cmd);
		occupyCommandBus(x_cmd);
		return true;
	} else
		return false;
}


/*!
 )*
 * @param x_ch
 * @param x_rankID
 */
void c_DeviceDriver::createRefreshCmds(unsigned x_rankID) {
	std::vector<c_BankInfo*> l_refreshRequester;

	//create a vector of bank to be refreshed
	if(k_useSBRefresh) // per-bank refresh (single bank refresh)
	{
        //get bank list belongs to the current rank
		std::vector<c_BankInfo*>& l_banks = m_ranks[x_rankID]->getBankPtrs();
        //select a bank to be refreshed
		unsigned & l_nextBankToRefresh = m_nextBankToRefresh[x_rankID];
        //put the selected bank to the refresh requester list
		l_refreshRequester.push_back(l_banks[l_nextBankToRefresh]);

		//select next bank to be refreshed
		l_nextBankToRefresh = (l_nextBankToRefresh+1)% l_banks.size();

	} else // per-rank refresh (all bank refresh)
		l_refreshRequester = m_ranks[x_rankID]->getBankPtrs(); //all bank will be refreshed

	//get channel id --> used for the command bus arbitration
	unsigned l_ch=l_refreshRequester.front()->getBankGroupPtr()->getRankPtr()->getChannelPtr()->getChannelId();
    unsigned l_pch=0;
    if(m_numPseudoChannels>1)
    {
        l_ch=l_ch%m_numChannels;
        l_pch=l_ch/m_numChannels;
    }
	c_HashedAddress *l_hashedAddress = new c_HashedAddress(l_ch, l_pch, 0, 0, 0, 0, 0, 0);
	l_hashedAddress->setRankId(x_rankID);

	m_refreshCmdQ[x_rankID].clear();

	//add precharge commands if the bank is open
	std::vector<unsigned> l_bankVec;
	for(auto &l_bank : l_refreshRequester) {
		if(!(l_bank->getCurrentState() == e_BankState::IDLE
			 || l_bank->getCurrentState() == e_BankState::PRE))
			l_bankVec.push_back(l_bank->getBankId());
	}
	//todo: PRE --> PREA
	if(!l_bankVec.empty()) {
		c_HashedAddress *l_hashedAddress = new c_HashedAddress(l_ch, l_pch, 0, 0, 0, 0, 0, 0);
		l_hashedAddress->setRankId(x_rankID);

		m_refreshCmdQ[x_rankID].push_back(
				new c_BankCommand(0, e_BankCommandType::PRE, 0, *l_hashedAddress, l_bankVec));
		delete l_hashedAddress;
	}


	//add refresh commands
	l_bankVec.clear();
	for(auto &l_bank : l_refreshRequester) {
			l_bankVec.push_back(l_bank->getBankId());
	}
	if(!l_bankVec.empty()) {
		c_HashedAddress *l_hashedAddress = new c_HashedAddress(l_ch, l_pch, 0, 0, 0, 0, 0, 0);
		l_hashedAddress->setRankId(x_rankID);

		m_refreshCmdQ[x_rankID].push_back(
				new c_BankCommand(0, e_BankCommandType::REF, 0, *l_hashedAddress, l_bankVec));
		delete l_hashedAddress;
	}

	//add active commands
	for (auto &l_bank : l_refreshRequester) {
        for (auto &l_cmd : m_inputQ)
        {
            if(l_bank->getBankId() == l_cmd->getBankId())
            {
				//add active commands if there is a column command going to the bank closed by the refresh operation.
                if(l_cmd->isColCommand())
                {
                    ulong l_addr = l_cmd->getAddress();
                    const c_HashedAddress *l_hashedAddr = l_cmd->getHashedAddress();
                    c_BankCommand *l_newCmd = new c_BankCommand(0, e_BankCommandType::ACT, l_addr, *l_hashedAddr);
                    m_inputQ.push_front(l_newCmd);

                    break;
                } else // stop if there is a row command (precharge or active) followed by a column command
                    break;
            }
        }
	}
}

/*!
 *
 * @param x_addr
 * @return
 */
bool c_DeviceDriver::isRefreshing(const c_HashedAddress *x_addr)
{
	unsigned l_rank=x_addr->getRankId();
	if(!m_refreshCmdQ[l_rank].empty())
		return true;
	else
		return false;
}
