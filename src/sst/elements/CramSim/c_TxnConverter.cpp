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

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//SST includes

// std includes
#include <iostream>
#include <assert.h>

// local includes
#include "c_TxnConverter.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdPtrPkgEvent.hpp"
#include "c_CmdResEvent.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_TxnConverter::c_TxnConverter(SST::Component *owner, SST::Params& x_params) : SubComponent(owner) {

	m_owner = dynamic_cast<c_Controller *>(owner);
	m_cmdScheduler= m_owner->getCmdScheduler();
	m_cmdSeqNum=0;
	output = m_owner->getOutput();

	unsigned l_bankNum=m_owner->getDeviceDriver()->getTotalNumBank();
	assert(l_bankNum>0);
	for(unsigned i=0; i<l_bankNum;i++)
	{
        c_BankInfo* l_bankinfo= new c_BankInfo();
        l_bankinfo->resetRowOpen();
		l_bankinfo->setBankId(i);
		m_bankInfo.push_back(l_bankinfo);
	}

	// read params here
	bool l_found = false;


	k_relCommandWidth = (uint32_t) x_params.find<uint32_t>("relCommandWidth", 1, l_found);
	if (!l_found) {
		std::cout << "relCommandWidth value is missing ... exiting" << std::endl;
		exit(-1);
	}


	k_useReadA = (uint32_t) x_params.find<uint32_t>("boolUseReadA", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseWriteA param value is missing... exiting" << std::endl;
		exit(-1);
	}

	k_useWriteA = (uint32_t) x_params.find<uint32_t>("boolUseWriteA", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseWriteA param value is missing... exiting" << std::endl;
		exit(-1);
	}

	std::string l_bankPolicy = (std::string) x_params.find<std::string>("bankPolicy", "CLOSE", l_found);
	if (!l_found) {
		std::cout << "bankPolicy value is missing... it will be \"close-page policy\" (Default)" << std::endl;
	}
	if(l_bankPolicy=="CLOSE")
		k_bankPolicy=0;
	else if(l_bankPolicy=="OPEN")
		k_bankPolicy=1;
    else if(l_bankPolicy=="POPEN")
    {
        k_bankPolicy=2;
		k_bankCloseTime =(SimTime_t) x_params.find<SimTime_t>("bankCloseTime",1000, l_found);
		if(!l_found){
			std::cout <<"bank policy is set to pseudo open policy, but bankCloseTime value is missing... exiting" <<std::endl;
			exit(-1);
		}
		if(k_bankPolicy==2) {
			for (auto &it:m_bankInfo)
				it->setAutoPreTimer(k_bankCloseTime);
		}
    }
	else
	{
		std::cout << "TxnConverter: bank policy error!!\n";
		exit(-1);
	}

	if ((k_bankPolicy == 1 || k_bankPolicy==2 ) && (k_useReadA || k_useWriteA)) {
		std::cout << "Open bank/row and READA or WRITEA makes no sense" << std::endl;
		exit(-1);
	}


	// Statistics setup
	s_readTxnsRecvd = registerStatistic<uint64_t>("readTxnsRecvd");
	s_writeTxnsRecvd = registerStatistic<uint64_t>("writeTxnsRecvd");
	s_totalTxnsRecvd = registerStatistic<uint64_t>("totalTxnsRecvd");

}

c_TxnConverter::~c_TxnConverter() {
	for(auto &it : m_bankInfo)
	{
		delete it;
	}
}



void c_TxnConverter::run(){

	for (std::deque<c_Transaction*>::iterator l_it=m_inputQ.begin(); l_it!=m_inputQ.end();) {
		c_Transaction* l_reqTxn=*l_it;


		unsigned l_numToken=m_cmdScheduler->getToken(l_reqTxn->getHashedAddress());
		if(l_numToken>=3) {
			//1. Convert a transaction to commands
			std::vector<c_BankCommand *> l_cmdPkg = getCommands(l_reqTxn);

			//2. Send commands to the command scheduler
			if (!l_cmdPkg.empty()) {
				for (auto &it : l_cmdPkg) {
					c_BankCommand* l_cmd = it;
					bool isSuccess = m_cmdScheduler->push(l_cmd);
					l_cmd->print(output, "[c_TxnConverter]", m_owner->getSimCycle());
					assert(isSuccess);
				}
				updateBankInfo(l_reqTxn);
				l_it = m_inputQ.erase(l_it);

			} else
				l_it++;
		} else
			l_it++;
	}

	//For psuedo open page policy, update bankinfo
	if(k_bankPolicy==2) {
		for (auto &it:m_bankInfo)
			if(it->isRowOpen())
				it->clockTic(m_owner->getSimCycle());
	}
	assert(m_inputQ.empty());
}



void c_TxnConverter::push(c_Transaction* newTxn) {

	// make sure the internal req q has at least one empty entry
	// to accept a new txn ptr
		if(newTxn->getTransactionMnemonic() == e_TransactionType::READ) {
		  s_readTxnsRecvd->addData(1);
		}
		if(newTxn->getTransactionMnemonic() == e_TransactionType::WRITE) {
		  s_writeTxnsRecvd->addData(1);
		}
		s_totalTxnsRecvd->addData(1);

		m_inputQ.push_back(newTxn);
}


