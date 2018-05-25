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

#include <algorithm>

#include "c_Transaction.hpp"
#include "c_BankCommand.hpp"
#include "c_AddressHasher.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_Transaction::c_Transaction(ulong x_seqNum, e_TransactionType x_txnMnemonic,
			     ulong x_addr, unsigned x_dataWidth) :
		m_seqNum(x_seqNum), m_txnMnemonic(x_txnMnemonic), m_addr(x_addr), m_isResponseReady(
				false), m_numWaitingCommands(0), m_dataWidth(x_dataWidth), m_processed(
				false) {
       
	m_hasHashedAddr= false;

}

void c_Transaction::setWaitingCommands(const unsigned x_numWaitingCommands) {
	m_numWaitingCommands = x_numWaitingCommands;
}

unsigned c_Transaction::getWaitingCommands() const {
	return (m_numWaitingCommands);
}

//
c_Transaction::~c_Transaction() {
	// delete the list of commands that this transaction translates into
}

ulong c_Transaction::getAddress() const {
	return (m_addr);
}

std::string c_Transaction::getTransactionString() const {
	if(m_txnMnemonic==e_TransactionType::READ)
		return "READ";
	else
		return "WRITE";
	//return (m_txnToString.find(m_txnMnemonic)->second);
}

e_TransactionType c_Transaction::getTransactionMnemonic() const {
	return (m_txnMnemonic);
}

void c_Transaction::setResponseReady() {
	m_isResponseReady = true;
}

bool c_Transaction::isResponseReady() {
	return (m_isResponseReady);
}

ulong c_Transaction::getSeqNum() const {
	return (m_seqNum);
}

bool c_Transaction::matchesCmdSeqNum(ulong x_seqNum) {
  return(std::find(m_cmdSeqNumList.begin(), m_cmdSeqNumList.end(), x_seqNum)
	 != m_cmdSeqNumList.end());
}

void c_Transaction::addCommandPtr(c_BankCommand* x_cmdPtr) {
  //m_cmdPtrList.push_back(x_cmdPtr);
  m_cmdSeqNumList.push_back(x_cmdPtr->getSeqNum());
}

unsigned c_Transaction::getDataWidth() const {
	return (m_dataWidth);
}

bool c_Transaction::isProcessed() const {
	return (m_processed);
}

void c_Transaction::isProcessed(bool x_processed) {
	m_processed = x_processed;
}

//
// FIXME: print function should be actually be overloaded in operator<< but for some reason operator overloading does not working when creating the library, so for now we will have the print function.
void c_Transaction::print() const {
  std::cout << this << " " << getTransactionString() << ", seqNum: " << std::dec << m_seqNum
	    << ", address: 0x" << std::hex << getAddress() << std::dec
	    << ", dataWidth = " << m_dataWidth << ", m_numWaitingCommands = "
	    << std::dec << m_numWaitingCommands << ", isProcessed = "
	    << std::boolalpha << m_processed << ", isResponseReady = "
	    << std::boolalpha << m_isResponseReady <<std::endl;
}


void c_Transaction::print(SST::Output *x_output, const std::string x_prefix, SimTime_t x_cycle) const {
	x_output->verbosePrefix(x_prefix.c_str(),CALL_INFO,1,0,"Cycle:%lld Cmd:%s seqNum: %llu addr:0x%lx CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d\n",
				  x_cycle,
				  getTransactionString().c_str(),
					m_seqNum,
							m_addr,
				  getHashedAddress().getChannel(),
				  getHashedAddress().getPChannel(),
				  getHashedAddress().getRank(),
				  getHashedAddress().getBankGroup(),
				  getHashedAddress().getBank(),
				  getHashedAddress().getRow(),
				  getHashedAddress().getCol(),
				  getHashedAddress().getBankId()
	);
}

void c_Transaction::serialize_order(SST::Core::Serialization::serializer &ser)
{
  ser & m_seqNum;
  ser & m_txnMnemonic;
  ser & m_addr;
  //ser & m_txnToString;
    
  ser & m_isResponseReady;
  ser & m_numWaitingCommands;
  ser & m_dataWidth;
  ser & m_processed;
	ser & m_hasHashedAddr;

}
