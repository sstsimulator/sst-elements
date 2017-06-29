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

/*
 * c_DeviceController.cpp
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
#include "c_CmdUnit.hpp"
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

c_DeviceController::c_DeviceController(Component *owner, Params& x_params) : c_CtrlSubComponent<c_BankCommand*,c_BankCommand*>(owner, x_params) {

	m_Owner = dynamic_cast<c_Controller *>(owner);
    output = m_Owner->getOutput();

	m_refsSent = 0;
	m_inflightWrites.clear();

	// read params here
	bool l_found = false;

	k_useDualCommandBus = (uint32_t) x_params.find<uint32_t>("boolDualCommandBus", 0, l_found);
	if (!l_found) {
		std::cout << "boolDualCommandBus value is missing... disabled" << std::endl;
	}

	k_multiCycleACT = (uint32_t) x_params.find<uint32_t>("boolMultiCycleACT", 0, l_found);
	if (!l_found) {
		std::cout << "boolMultiCycleACT value is missing... disabled" << std::endl;
	}



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
		c_Channel *l_channel = new c_Channel(&m_bankParams);
		m_channel.push_back(l_channel);
		//   int l_i = 0;
		for (unsigned l_j = 0; l_j != k_numRanksPerChannel; ++l_j) {
			std::cout << "Attaching Channel" << l_i << " to Rank" << l_rankNum << std::endl;
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

	/*m_statsReqQ = new unsigned[k_cmdReqQEntries + 1];
	m_statsResQ = new unsigned[k_cmdResQEntries + 1];

	for (unsigned l_i = 0; l_i != k_cmdReqQEntries + 1; ++l_i)
		m_statsReqQ[l_i] = 0;

	for (unsigned l_i = 0; l_i != k_cmdResQEntries + 1; ++l_i)
		m_statsResQ[l_i] = 0;
*/


	// reset last data cmd issue cycle
	m_lastDataCmdIssueCycle = 0;
	m_lastDataCmdType = e_BankCommandType::READ;
	m_lastPseudoChannel = 0;

	// reset command bus
	m_blockColCmd.resize(k_numChannelsPerDimm);
	m_blockRowCmd.resize(k_numChannelsPerDimm);

	//init per-rank FAW tracker
	initACTFAWTracker();

	// set up cmd trace output
	k_printCmdTrace = (uint32_t) x_params.find<uint32_t>("boolPrintCmdTrace", 0, l_found);

	k_cmdTraceFileName = (std::string) x_params.find<std::string>("strCmdTraceFile", "-", l_found);
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

	//set debug mask and debug prefix
	m_debugMask = DVCCTRL;
	m_debugPrefix="[Device Controller]";

}
c_DeviceController::~c_DeviceController() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	// for (unsigned l_i = 0; l_i != m_numBanks; ++l_i)
	// 	delete m_banks.at(l_i);
	//
	// for (unsigned l_i = 0; l_i != k_numBankGroups; ++l_i)
	// 	delete m_bankGroups.at(l_i);
}


void c_DeviceController::print(){
	//std::cout << "***DeviceController " << SubComponent::getName() << std::endl;
	//std::cout << "Internal Queue Entries=" << k_numCtrlIntQEntries <<std::endl;
	debug(m_debugMask, 3, "Internal Queue Entries=%d", k_numCtrlIntQEntries);
}

void c_DeviceController::printQueues() {
	std::cout << "m_inputQ: " << std::endl;
	for (auto& l_entry : m_inputQ) {
		l_entry->print();
		unsigned l_bankNum = l_entry->getHashedAddress()->getBankId();

		std::cout << " - Going to Bank " << std::dec << l_bankNum << std::endl;
	}

}

bool c_DeviceController::clockTic(SST::Cycle_t) {

	for (int l_i = 0; l_i != m_banks.size(); ++l_i) {

		m_banks.at(l_i)->clockTic();
		// m_banks.at(l_i)->printState();
	}
	run();
	send();

}


