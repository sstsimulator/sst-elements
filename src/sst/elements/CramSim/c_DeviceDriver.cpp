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
 * @param x_params
 */
c_DeviceDriver::c_DeviceDriver(Component *owner, Params& x_params) : c_CtrlSubComponent<c_BankCommand*,c_BankCommand*>(owner, x_params) {

	m_Owner = dynamic_cast<c_Controller *>(owner);
    output = m_Owner->getOutput();

	m_inflightWrites.clear();
	m_blockBank.clear();
	m_blockBank.resize(m_numBanks, false);
	releaseCommandBus();  //update the command bus status
	m_isACTIssued.clear();
	m_isACTIssued.resize(m_numRanks, false);

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

	// reset last data cmd issue cycle
	m_lastDataCmdIssueCycle = 0;
	m_lastDataCmdType = e_BankCommandType::READ;
	m_lastPseudoChannel = 0;

	// reset command bus
	m_blockColCmd.resize(k_numChannelsPerDimm);
	m_blockRowCmd.resize(k_numChannelsPerDimm);

	//init per-rank FAW tracker
	initACTFAWTracker();

	//init per-rank Refresh
	m_isRefreshing.resize(m_numRanks,false);
	m_currentREFICount.resize(m_numRanks,0);
	m_refreshCmdQ.resize(m_numRanks);

	for(int i=0;i<m_numRanks;i++)
	{
		//refresh commands are timely interleaved to ranks.
		m_currentREFICount[i]=k_REFI/(i+1);
	}


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

	unsigned l_rankID=0;
	if (k_useRefresh) {
		for (unsigned l_ch = 0 ; l_ch < k_numChannelsPerDimm ; l_ch++)
            for (int l_rank = 0; l_rank < k_numRanksPerChannel; l_rank++) {
                if (m_currentREFICount[l_rankID] > 0) {
                    --m_currentREFICount[l_rankID];
                } else {
                    createRefreshCmds(l_ch, l_rankID);
                    m_currentREFICount[l_rankID] = k_REFI;
                }

                if (!m_refreshCmdQ[l_rankID].empty())
                    sendRefresh(l_rankID);
                else
                    m_isRefreshing[l_rankID] = false;

			l_rankID++;
		}
	}

	if (!m_inputQ.empty())
		sendRequest();
}


/*!
 *
 */
