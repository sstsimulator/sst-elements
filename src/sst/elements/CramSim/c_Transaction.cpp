// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

#include "c_Transaction.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_Transaction::c_Transaction(unsigned x_seqNum, e_TransactionType x_txnMnemonic,
		unsigned x_addr, unsigned x_dataWidth) :
		m_seqNum(x_seqNum), m_txnMnemonic(x_txnMnemonic), m_addr(x_addr), m_isResponseReady(
				false), m_numWaitingCommands(0), m_dataWidth(x_dataWidth), m_processed(
				false) {

	m_txnToString[e_TransactionType::READ] = "READ";
	m_txnToString[e_TransactionType::WRITE] = "WRITE";
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

unsigned c_Transaction::getAddress() const {
	return (m_addr);
}

std::string c_Transaction::getTransactionString() const {
	return (m_txnToString.find(m_txnMnemonic)->second);
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

//std::ostream& operator<< (std::ostream& x_stream, const c_Transaction& x_transaction)
//{
//  x_stream<<x_transaction.getTransactionString()<<", seqNum: "<<std::dec<<x_transaction.m_seqNum<<", address: "<<std::hex<<x_transaction.getAddress()<<std::dec<<", dataWidth = "<<x_transaction.m_dataWidth<<", m_numWaitingCommands = "<<std::dec<<x_transaction.m_numWaitingCommands<<", isProcessed = "<<std::boolalpha<<x_transaction.m_processed<<", isResponseReady = "<<std::boolalpha<<x_transaction.m_isResponseReady;
//  return (x_stream);
//}

void c_Transaction::addCommandPtr(c_BankCommand* x_cmdPtr) {
	m_cmdPtrList.push_back(x_cmdPtr);
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
	std::cout << getTransactionString() << ", seqNum: " << std::dec << m_seqNum
			<< ", address: " << std::hex << getAddress() << std::dec
			<< ", dataWidth = " << m_dataWidth << ", m_numWaitingCommands = "
			<< std::dec << m_numWaitingCommands << ", isProcessed = "
			<< std::boolalpha << m_processed << ", isResponseReady = "
			<< std::boolalpha << m_isResponseReady;
}