//check
// 1. bank status
// 2. bus status
bool c_DeviceController::isCmdAllowed(c_BankCommand* x_bankCommandPtr)
{
	SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();
	// get count of ACT cmds issued in the FAW
	unsigned l_bankId=x_bankCommandPtr->getHashedAddress()->getBankId();



	if ((x_bankCommandPtr)->getCommandMnemonic() == e_BankCommandType::REF)
		return false;

    //check tFAW
    //todo: check tFAW per each rank
//	if ((e_BankCommandType::ACT == ((x_bankCommandPtr))->getCommandMnemonic()) && (l_cmdACTIssuedInFAW >= 4))
//		return false;

    //check inflight Write commands to the same address
    // block: READ after WRITE to the same address
    // block: WRITE after WRITE to the same address. Processor should make sure that the older WRITE is annulled but we will block the younger here.
    //todo: remove?
	if (m_inflightWrites.find((x_bankCommandPtr)->getAddress())
		!= m_inflightWrites.end()) {
		return false;
	}

    //todo: remove?
	//if(!m_blockBank.empty())
	//	if(m_blockBank.at(l_bankId))
	//		return false;

    //check timing constraints
	if (!m_banks.at(l_bankId)->isCommandAllowed(x_bankCommandPtr, l_time))
		return false;

	return true;
}



