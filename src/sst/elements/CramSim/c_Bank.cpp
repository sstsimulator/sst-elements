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

// C++ includes
#include <memory>
#include <string>
#include <assert.h>

// CramSim includes
#include "c_Dimm.hpp"
#include "c_Bank.hpp"
#include "c_Transaction.hpp"
//#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_BankCommand.hpp"

using namespace SST;
using namespace SST::n_Bank;


unsigned c_Bank::k_banks = 0;

c_Bank::c_Bank(SST::Params& x_params,unsigned x_bankId) {
	// read params here

	bool l_found = false;

	/* BUFFER ALLOCATION PARAMETERS */
	k_allocateCmdResQACT = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResACT", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResACT value is missing... exiting" << std::endl;
	}

	k_allocateCmdResQREAD = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResREAD", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResREAD value is missing... exiting" << std::endl;
	}

	k_allocateCmdResQREADA = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResREADA", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResREADA value is missing... exiting" << std::endl;
	}

	k_allocateCmdResQWRITE = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResWRITE", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResWRITE value is missing... exiting" << std::endl;
	}

	k_allocateCmdResQWRITEA = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResWRITEA", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResWRITEA value is missing... exiting" << std::endl;
	}

	k_allocateCmdResQPRE = (uint32_t)x_params.find<uint32_t>("boolAllocateCmdResPRE", 1, l_found);
	if (!l_found){
		std::cout << "boolAllocateCmdResPRE value is missing... exiting" << std::endl;
	}

	m_cmd = nullptr;

	/*--static variable makes an error in multi-lane configuration.
	    so, fixed to set the bank id with an input argument --*/
	//m_bankNum = k_banks;
	//k_banks++;
	m_bankNum = x_bankId;

        m_prevOpenRow = 0;

	m_ACTCmdsReceived = 0;
	m_READCmdsReceived = 0;
	m_WRITECmdsReceived = 0;
	m_PRECmdsReceived = 0;

	m_ACTCmdsSent = 0;
	m_READCmdsSent = 0;
	m_WRITECmdsSent = 0;
	m_PRECmdsSent = 0;

	m_bankStats = NULL;
}

c_Bank::~c_Bank() {
		finish();
}

void c_Bank::handleCommand(c_BankCommand* x_bankCommandPtr) {


  //std::cout << "bankNum " << m_bankNum << std::endl;
  if(m_cmd) {
   // std::cout << "m_cmd = ";       m_cmd->print();
   // std::cout << "x_cmd = ";       x_bankCommandPtr->print();
  }

	assert(nullptr == m_cmd);

	if (nullptr == m_cmd) {

		m_cmd = x_bankCommandPtr;

                unsigned l_row = m_cmd->getHashedAddress()->getRow();
		
                switch (x_bankCommandPtr->getCommandMnemonic()){
			case e_BankCommandType::ACT:
				m_ACTCmdsReceived++;
				m_bankStats->s_bankACTsRecvd->addData(1);
				break;
			case e_BankCommandType::READ:
			case e_BankCommandType::READA:
				m_READCmdsReceived++;
				m_bankStats->s_bankREADsRecvd->addData(1);
                                
                                if(m_prevOpenRow == l_row)
                                {
                                    m_bankStats->s_bankRowHits->addData(1);
                                    m_bankStats->s_totalRowHits->addData(1);
                                }
                                else
                                    m_prevOpenRow = m_cmd->getHashedAddress()->getRow();
				break;
			case e_BankCommandType::WRITE:
			case e_BankCommandType::WRITEA:
				m_WRITECmdsReceived++;
				m_bankStats->s_bankWRITEsRecvd->addData(1);
                                
                                if(m_prevOpenRow == l_row)
                                {
                                    m_bankStats->s_bankRowHits->addData(1);
                                    m_bankStats->s_totalRowHits->addData(1);
                                }else
                                    m_prevOpenRow = m_cmd->getHashedAddress()->getRow();
	
				break;
			case e_BankCommandType::PRE:
				m_PRECmdsReceived++;
				m_bankStats->s_bankPREsRecvd->addData(1);
                                m_prevOpenRow=-1;
				break;
			case e_BankCommandType::REF:
				break;
			default:
				std::cout << "ERR @ " << __PRETTY_FUNCTION__ << std::endl;
				exit(-1);
		}
	}
}

c_BankCommand* c_Bank::clockTic() {


	c_BankCommand* l_resPtr = nullptr;
	if (m_cmd != nullptr) {


		if (m_cmd->isResponseReady()) {
			bool l_doSendRes = false;
			switch (m_cmd->getCommandMnemonic()) {
				case e_BankCommandType::ACT:
					l_doSendRes = k_allocateCmdResQACT;
					m_ACTCmdsSent++;
					break;
				case e_BankCommandType::READ:
					l_doSendRes = k_allocateCmdResQREAD;
					m_READCmdsSent++;
					break;
				case e_BankCommandType::READA:
					l_doSendRes = k_allocateCmdResQREADA;
					m_READCmdsSent++;
					break;
				case e_BankCommandType::WRITE:
					l_doSendRes = k_allocateCmdResQWRITE;
					m_WRITECmdsSent++;
					break;
				case e_BankCommandType::WRITEA:
					l_doSendRes = k_allocateCmdResQWRITEA;
					m_WRITECmdsSent++;
					break;
				case e_BankCommandType::PRE:
					l_doSendRes = k_allocateCmdResQPRE;
					m_PRECmdsSent++;
					break;
				case e_BankCommandType::REF:
					delete m_cmd;
					m_cmd = nullptr;
					return m_cmd;
				default:
					std::cout << "Invalid command type... exiting" << std::endl;
					exit(-1);
			}

			// If allocation knob allows sending the response down the pipe,
			// then forward it. If not, simply decrement the txn's
			// waiting commands and delete the command
			if (l_doSendRes) {


				l_resPtr = m_cmd;
				m_cmd = nullptr;
			} else {

				delete m_cmd;
				m_cmd = nullptr;
			}
		}
	}


	return l_resPtr;
} // c_BankCommand* c_Bank::clockTic()

void c_Bank::acceptStatistics(c_BankStatistics *x_bankStats) {
  assert(m_bankStats == NULL);
  m_bankStats = x_bankStats;
}

void c_Bank::print() {

}
