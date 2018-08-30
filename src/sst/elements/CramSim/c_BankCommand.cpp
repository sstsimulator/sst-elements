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
#include <assert.h>
#include "sst/core/simulation.h"
#include "c_BankCommand.hpp"
//#include "c_Transaction.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankCommand::c_BankCommand(unsigned x_cmdSeqNum,
			     e_BankCommandType x_cmdMnemonic, ulong x_addr,
			     const c_HashedAddress &x_hashedAddr) :
		m_seqNum(x_cmdSeqNum), m_addr(x_addr), m_cmdMnemonic(x_cmdMnemonic),
		m_isResponseReady(false), m_hashedAddr(x_hashedAddr), m_bankId(x_hashedAddr.getBankId()), m_isRefreshType(false) {

	m_cmdToString[e_BankCommandType::ERR] = "ERR";
	m_cmdToString[e_BankCommandType::ACT] = "ACT";
	m_cmdToString[e_BankCommandType::READ] = "READ";
	m_cmdToString[e_BankCommandType::READA] = "READA";
	m_cmdToString[e_BankCommandType::WRITE] = "WRITE";
	m_cmdToString[e_BankCommandType::WRITEA] = "WRITEA";
	m_cmdToString[e_BankCommandType::PRE] = "PRE";
	m_cmdToString[e_BankCommandType::PREA] = "PREA";
	m_cmdToString[e_BankCommandType::REF] = "REF";
}

c_BankCommand::c_BankCommand(unsigned x_cmdSeqNum,
			     e_BankCommandType x_cmdMnemonic, ulong x_addr,
			     unsigned x_bankId) :
		m_seqNum(x_cmdSeqNum), m_addr(x_addr), m_cmdMnemonic(x_cmdMnemonic),
		m_isResponseReady(false), m_bankId(x_bankId), m_isRefreshType(true) {

	assert(x_cmdMnemonic == e_BankCommandType::REF ||x_cmdMnemonic == e_BankCommandType::PRE); // This constructor only for REF cmds!

	m_cmdToString[e_BankCommandType::ERR] = "ERR";
	m_cmdToString[e_BankCommandType::ACT] = "ACT";
	m_cmdToString[e_BankCommandType::READ] = "READ";
	m_cmdToString[e_BankCommandType::READA] = "READA";
	m_cmdToString[e_BankCommandType::WRITE] = "WRITE";
	m_cmdToString[e_BankCommandType::WRITEA] = "WRITEA";
	m_cmdToString[e_BankCommandType::PRE] = "PRE";
	m_cmdToString[e_BankCommandType::PREA] = "PREA";
	m_cmdToString[e_BankCommandType::REF] = "REF";
}

c_BankCommand::c_BankCommand(unsigned x_cmdSeqNum,
			     e_BankCommandType x_cmdMnemonic, ulong x_addr, const c_HashedAddress &x_hashedAddr,
			     std::vector<unsigned> &x_bankIdVec) :
		m_seqNum(x_cmdSeqNum), m_addr(x_addr), m_cmdMnemonic(x_cmdMnemonic),
		m_isResponseReady(false), m_bankIdVec(x_bankIdVec), m_isRefreshType(true) {

        assert((x_cmdMnemonic == e_BankCommandType::REF||x_cmdMnemonic == e_BankCommandType::PRE)); // This constructor only for REF cmds!

	m_hashedAddr = x_hashedAddr;
	m_bankId = x_bankIdVec.front();
	
	m_cmdToString[e_BankCommandType::ERR] = "ERR";
	m_cmdToString[e_BankCommandType::ACT] = "ACT";
	m_cmdToString[e_BankCommandType::READ] = "READ";
	m_cmdToString[e_BankCommandType::READA] = "READA";
	m_cmdToString[e_BankCommandType::WRITE] = "WRITE";
	m_cmdToString[e_BankCommandType::WRITEA] = "WRITEA";
	m_cmdToString[e_BankCommandType::PRE] = "PRE";
	m_cmdToString[e_BankCommandType::PREA] = "PREA";
	m_cmdToString[e_BankCommandType::REF] = "REF";
}

ulong c_BankCommand::getAddress() const {
	return (m_addr);
}