//
// Model a close bank policy as an approximation to a close row policy
// FIXME: update this to close row policy
//
// TODO: call this function only when a config knob k_closeBankPolicy is true
//
// Iterate through the cmdReqQ and mark the bank to which a cmd cannot go
// do not send any further cmds to that bank
void c_DeviceController::sendRequest() {

	m_blockBank.resize(m_numBanks,false);

	// do the member var setup up before calling any req sending policy function
	if(m_inflightWrites.size()>0)
		m_inflightWrites.clear();

	releaseCommandBus();  //update the command bus status

	// get count of ACT cmds issued in the FAW
	std::vector<unsigned> l_numACTIssuedInFAW;
	l_numACTIssuedInFAW.clear();
	for(int l_rankId=0;l_rankId<m_numRanks;l_rankId++)
	{
		l_numACTIssuedInFAW.push_back(getNumIssuedACTinFAW(l_rankId));
	}

	m_isACTIssued.clear();
	m_isACTIssued.resize(m_numRanks,false);


	for (auto l_cmdPtrItr = m_inputQ.begin(); l_cmdPtrItr != m_inputQ.end();)  {
		c_BankCommand* l_cmdPtr = (*l_cmdPtrItr);
		bool l_proceed = true;
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		unsigned l_rankNum = l_cmdPtr->getHashedAddress()->getRankId();

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
			if ( isCommandBusAvailable(l_cmdPtr)){   //check if the command bus is available

				//if(!m_blockBank.at(l_bankNum) ) {
				c_BankInfo *l_bank = m_banks.at(l_bankNum);

				if (sendCommand((l_cmdPtr), l_bank)) {

					// remove cmd from ReqQ
					l_cmdPtrItr=m_inputQ.erase(l_cmdPtrItr);

					if (l_cmdPtr->isColCommand()) {
						assert((m_lastDataCmdType != ((l_cmdPtr))->getCommandMnemonic()) ||
							   (m_lastPseudoChannel != (l_cmdPtr->getHashedAddress()->getPChannel())) ||
							   (m_lastChannel !=(l_cmdPtr->getHashedAddress()->getChannel())) ||
							   (Simulation::getSimulation()->getCurrentSimCycle() - m_lastDataCmdIssueCycle) >=
							   (std::min(m_bankParams.at("nBL"),
										 std::max(m_bankParams.at("nCCD_L"), m_bankParams.at("nCCD_S")))));

						m_lastChannel = ((l_cmdPtr))->getHashedAddress()->getChannel();
						m_lastDataCmdIssueCycle = Simulation::getSimulation()->getCurrentSimCycle();
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

//
// Model a close bank policy as an approximation to a close row policy
// FIXME: update this to close row policy
//
// TODO: call this function only when a config knob k_closeBankPolicy is true
//
// Iterate through the cmdReqQ and mark the bank to which a cmd cannot go
// do not send any further cmds to that bank
void c_DeviceController::sendReqCloseBankPolicy(
		std::deque<c_BankCommand*>::iterator x_startItr) {

	// do the member var setup up before calling any req sending policy function
	if(m_inflightWrites.size()>0)
		m_inflightWrites.clear();

	m_blockBank.clear();
	m_blockBank.resize(m_numBanks, false);
	releaseCommandBus();  //update the command bus status

	// get count of ACT cmds issued in the FAW
    std::vector<unsigned> l_numACTIssuedInFAW;
	l_numACTIssuedInFAW.clear();
    for(int l_rankId=0;l_rankId<m_numRanks;l_rankId++)
    {
		l_numACTIssuedInFAW.push_back(getNumIssuedACTinFAW(l_rankId));
    }
	std::vector<unsigned> l_isACTIssued;
	l_isACTIssued.clear();
	l_isACTIssued.resize(m_numRanks,0);


	for (auto l_cmdPtrItr = x_startItr; l_cmdPtrItr != m_inputQ.end();)  {
		c_BankCommand* l_cmdPtr = (*l_cmdPtrItr);
		bool l_proceed = true;
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		unsigned l_rankNum = l_cmdPtr->getHashedAddress()->getRankId();

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
			if ( isCommandBusAvailable(l_cmdPtr)){   //check if the command bus is available

				//if(!m_blockBank.at(l_bankNum) ) {
					c_BankInfo *l_bank = m_banks.at(l_bankNum);


					if (sendCommand((l_cmdPtr), l_bank)) {

						// remove cmd from ReqQ
						l_cmdPtrItr=m_inputQ.erase(l_cmdPtrItr);

						if (l_cmdPtr->isColCommand()) {
							assert((m_lastDataCmdType != ((l_cmdPtr))->getCommandMnemonic()) ||
								   (m_lastPseudoChannel != (l_cmdPtr->getHashedAddress()->getPChannel())) ||
								   (m_lastChannel !=(l_cmdPtr->getHashedAddress()->getChannel())) ||
								   (Simulation::getSimulation()->getCurrentSimCycle() - m_lastDataCmdIssueCycle) >=
								   (std::min(m_bankParams.at("nBL"),
											 std::max(m_bankParams.at("nCCD_L"), m_bankParams.at("nCCD_S")))));

							m_lastChannel = ((l_cmdPtr))->getHashedAddress()->getChannel();
							m_lastDataCmdIssueCycle = Simulation::getSimulation()->getCurrentSimCycle();
							m_lastDataCmdType = ((l_cmdPtr))->getCommandMnemonic();
							m_lastPseudoChannel = ((l_cmdPtr))->getHashedAddress()->getPChannel();
						}

						bool l_isACT= (e_BankCommandType::ACT == ((l_cmdPtr))->getCommandMnemonic());
						if(l_isACT)
						{
							assert(l_isACTIssued[l_rankNum]==0);
							l_isACTIssued[l_rankNum] = true;
						}


						if (occupyCommandBus(l_cmdPtr))
							break;// all command buses are occupied, so stop

						continue;
					}
				}
		//	}

		}
		++l_cmdPtrItr;
	}// for

	//update ACTFAWTracker info
	for(int l_rankNum=0;l_rankNum<m_numRanks;l_rankNum++) {
		m_cmdACTFAWtrackers[l_rankNum].push_back(l_isACTIssued[l_rankNum] ? static_cast<unsigned>(1) : static_cast<unsigned>(0));
		m_cmdACTFAWtrackers[l_rankNum].pop_front();
	}

}

//
// Model a open row policy
// FIXME: update this to open row policy
//
// TODO: call this function only when a config knob k_closeBankPolicy is set to 1
//
// Iterate through the cmdReqQ and mark the bank to which a cmd cannot go
// do not send any further cmds to that bank
/*
void c_DeviceController::sendReqOpenRowPolicy() {

	c_BankCommand* l_openBankCmdPtr = nullptr;
	auto l_openBankCmdPtrItr = m_inputQ.begin();
	for (auto l_cmdPtr : m_inputQ) {
		if (l_cmdPtr->getCommandMnemonic() == e_BankCommandType::REF)
			break;

		bool l_isDataCmd = ((((l_cmdPtr))->getCommandMnemonic()
				== e_BankCommandType::READ)
				|| (((l_cmdPtr))->getCommandMnemonic()
						== e_BankCommandType::WRITE));
		ulong l_addr = ((l_cmdPtr))->getAddress();
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		c_BankInfo* l_bankPtr = m_banks.at(l_bankNum);
        SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();

		if (l_isDataCmd && (l_bankPtr->isCommandAllowed(l_cmdPtr, l_time))
				&& (l_bankPtr->isRowOpen())
		    && (l_bankPtr->getOpenRowNum() == l_cmdPtr->getHashedAddress()->getRow()))
		{
			l_openBankCmdPtr = l_cmdPtr;

			break;
		} // if
	} // for
	if (nullptr == l_openBankCmdPtr) {

		sendReqCloseBankPolicy(m_inputQ.begin());
	} else {
		// found a command, so now remove older ACT and PRE commands
		std::list<c_BankCommand*> l_deleteList;
		l_deleteList.clear();
		for (auto l_cmdPtr : m_inputQ) {
			if (l_cmdPtr == l_openBankCmdPtr) {
				break;
			}

			bool l_isActPreCmd = ((((l_cmdPtr))->getCommandMnemonic()
					== e_BankCommandType::ACT)
					|| (((l_cmdPtr))->getCommandMnemonic()
							== e_BankCommandType::PRE));

			unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
			unsigned l_bankNumOpenBank =
			  l_openBankCmdPtr->getHashedAddress()->getBankId();

			if (l_isActPreCmd && (l_bankNum == l_bankNumOpenBank)) {
				l_deleteList.push_back(l_cmdPtr);
			}
		}
		// delete the PRE and ACT
		for (auto l_delPtr : l_deleteList) {
			// set this command as response ready
			l_delPtr->setResponseReady();
			// reclaim response queue tokens for eliminated commands
			if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::ACT)
					&& k_allocateCmdResQACT) {
				++m_cmdResQTokens;
			}
			if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::PRE)
					&& k_allocateCmdResQPRE) {
				++m_cmdResQTokens;
			}

			// erase entries in the command req q
			m_inputQ.erase(
					std::remove(m_inputQ.begin(), m_inputQ.end(), l_delPtr),
					m_inputQ.end());
		}
		sendReqCloseBankPolicy(
				std::find(m_inputQ.begin(), m_inputQ.end(),
						l_openBankCmdPtr));
	}

}
*/
////////////////
void c_DeviceController::sendReqOpenRowPolicy() {

	c_BankCommand* l_openBankCmdPtr = nullptr;
	auto l_openBankCmdPtrItr = m_inputQ.begin();
	std::deque<c_BankCommand*> tempQ=m_inputQ;
	for (auto l_cmdPtr : m_inputQ) {
		if (l_cmdPtr->getCommandMnemonic() == e_BankCommandType::REF)
			break;

		bool l_isDataCmd = ((((l_cmdPtr))->getCommandMnemonic()
							 == e_BankCommandType::READ)
							|| (((l_cmdPtr))->getCommandMnemonic()
								== e_BankCommandType::WRITE));
		ulong l_addr = ((l_cmdPtr))->getAddress();
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		c_BankInfo* l_bankPtr = m_banks.at(l_bankNum);
		SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();

		if (l_isDataCmd && (l_bankPtr->isCommandAllowed(l_cmdPtr, l_time))
			&& (l_bankPtr->isRowOpen())
			&& (l_bankPtr->getOpenRowNum() == l_cmdPtr->getHashedAddress()->getRow()))
		{
			l_openBankCmdPtr = l_cmdPtr;

			// found a command, so now remove older ACT and PRE commands
			for (auto l_cmdPtr : m_inputQ) {
				if (l_cmdPtr == l_openBankCmdPtr) {
					break;
				}

				bool l_isActPreCmd = ((((l_cmdPtr))->getCommandMnemonic()
									   == e_BankCommandType::ACT)
									  || (((l_cmdPtr))->getCommandMnemonic()
										  == e_BankCommandType::PRE));

				unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
				unsigned l_bankNumOpenBank =
						l_openBankCmdPtr->getHashedAddress()->getBankId();

				if (l_isActPreCmd && (l_bankNum == l_bankNumOpenBank)) {
					l_cmdPtr->setResponseReady();
					tempQ.erase((std::remove(tempQ.begin(), tempQ.end(), l_cmdPtr),
							tempQ.end()));
				}
			}
		} // if
	} // for
	m_inputQ.clear();
	m_inputQ=tempQ;

		sendReqCloseBankPolicy(m_inputQ.begin());
}
//
// Model a open row policy where a timer closes a row, so no infinite open row
//
// TODO: call this function only when a config knob k_closeBankPolicy is set to 1
//
// Iterate through the cmdReqQ and mark the bank to which a cmd cannot go
// do not send any further cmds to that bank
void c_DeviceController::sendReqPseudoOpenRowPolicy() {

	c_BankCommand* l_openBankCmdPtr = nullptr;
	auto l_openBankCmdPtrItr = m_inputQ.begin();
	for (auto l_cmdPtr : m_inputQ) {
		if (l_cmdPtr->getCommandMnemonic() == e_BankCommandType::REF)
			break;

		bool l_isDataCmd = ((((l_cmdPtr))->getCommandMnemonic()
				== e_BankCommandType::READ)
				|| (((l_cmdPtr))->getCommandMnemonic()
						== e_BankCommandType::WRITE));
		ulong l_addr = ((l_cmdPtr))->getAddress();
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		c_BankInfo* l_bankPtr = m_banks.at(l_bankNum);
        SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();

		if (l_isDataCmd && (l_bankPtr->isCommandAllowed(l_cmdPtr, l_time))
				&& (l_bankPtr->isRowOpen())
		    && (l_bankPtr->getOpenRowNum() == l_cmdPtr->getHashedAddress()->getRow())
				&& (l_bankPtr->getAutoPreTimer() > 0)) {
			l_openBankCmdPtr = l_cmdPtr;

			break;
		} // if
	} // for
	if (nullptr == l_openBankCmdPtr) {

		sendReqCloseBankPolicy(m_inputQ.begin());
	} else {
		// found a command, so now remove older ACT and PRE commands
		std::list<c_BankCommand*> l_deleteList;
		l_deleteList.clear();
		for (auto l_cmdPtr : m_inputQ) {
			if (l_cmdPtr == l_openBankCmdPtr) {
				break;
			}

			bool l_isActPreCmd = ((((l_cmdPtr))->getCommandMnemonic()
					== e_BankCommandType::ACT)
					|| (((l_cmdPtr))->getCommandMnemonic()
							== e_BankCommandType::PRE));
			unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
			unsigned l_bankNumOpenBank =
			  l_openBankCmdPtr->getHashedAddress()->getBankId();

			if (l_isActPreCmd && (l_bankNum == l_bankNumOpenBank)) {
				l_deleteList.push_back(l_cmdPtr);
			}
		}
		// delete the PRE and ACT
		for (auto l_delPtr : l_deleteList) {
			// set this command as response ready
			l_delPtr->setResponseReady();
			// reclaim response queue tokens for eliminated commands
			if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::ACT)
					&& k_allocateCmdResQACT) {
				++m_cmdResQTokens;
			}
			if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::PRE)
					&& k_allocateCmdResQPRE) {
				++m_cmdResQTokens;
			}

			// erase entries in the command req q
			m_inputQ.erase(
					std::remove(m_inputQ.begin(), m_inputQ.end(), l_delPtr),
					m_inputQ.end());
		}
		sendReqCloseBankPolicy(
				std::find(m_inputQ.begin(), m_inputQ.end(),
						l_openBankCmdPtr));
	}

}

