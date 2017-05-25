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

#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <queue>
#include <assert.h>

#include "c_TransactionToCommands.hpp"
#include "c_AddressHasher.hpp"

using namespace SST;
using namespace n_Bank;

c_TransactionToCommands* c_TransactionToCommands::m_instance = nullptr;
unsigned c_TransactionToCommands::m_cmdSeqNum = 0;

c_TransactionToCommands* c_TransactionToCommands::getInstance() {
	if (nullptr == m_instance) {
		m_instance->construct();
	}

	return (m_instance);
}

void c_TransactionToCommands::construct() {
}

std::vector<c_BankCommand*> c_TransactionToCommands::getCommands(
		c_Transaction* x_txn, unsigned x_relCommandWidth, bool x_useReadA,
		bool x_useWriteA) {
	std::vector<c_BankCommand*> l_commandVec;

	if (e_TransactionType::READ == x_txn->getTransactionMnemonic())
		l_commandVec = getReadCommands(x_txn, x_relCommandWidth, x_useReadA);
	else
		l_commandVec = getWriteCommands(x_txn, x_relCommandWidth, x_useWriteA);


	// set the number of waiting commands for the transaction
	x_txn->setWaitingCommands(l_commandVec.size()/3);

	x_txn->isProcessed(true);

	for (auto& l_cmd : l_commandVec) {
	  //l_cmd->acceptTransaction(x_txn);
	  x_txn->addCommandPtr(l_cmd); // only copies seq num
	}

	return (l_commandVec);
}

std::vector<c_BankCommand*> c_TransactionToCommands::getReadCommands(
		c_Transaction* x_txn, unsigned x_relCommandWidth, bool x_useReadA) {

	std::vector<c_BankCommand*> l_commandVec;

	e_BankCommandType l_readCmdType = (x_useReadA) ? e_BankCommandType::READA
			: e_BankCommandType::READ;


	unsigned l_numCmdsPerTrans = x_txn->getDataWidth() / x_relCommandWidth;

	for (unsigned l_i = 0; l_i < l_numCmdsPerTrans; ++l_i) {
 	        ulong l_nAddr = x_txn->getAddress() + (x_relCommandWidth * l_i);
		c_HashedAddress l_hashedAddr;
		c_AddressHasher::getInstance()->fillHashedAddress(&l_hashedAddr,l_nAddr);
	  
		l_commandVec.push_back(
			new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::ACT,
					  l_nAddr, l_hashedAddr));

//		std::cout << __PRETTY_FUNCTION__ << " m_cmdSeqNum = "<<m_cmdSeqNum<<std::endl;


		l_commandVec.push_back(
			new c_BankCommand(m_cmdSeqNum++, l_readCmdType,
					   l_nAddr, l_hashedAddr));

//		std::cout << __PRETTY_FUNCTION__ << " m_cmdSeqNum = "<<m_cmdSeqNum<<std::endl;


		// Last command will be precharge IFF ReadA is not used
		if (!x_useReadA)
		  l_commandVec.push_back(
		          new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::PRE,
					    l_nAddr, l_hashedAddr));
	}

//	std::cout << __PRETTY_FUNCTION__ << " m_cmdSeqNum = "<<m_cmdSeqNum<<std::endl;

	return l_commandVec;
}

std::vector<c_BankCommand*> c_TransactionToCommands::getWriteCommands(
		c_Transaction* x_txn, unsigned x_relCommandWidth, bool x_useWriteA) {

	std::vector<c_BankCommand*> l_commandVec;

	e_BankCommandType l_writeCmdType =
			(x_useWriteA) ? e_BankCommandType::WRITEA
					: e_BankCommandType::WRITE;


	unsigned l_numCmdsPerTrans = x_txn->getDataWidth() / x_relCommandWidth;

	for (unsigned l_i = 0; l_i < l_numCmdsPerTrans; ++l_i) {
 	        ulong l_nAddr = x_txn->getAddress() + (x_relCommandWidth * l_i);
		c_HashedAddress l_hashedAddr;
		c_AddressHasher::getInstance()->fillHashedAddress(&l_hashedAddr,l_nAddr);
		
		l_commandVec.push_back(
				new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::ACT,
						  l_nAddr, l_hashedAddr));

		l_commandVec.push_back(
				new c_BankCommand(m_cmdSeqNum++, l_writeCmdType,
						  l_nAddr, l_hashedAddr));

		// Last command will be precharge IFF WriteA is not used
		if (!x_useWriteA)
				l_commandVec.push_back(
					new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::PRE,
							  l_nAddr, l_hashedAddr));
	}



	return l_commandVec;
}

std::queue<c_BankCommand*> c_TransactionToCommands::getRefreshCommands(unsigned x_numBanks) {
	std::queue<c_BankCommand*> l_commandVec;

	for (unsigned l_i = 0; l_i != x_numBanks; ++l_i) {
		ulong l_addr = 32 * l_i;
		l_commandVec.push(
			new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::REF,
					l_addr)
		);
	}
//	std::cout << __PRETTY_FUNCTION__ << " m_cmdSeqNum = "<<m_cmdSeqNum<<std::endl;

	return l_commandVec;
}

std::queue<c_BankCommand*> c_TransactionToCommands::getRefreshCommands(std::vector<unsigned> &x_refreshGroup) {
	std::queue<c_BankCommand*> l_commandVec;

	//	for(ulong l_bankId : x_refreshGroup) {
	//  l_commandVec.push(new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::REF, 0,
	//				      l_bankId)
	//		    );
	//}
	l_commandVec.push(new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::REF, 0,
					    x_refreshGroup));
	//std::cout << __PRETTY_FUNCTION__ << " m_cmdSeqNum = "<<m_cmdSeqNum<<std::endl;

	return l_commandVec;
}