std::string c_BankCommand::getCommandString() const {
  return (m_cmdToString.find(m_cmdMnemonic)->second);
}

e_BankCommandType c_BankCommand::getCommandMnemonic() const {
	return (m_cmdMnemonic);
}


void c_BankCommand::print(SimTime_t x_cycle) const {
  std::cout << "[" << this << " Cycle:" <<  x_cycle
			<< " CMD: " << this->getCommandString()
			<< ", SEQNUM: " << std::dec << this->getSeqNum()
			<< ", ADDR: 0x" << std::hex << this->getAddress()
			<< ", isResponseReady: " << std::boolalpha << this->isResponseReady()
		    << ", BankId: " << std::dec << this->getHashedAddress()->getBankId()
			<< ", Ch: " << std::dec << this->getHashedAddress()->getChannel()
			<< ", Pch: " << std::dec << this->getHashedAddress()->getPChannel()
		    << ", Rank: " << std::dec << this->getHashedAddress()->getRank()
			<< ", BG: " << std::dec << this->getHashedAddress()->getBankGroup()
			<< ", Bank: " << std::dec << this->getHashedAddress()->getBank()
			<< ", Row: " << std::dec << this->getHashedAddress()->getRow()
			<< ", Col: " << std::dec << this->getHashedAddress()->getCol()
		  	<< ", Cacheline: " << std::dec << this->getHashedAddress()->getCacheline() << "]"
			<< std::endl;

}

void c_BankCommand::print(SST::Output *x_debugOutput, SimTime_t x_cycle) const {
	x_debugOutput->verbose(CALL_INFO, 1, 0, "[BankCommand] Cycle:%llu," , x_cycle);
	x_debugOutput->verbose(CALL_INFO, 1, 0, "CMD:%s,",this->getCommandString().c_str());
	x_debugOutput->verbose(CALL_INFO, 1, 0,	"SEQNUM:%d,",this->getSeqNum());
	x_debugOutput->verbose(CALL_INFO, 1, 0,	"ADDR:%lx,",this->getAddress());
	x_debugOutput->verbose(CALL_INFO, 1, 0,	"isResponseReady:%d,",this->isResponseReady());
	x_debugOutput->verbose(CALL_INFO, 1, 0,	"BankId:%d, ", this->getHashedAddress()->getBankId());
	x_debugOutput->verbose(CALL_INFO, 1, 0,	"Ch:%d,",this->getHashedAddress()->getChannel());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Pch:%d, ",this->getHashedAddress()->getPChannel());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Rank:%d,",this->getHashedAddress()->getRank());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "BG:%d,", this->getHashedAddress()->getBankGroup());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Bank:%d,",this->getHashedAddress()->getBank());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Row:%d,",this->getHashedAddress()->getRow());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Col:%d,",this->getHashedAddress()->getCol());
	x_debugOutput->verbose(CALL_INFO, 1, 0, "Cacheline:%d\n",this->getHashedAddress()->getCacheline());
	x_debugOutput->flush();
}

void c_BankCommand::print(SST::Output *x_debugOutput,const std::string x_prefix, SimTime_t x_cycle) const {
	x_debugOutput->verbosePrefix(x_prefix.c_str(),CALL_INFO,1,0,"Cycle:%lld Cmd:%s seqNum: %llu CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d\n",
							x_cycle,
							getCommandString().c_str(),
							m_seqNum,
							getHashedAddress()->getChannel(),
							getHashedAddress()->getPChannel(),
							getHashedAddress()->getRank(),
							getHashedAddress()->getBankGroup(),
							getHashedAddress()->getBank(),
							getHashedAddress()->getRow(),
							getHashedAddress()->getCol(),
							getHashedAddress()->getBankId());

}


void c_BankCommand::serialize_order(SST::Core::Serialization::serializer &ser)
{
  ser & m_seqNum;
  ser & m_addr;
  ser & m_row;
  ser & m_bankId;
  ser & m_bankIdVec;
  ser & m_cmdMnemonic;
  ser & m_cmdToString;
  ser & m_isResponseReady;
  ser & m_isResponseReady;
  
  ser & m_hashedAddr;

}