//////////////
//
// Model a open bank policy
// this is approximation to the open row policy
//
// TODO: call this function only when a config knob k_closeBankPolicy is set to 2
//
// Iterate through the cmdReqQ and mark the bank to which a cmd cannot go
// do not send any further cmds to that bank
void c_DeviceController::sendReqOpenBankPolicy() {

	c_BankCommand* l_openBankCmdPtr = nullptr;
	auto l_openBankCmdPtrItr = m_inputQ.begin();
	for (auto l_cmdPtr : m_inputQ) {
		if (l_cmdPtr->getCommandMnemonic() == e_BankCommandType::REF)
			break;

		bool l_isDataCmd = ((((l_cmdPtr))->getCommandMnemonic()
				== e_BankCommandType::READ)
				|| (((l_cmdPtr))->getCommandMnemonic()
						== e_BankCommandType::WRITE));
		ulong l_addr = ((l_cmdPtr))->getAddress();
		unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
		c_BankInfo* l_bankPtr = m_banks.at(l_bankNum);
        SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();


		if (l_isDataCmd && (l_bankPtr->isCommandAllowed(l_cmdPtr, l_time))) {
			l_openBankCmdPtr = l_cmdPtr;
            // found a command, so now remove older ACT and PRE commands
            std::list<c_BankCommand*> l_deleteList;
            l_deleteList.clear();
            for (auto l_cmdPtr : m_inputQ) {
                if (l_cmdPtr == l_openBankCmdPtr) {
                    break;
                }

                bool l_isActPreCmd = ((((l_cmdPtr))->getCommandMnemonic()
                                       == e_BankCommandType::ACT)
                                      || (((l_cmdPtr))->getCommandMnemonic()
                                          == e_BankCommandType::PRE));
                unsigned l_bankNum = l_cmdPtr->getHashedAddress()->getBankId();
                unsigned l_bankNumOpenBank =
                        l_openBankCmdPtr->getHashedAddress()->getBankId();

                if (l_isActPreCmd && (l_bankNum == l_bankNumOpenBank)) {
                    l_deleteList.push_back(l_cmdPtr);
                }
            }
            // delete the PRE and ACT
            for (auto l_delPtr : l_deleteList) {
                // set this command as response ready
                l_delPtr->setResponseReady();
                // reclaim response queue tokens for eliminated commands
                if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::ACT)
                    && k_allocateCmdResQACT) {
                    ++m_cmdResQTokens;
                }
                if ((l_delPtr->getCommandMnemonic() == e_BankCommandType::PRE)
                    && k_allocateCmdResQPRE) {
                    ++m_cmdResQTokens;
                }

                // erase entries in the command req q
                m_inputQ.erase(
                        std::remove(m_inputQ.begin(), m_inputQ.end(), l_delPtr),
                        m_inputQ.end());
            }
            sendReqCloseBankPolicy(
                    std::find(m_inputQ.begin(), m_inputQ.end(),
                              l_openBankCmdPtr));
		}
	} // for
	if (nullptr == l_openBankCmdPtr) {

		sendReqCloseBankPolicy(m_inputQ.begin());
	}
}


