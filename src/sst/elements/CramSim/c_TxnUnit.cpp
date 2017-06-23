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
 * c_TxnConverter.cpp
 *
 *  Created on: May 18, 2016
 *      Author: tkarkha
 */

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
#include "sst_config.h"

// std includes
#include <iostream>
#include <assert.h>

// local includes
#include "c_TxnUnit.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdPtrPkgEvent.hpp"
#include "c_CmdResEvent.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_TxnConverter::c_TxnConverter(SST::Component *owner, SST::Params& x_params) :  c_CtrlSubComponent <c_Transaction*,c_BankCommand*> (owner, x_params) {

	m_nextSubComponent = dynamic_cast<c_Controller *>(owner)->getCmdScheduler();
	m_converter = new c_TransactionToCommands(((c_Controller *) parent)->getAddrHasher());

	// read params here
	bool l_found = false;

	// load internal params

	k_relCommandWidth = (uint32_t) x_params.find<uint32_t>("relCommandWidth", 1, l_found);
	if (!l_found) {
		std::cout << "relCommandWidth value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_REFI = (uint32_t) x_params.find<uint32_t>("nREFI", 1, l_found);
	if (!l_found) {
		std::cout << "nREFI value is missing ... exiting" << std::endl;
		exit(-1);
	}

	// refresh stuff
	// populate the m_refreshGroups with bankIds

	//per-rank refresh groups (all-banks refresh)
	uint l_groupId = 0;
	uint l_bankId = 0;
	for (uint l_chan = 0; l_chan < k_numChannelsPerDimm; l_chan++) {
		for (uint l_rank = 0; l_rank < k_numRanksPerChannel; l_rank++) {
			// every rank in a different refresh group
			m_refreshGroups.push_back(std::vector<unsigned>());
			for (uint l_bankGroup = 0; l_bankGroup < k_numBankGroupsPerRank; l_bankGroup++) {
				for (uint l_bank = 0; l_bank < k_numBanksPerBankGroup; l_bank++) {
					m_refreshGroups[l_groupId].push_back(l_bankId);
					l_bankId++;
				} // banks
			} // bankgroups
			l_groupId++;
		} // ranks
	} // channels

	m_currentRefreshGroup = 0;
	m_currentREFICount = (int) ((double) k_REFI / m_refreshGroups.size());

	int l_tmp = 0;
	for (auto l_vec : m_refreshGroups) {
		std::cout << "Refresh Group " << l_tmp << " : ";
		for (auto l_bankId : l_vec) {
			std::cout << l_bankId << " ";
		}
		std::cout << std::endl;
		l_tmp++;
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

	k_bankPolicy = (uint32_t) x_params.find<uint32_t>("bankPolicy", 0, l_found);
	if (!l_found) {
		std::cout << "bankPolicy value is missing... exiting" << std::endl;
		exit(-1);
	}

	if ((k_bankPolicy == 1) && (k_useReadA || k_useWriteA)) {
		std::cout << "Open bank/row and READA or WRITEA makes no sense" << std::endl;
		exit(-1);
	}

	k_useRefresh = (uint32_t) x_params.find<uint32_t>("boolUseRefresh", 1, l_found);
	if (!l_found) {
		std::cout << "boolUseRefresh param value is missing... exiting" << std::endl;
		exit(-1);
	}

	m_processingRefreshCmds = false;

	// Statistics setup
	s_readTxnsRecvd = registerStatistic<uint64_t>("readTxnsRecvd");
	s_writeTxnsRecvd = registerStatistic<uint64_t>("writeTxnsRecvd");
	s_totalTxnsRecvd = registerStatistic<uint64_t>("totalTxnsRecvd");
	s_reqQueueSize = registerStatistic<uint64_t>("reqQueueSize");
	s_resQueueSize = registerStatistic<uint64_t>("resQueueSize");

	//debug mask
	m_debugMask = TXNCVT;
	m_debugPrefix = "[TxnConverter]";

}

c_TxnConverter::~c_TxnConverter() {

}


void c_TxnConverter::print() const {
	std::cout << "ReqQEntries=" << k_txnReqQEntries << std::endl;
}

void c_TxnConverter::printQueues() {
	std::cout << "TxnReqQ: " << std::endl;
	for (auto it = m_inputQ.front();it!=m_inputQ.back();++it) {
		it->print();
		std::cout << std::endl;
	}
}


bool c_TxnConverter::clockTic(SST::Cycle_t){

	if (k_useRefresh) {
		// if the refresh counter is still counting, send regular Request
		// else send REF to all banks
		if (m_currentREFICount > 0) {
			--m_currentREFICount;
		} else {
			if (!m_processingRefreshCmds) {
				createRefreshCmds();
				m_processingRefreshCmds = true;
			}
		}

		//Todo: check the condition-state. (m_cmdUnitReqQTokens == k_cmdUnitReqQEntries)?
		if (m_processingRefreshCmds && (m_refreshList.size() == 0)
			/*&& (m_cmdUnitReqQTokens == k_cmdUnitReqQEntries)*/) {
			// refresh was started and now we have all tokens from CmdUnit CmdReqQ and the refresh list container is empty so refresh must have finished
			// therefore now we can start another refresh cycle
		        m_currentREFICount = (int)((double)k_REFI/m_refreshGroups.size());
			m_processingRefreshCmds = false;
		}
	}

	run();
	send();

	s_reqQueueSize->addData(m_inputQ.size());
//	s_resQueueSize->addData(m_txnResQ.size());

	return false;
}


void c_TxnConverter::createRefreshCmds() {

	m_refreshList = m_converter->getRefreshCommands(m_refreshGroups[m_currentRefreshGroup]);
	m_currentRefreshGroup++;
	if(m_currentRefreshGroup >= m_refreshGroups.size()) {
	  m_currentRefreshGroup = 0;
	}
}

void c_TxnConverter::run(){
	//std::cout<<"m_inputQ pre size():"<<m_inputQ.size()<<std::endl;
	// convert transactions to commands and push them into the command queue
	if(m_outputQ.size()==0) {
		if (m_refreshList.size() > 0) {
			while (!m_refreshList.empty()) {
				m_outputQ.push_back(m_refreshList.front());
				m_refreshList.pop();
			}
		} else if ((m_inputQ.size() > 0) && !m_processingRefreshCmds) {
			while(!m_inputQ.empty())
			{
                c_Transaction *l_reqTxn = m_inputQ.front();

                //Convert a transaction to commands
                std::vector<c_BankCommand *> l_cmdPkg = m_converter->getCommands(l_reqTxn, k_relCommandWidth, k_useReadA,
                                                                                 k_useWriteA);
                m_inputQ.pop_front();

                for (std::vector<c_BankCommand *>::iterator it = l_cmdPkg.begin(); it != l_cmdPkg.end(); ++it) {
					//print debug messages
#if 0
                    if(isDebugEnabled(TXNCVT))
                        (*it)->print(m_debugOutput);
#endif
					m_outputQ.push_back(*it);
				}
			}
		}
	}
}


void c_TxnConverter::send() {
	int token=m_nextSubComponent->getToken();

	while(token>0 && !m_outputQ.empty()) {
		//print debug message
		debug(m_debugPrefix.c_str(), m_debugMask, 1," send command to scheduler: Ch:%d, pCh:%d, rank:%d, bg:%d, b:%d, cl: %d\n",
			  m_outputQ.front()->getHashedAddress()->getChannel(),
			  m_outputQ.front()->getHashedAddress()->getPChannel(),
			  m_outputQ.front()->getHashedAddress()->getRank(),
			  m_outputQ.front()->getHashedAddress()->getBankGroup(),
			  m_outputQ.front()->getHashedAddress()->getBank(),
			  m_outputQ.front()->getHashedAddress()->getCacheline());

		m_nextSubComponent->push(m_outputQ.front());
		m_outputQ.pop_front();
		token--;
	}
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

		c_CtrlSubComponent<c_Transaction*,c_BankCommand*>::push(newTxn);
}