void c_DeviceDriver::update() {
	for (int l_i = 0; l_i != m_banks.size(); ++l_i) {

		m_banks.at(l_i)->clockTic();
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

		c_BankCommand* l_cmdPtr = (*l_cmdPtrItr);

		if(isRefreshing(l_cmdPtr->getHashedAddress())==true)
		{
			l_cmdPtrItr++;
			continue;
		}

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
			//skip a precharge command if the bank is already precharged
			c_BankInfo *l_bank = m_banks.at(l_bankNum);

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


/*!
 *
 * @param rank
 * @return
 */
bool c_DeviceDriver::sendRefresh(unsigned rank) {


	std::vector<c_BankCommand*> &cmdQ=m_refreshCmdQ[rank];

	c_BankCommand* l_cmdPtr = cmdQ.front();
    SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();
	std::vector<unsigned>& l_bankIdVec = l_cmdPtr->getBankIdVec();

	//check if all bank are ready for the current command
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
		// get count of ACT cmds issued in the FAW
		std::vector<unsigned> l_numACTIssuedInFAW;
		l_numACTIssuedInFAW.clear();
		for(int l_rankId=0;l_rankId<m_numRanks;l_rankId++)
		{
			l_numACTIssuedInFAW.push_back(getNumIssuedACTinFAW(l_rankId));
		}


		if ((e_BankCommandType::ACT == ((l_cmdPtr))->getCommandMnemonic()) && (l_numACTIssuedInFAW[rank] >= 4))
		{
			return false;
		}

		if (l_bankIdVec.size() > 0){
			for (auto &l_bankid:l_bankIdVec) {
				c_BankInfo *l_bank = m_banks.at(l_bankid);
				c_BankCommand *l_cmd = new c_BankCommand(l_cmdPtr->getSeqNum(), l_cmdPtr->getCommandMnemonic(),
														 l_cmdPtr->getAddress(), l_bankid);
				sendCommand((l_cmd), l_bank);
			}
		} else {
			c_BankInfo *l_bank = m_banks.at(l_cmdPtr->getBankId());
			sendCommand((l_cmdPtr), l_bank);
		}

		cmdQ.erase(cmdQ.begin());
		occupyCommandBus(l_cmdPtr);

		bool l_isACT= (e_BankCommandType::ACT == ((l_cmdPtr))->getCommandMnemonic());
		if(l_isACT)
		{
			assert(m_isACTIssued[rank]==false);
			m_isACTIssued[rank] = true;
		}
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
    SimTime_t l_time = Simulation::getSimulation()->getCurrentSimCycle();

	if (x_bank->isCommandAllowed(x_bankCommandPtr, l_time)) {
	  if(k_printCmdTrace) {
	    if(x_bankCommandPtr->getCommandMnemonic() == e_BankCommandType::REF) {
	      (*m_cmdTraceStream) << "@" << std::dec
				  << Simulation::getSimulation()->getCurrentSimCycle()
				  << " " << (x_bankCommandPtr)->getCommandString()
				  << " " << std::dec << (x_bankCommandPtr)->getSeqNum()
					<< " " << std::dec << x_bankCommandPtr->getBankId()
							  << std::endl;
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
				  << "\t" << std::dec << x_bankCommandPtr->getBankId()
				  << std::endl;
	    }
	  }

		#ifdef __SST_DEBUG_OUTPUT__
				output->verbose(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\n",
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


		x_bank->handleCommand(x_bankCommandPtr, l_time);
		// send command to Dimm component
		m_Owner->sendCommand(x_bankCommandPtr);


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
		return true;
	} else
		return false;
}


/*!
 *
 * @param x_ch
 * @param x_rankID
 */
void c_DeviceDriver::createRefreshCmds(unsigned x_ch, unsigned x_rankID) {
	std::vector<c_BankInfo*> l_bankInfo=m_ranks[x_rankID]->getBankPtrs();

	//allbank refresh
	m_refreshCmdQ[x_rankID].clear();

	//add precharge commands if the bank is open
	std::vector<unsigned> l_bankVec;
	for(auto &l_bank : l_bankInfo) {
		if(!(l_bank->getCurrentState() == e_BankState::IDLE
			 || l_bank->getCurrentState() == e_BankState::PRE))
			l_bankVec.push_back(l_bank->getBankId());
	}
	//todo: PRE --> PREA
	if(!l_bankVec.empty()) {

		m_refreshCmdQ[x_rankID].push_back(
				new c_BankCommand(0, e_BankCommandType::PRE, 0, c_HashedAddress(x_ch, 0, 0, 0, 0, 0, 0, 0), l_bankVec));

		#ifdef __SST_DEBUG_OUTPUT__
		output->verbose(CALL_INFO,2,0,"Cycle: %llu, ch: %d, rankid: %d, Insert precharge commands to close rows before issueing refresh commands\n",
						Simulation::getSimulation()->getCurrentSimCycle(),x_ch, x_rankID);
		#endif
	}


	//add refresh commands
	l_bankVec.clear();
	for(auto &l_bank : l_bankInfo) {
			l_bankVec.push_back(l_bank->getBankId());
	}
	if(!l_bankVec.empty()) {

		m_refreshCmdQ[x_rankID].push_back(
				new c_BankCommand(0, e_BankCommandType::REF, 0, c_HashedAddress(x_ch, 0, 0, 0, 0, 0, 0, 0), l_bankVec));

		#ifdef __SST_DEBUG_OUTPUT__
		output->verbose(CALL_INFO,2,0,"Cycle: %llu, ch:%d, rankid: %d, Insert refresh commands\n",
						 Simulation::getSimulation()->getCurrentSimCycle(),x_ch, x_rankID);
		#endif
	}

	//add active commands if the bank is open
	for (auto &l_bank : l_bankInfo) {
		if (l_bank->isRowOpen()) {
				c_HashedAddress *l_addr = new c_HashedAddress(x_ch, 0, 0, 0, 0, l_bank->getOpenRowNum(), 0,
															  l_bank->getBankId());
				m_refreshCmdQ[x_rankID].push_back(new c_BankCommand(0, e_BankCommandType::ACT, 0, *l_addr));

				#ifdef __SST_DEBUG_OUTPUT__
				output->verbose(CALL_INFO,2,0,"Cycle: %llu, ch: %d, rankid: %d, Insert active commands if the current bank is activated\n",
								Simulation::getSimulation()->getCurrentSimCycle(),x_ch, x_rankID);
				#endif

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