void c_DeviceController::run() {

	// if REFs are enabled, check the Req Q's head for REF
	if (k_useRefresh && m_inputQ.size() > 0) {
		if (m_inputQ.front()->getCommandMnemonic() == e_BankCommandType::REF) {
			sendRefresh();
			return;
		}
	}

    // do the member var setup up before calling any req sending policy function
    if(m_inflightWrites.size()>0)
        m_inflightWrites.clear();

    m_blockBank.clear();
    m_blockBank.resize(m_numBanks, false);
    releaseCommandBus();  //update the command bus status
	m_isACTIssued.clear();
	m_isACTIssued.resize(m_numRanks,false);

	if (m_inputQ.size() > 0 ) {
		sendRequest();
	}

	//update ACTFAWTracker info
	for(int l_rankNum=0;l_rankNum<m_numRanks;l_rankNum++) {
		m_cmdACTFAWtrackers[l_rankNum].push_back(
				m_isACTIssued[l_rankNum] ? static_cast<unsigned>(1) : static_cast<unsigned>(0));
		m_cmdACTFAWtrackers[l_rankNum].pop_front();
	}

}
/*
void c_DeviceController::run() {

	// if REFs are enabled, check the Req Q's head for REF
	if (k_useRefresh && m_inputQ.size() > 0) {
		if (m_inputQ.front()->getCommandMnemonic() == e_BankCommandType::REF) {
			sendRefresh();
			return;
		}
	}

	if (m_inputQ.size() > 0 ) {



		switch (k_bankPolicy) {
		case 0:
			sendReqCloseBankPolicy(m_inputQ.begin());
			break;
		case 1:
			sendReqOpenRowPolicy();
			break;
		case 2:
			sendReqOpenBankPolicy();
			break;
		case 3:
			sendReqPseudoOpenRowPolicy();
			break;
		default:
			std::cout << "ERROR: k_bankPolicy not set to the support choice";
			exit(-1);
		}
	}
}
*/

