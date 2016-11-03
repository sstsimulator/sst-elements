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
#include "c_BankCommand.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_BankCommand::c_BankCommand(unsigned x_cmdSeqNum,
		e_BankCommandType x_cmdMnemonic, unsigned x_addr) :
		m_seqNum(x_cmdSeqNum), m_addr(x_addr), m_cmdMnemonic(x_cmdMnemonic), m_isResponseReady(
				false) {

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

unsigned c_BankCommand::getAddress() const {
	return (m_addr);
}

std::string c_BankCommand::getCommandString() const {
	return (m_cmdToString.find(m_cmdMnemonic)->second);
}

e_BankCommandType c_BankCommand::getCommandMnemonic() const {
	return (m_cmdMnemonic);
}

// acceptTransaction is called from the process that converts a c_Transaction into one or more c_BankCommand objects
// this function links the c_Transaction object with it constituent c_BankCommand objects
void c_BankCommand::acceptTransaction(c_Transaction* x_transaction) {
	m_transactionPtr = x_transaction;
}

c_Transaction* c_BankCommand::getTransaction() const {
	return (m_transactionPtr);
}

void c_BankCommand::print() const {

	std::cout << "[CMD: " << this->getCommandString() << ", SEQNUM: "
			<< std::dec << this->getSeqNum() << " , ADDR: " << std::hex
			<< this->getAddress() << " , isResponseReady: " << std::boolalpha
			<< this->isResponseReady() << " row: " << getRow() << "]"
			<< std::endl;

}

// TODO: Implement ostream operator overloading for c_BankCommand class. For some reason overloading the ostream operator does not get found during runtime

//std::ostream& operator<< (
//    std::ostream&        x_stream,
//    const c_BankCommand& x_bankCommand
//)
//{
//    x_stream<<"[CMD: "<<x_bankCommand.getCommandString()<<", SEQNUM: "<<std::dec<<x_bankCommand.getSeqNum()<<" , ADDR: "<<std::hex<<x_bankCommand.getAddress()<<" , isResponseReady: "<<std::boolalpha<<x_bankCommand.isResponseReady()<<"]";
//    return x_stream;
//}