std::vector<c_BankCommand*> c_TxnConverter::getCommands(c_Transaction* x_txn) {

	std::vector<c_BankCommand*> l_commandVec;

	//1. Generate a command sequence for a transaction
	unsigned l_numCmdsPerTrans = x_txn->getDataWidth() / k_relCommandWidth;
	for (unsigned l_i = 0; l_i < l_numCmdsPerTrans; ++l_i) {
		ulong l_nAddr = x_txn->getAddress() + (k_relCommandWidth * l_i);
		const c_HashedAddress &l_hashedAddr=x_txn->getHashedAddress();

		getPreCommands(l_commandVec,x_txn,l_nAddr);
		getPostCommands(l_commandVec,x_txn,l_nAddr);
	}

	//2. Record the information of the generated commands for the transaction
	x_txn->setWaitingCommands(1);
	x_txn->isProcessed(true);
	for (auto& l_cmd : l_commandVec) {
		x_txn->addCommandPtr(l_cmd); // only copies seq num
	}

	return (l_commandVec);
}


void c_TxnConverter::getPreCommands(
		std::vector<c_BankCommand*> &x_commandVec, c_Transaction* x_txn, ulong x_nAddr) {
	unsigned l_bankId=x_txn->getHashedAddress().getBankId();
	c_BankInfo* l_bankinfo=m_bankInfo.at(l_bankId);
	const c_HashedAddress &l_hashedAddr=x_txn->getHashedAddress();

	//close policy
	if(k_bankPolicy==0)
	{
		x_commandVec.push_back(
			new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::ACT, x_nAddr, l_hashedAddr));
	}//open policy or pseudo open
	else if(k_bankPolicy==1 || k_bankPolicy==2)
	{
		if(l_bankinfo->isRowOpen())
		{
			unsigned l_row = x_txn->getHashedAddress().getRow();

			if(l_bankinfo->getOpenRowNum()!=l_row) {
				//other row is open, so need to close
				x_commandVec.push_back(new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::PRE, x_nAddr, l_hashedAddr));
				//open row
				x_commandVec.push_back(new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::ACT, x_nAddr, l_hashedAddr));

				if(k_bankPolicy==2)
				{
					l_bankinfo->setAutoPreTimer(k_bankCloseTime);
				}
			}
		} else
		{
			//open row
			x_commandVec.push_back(
					new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::ACT, x_nAddr, l_hashedAddr));
			if(k_bankPolicy==2)
				{
					l_bankinfo->setAutoPreTimer(k_bankCloseTime);
				}
		}
	}

}


void c_TxnConverter::getPostCommands(
		std::vector<c_BankCommand*> &x_commandVec,  c_Transaction* x_txn, ulong x_nAddr) {

	const c_HashedAddress &l_hashedAddr = x_txn->getHashedAddress();
	e_BankCommandType l_CmdType;
	bool l_useAutoPRE=false;


	if(k_bankPolicy==0) //close policy
	{
		//1. Decode column command
		if (e_TransactionType::READ == x_txn->getTransactionMnemonic())
		{
			if(k_useReadA) {
				l_CmdType=e_BankCommandType::READA;
				l_useAutoPRE=true;
			} else {
				l_CmdType=e_BankCommandType::READ;
				l_useAutoPRE=false;
			}
		} else {
			if(k_useReadA) {
				l_CmdType=e_BankCommandType::WRITEA;
				l_useAutoPRE=true;
			}
			else {
				l_CmdType=e_BankCommandType::WRITE;
				l_useAutoPRE=false;
			}
		}

		x_commandVec.push_back(
				new c_BankCommand(m_cmdSeqNum++, l_CmdType, x_nAddr, l_hashedAddr));

		// Last command will be precharge IFF ReadA is not used
		if (!l_useAutoPRE)
			x_commandVec.push_back(
					new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::PRE,
									  x_nAddr, l_hashedAddr));

	} else if(k_bankPolicy==1 || k_bankPolicy==2){		//open policy

		if (e_TransactionType::READ == x_txn->getTransactionMnemonic())
				l_CmdType=e_BankCommandType::READ;
		else
				l_CmdType=e_BankCommandType::WRITE;

		x_commandVec.push_back(
				new c_BankCommand(m_cmdSeqNum++, l_CmdType, x_nAddr, l_hashedAddr));

		if(k_bankPolicy==2)
		{
			unsigned l_bankId=x_txn->getHashedAddress().getBankId();
			c_BankInfo* l_bankinfo=m_bankInfo.at(l_bankId);

			if(l_bankinfo->getAutoPreTimer()==0)
			{
				x_commandVec.push_back(new c_BankCommand(m_cmdSeqNum++, e_BankCommandType::PRE,
								  x_nAddr, l_hashedAddr));
			}
		}
	} else{
		printf("bank policy error!!");
		exit(1);
	}
}


void c_TxnConverter::updateBankInfo(c_Transaction* x_txn)
{
	unsigned l_bankid=x_txn->getHashedAddress().getBankId();
	unsigned l_row=x_txn->getHashedAddress().getRow();

	if(k_bankPolicy==0)//close policy
	{
		m_bankInfo.at(l_bankid)->resetRowOpen();
	} else if(k_bankPolicy==1) //open policy
	{
		m_bankInfo.at(l_bankid)->setRowOpen();
		m_bankInfo.at(l_bankid)->setOpenRowNum(l_row);
	}
	else if(k_bankPolicy==2) //pseudo open policy
	{

		if(m_bankInfo.at(l_bankid)->getAutoPreTimer()==0)
		{
			m_bankInfo.at(l_bankid)->resetRowOpen();
		}
		else
		{
			m_bankInfo.at(l_bankid)->setRowOpen();
			m_bankInfo.at(l_bankid)->setOpenRowNum(l_row);
		}
	}
	else{
		printf("bank policy error!!");
		exit(1);
	}
}


c_BankInfo* c_TxnConverter::getBankInfo(unsigned x_bankId)
{
	return m_bankInfo[x_bankId];
}