void c_DeviceController::send() {
	//int token=m_Owner->getToken();
	while(!m_outputQ.empty()) {

		//print debug message
		debug(m_debugPrefix.c_str(),m_debugMask,1," send command to device: Ch:%d, pCh:%d, rank:%d, bg:%d, b:%d, cl: %d\n",
			  m_outputQ.front()->getHashedAddress()->getChannel(),
			  m_outputQ.front()->getHashedAddress()->getPChannel(),
			  m_outputQ.front()->getHashedAddress()->getRank(),
			  m_outputQ.front()->getHashedAddress()->getBankGroup(),
			  m_outputQ.front()->getHashedAddress()->getBank(),
			  m_outputQ.front()->getHashedAddress()->getCacheline());

		m_Owner->sendCommand(m_outputQ.front());
		m_outputQ.pop_front();
	}
};


void c_DeviceController::sendRefresh() {

  c_BankCommand *l_refCmd = m_inputQ.front();
  assert(l_refCmd->getCommandMnemonic() == e_BankCommandType::REF);

  // determine if all banks are ready to refresh
  for(auto l_bankId : *(l_refCmd->getBankIdVec())) {
    c_BankInfo *l_bank = m_banks.at(l_bankId);
      SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();
    if (!l_bank->isCommandAllowed(l_refCmd, l_time)) { // can't send all refs now, cancel!
      return;
    }
  }

  bool l_first = true;
  c_BankCommand *l_cmdToSend = l_refCmd;
  for(auto l_bankId : *(l_refCmd->getBankIdVec())) {
    c_BankInfo *l_bank = m_banks.at(l_bankId);
    if(l_first) {
      l_first = false;
    } else {
      // make a new bank command for each bank after the first
      // each bank deletes their own command
      l_cmdToSend = new c_BankCommand(l_refCmd->getSeqNum(), l_refCmd->getCommandMnemonic(),
				      0, l_bankId);
    }
    if(sendCommand(l_cmdToSend, l_bank)) {
		// remove cmd from ReqQ
		m_inputQ.erase(
				std::remove(m_inputQ.begin(), m_inputQ.end(),
							l_cmdToSend), m_inputQ.end());
		m_refsSent++;
    } else {
      assert(0);
    }
  }

}


/**
 *
 * @param l_cmdPtr
 * @return "true" if the command bus required for the input command is available.
 */
bool c_DeviceController::isCommandBusAvailable(c_BankCommand *l_cmdPtr) {
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
bool c_DeviceController::occupyCommandBus(c_BankCommand *l_cmdPtr) {
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
void c_DeviceController::releaseCommandBus() {
	for(auto & value: m_blockColCmd)
	{
		if(value>0) value--;
	}

	for(auto & value: m_blockRowCmd)
	{
		if(value>0) value--;
	}
}


bool c_DeviceController::sendCommand(c_BankCommand* x_bankCommandPtr,
		c_BankInfo* x_bank) {
    SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();

	if (x_bank->isCommandAllowed(x_bankCommandPtr, l_time)) {
	  if(k_printCmdTrace) {
	    if(x_bankCommandPtr->getCommandMnemonic() == e_BankCommandType::REF) {
	      (*m_cmdTraceStream) << "@" << std::dec
				  << Simulation::getSimulation()->getCurrentSimCycle()
				  << " " << (x_bankCommandPtr)->getCommandString()
				  << " " << std::dec << (x_bankCommandPtr)->getSeqNum();
		//<< " " << std::dec << x_bankCommandPtr->getBankId()
	      for(auto l_bankId : *(x_bankCommandPtr->getBankIdVec())) {
	        (*m_cmdTraceStream) << " " << l_bankId;
	      }
	      (*m_cmdTraceStream) << std::endl;
	    } else {
	      (*m_cmdTraceStream) << "@" << std::dec
				  << Simulation::getSimulation()->getCurrentSimCycle()
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
				  << "\t" << std::dec << x_bankCommandPtr->getHashedAddress()->getBankId()
				  << std::endl;
	    }
	  }

#ifdef __SST_DEBUG_OUTPUT__
        output->debug(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\\n",
                      Simulation::getSimulation()->getCurrentSimCycle(),
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
		// send command to BankState
		x_bank->handleCommand(x_bankCommandPtr, l_time);


		// send command to Dimm component
		m_outputQ.push_back(x_bankCommandPtr);

		// Decrease token if we are allocating CmdResQ space.
		// DeviceController keeps track of its own res Q tokens
		bool l_doAllocateSpace = false;
		switch (x_bankCommandPtr->getCommandMnemonic()) {
		case e_BankCommandType::ACT:
			l_doAllocateSpace = k_allocateCmdResQACT;
			break;
		case e_BankCommandType::READ:
			l_doAllocateSpace = k_allocateCmdResQREAD;
			break;
		case e_BankCommandType::READA:
			l_doAllocateSpace = k_allocateCmdResQREADA;
			break;
		case e_BankCommandType::WRITE:
			l_doAllocateSpace = k_allocateCmdResQWRITE;
			break;
		case e_BankCommandType::WRITEA:
			l_doAllocateSpace = k_allocateCmdResQWRITEA;
			break;
		case e_BankCommandType::PRE:
			l_doAllocateSpace = k_allocateCmdResQPRE;
			break;
		default:
		    break;
		}


		return true;
	} else
		return false;


}

void c_DeviceController::initACTFAWTracker()
{
	m_cmdACTFAWtrackers.clear();
	for(int i=0; i<m_numRanks;i++)
	{
		std::list<unsigned> l_cmdACTFAWTracker;
		l_cmdACTFAWTracker.clear();
		l_cmdACTFAWTracker.resize(m_bankParams.at("nFAW"), 0);
		m_cmdACTFAWtrackers.push_back(l_cmdACTFAWTracker);
	}
}

unsigned c_DeviceController::getNumIssuedACTinFAW(unsigned x_rankid) {

	assert(x_rankid<m_numRanks);

	// get count of ACT cmds issued in the FAW
	unsigned l_cmdACTIssuedInFAW = 0;
	assert(m_cmdACTFAWtrackers[x_rankid].size() == m_bankParams.at("nFAW"));
	for(auto& l_issued : m_cmdACTFAWtrackers[x_rankid])
		l_cmdACTIssuedInFAW += l_issued;
	return l_cmdACTIssuedInFAW;
}
