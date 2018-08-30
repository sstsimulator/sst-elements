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

	m_cmd = nullptr;
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
					m_ACTCmdsSent++;
					break;
				case e_BankCommandType::READ:
					l_doSendRes = true;
					m_READCmdsSent++;
					break;
				case e_BankCommandType::READA:
					l_doSendRes = true;
					m_READCmdsSent++;
					break;
				case e_BankCommandType::WRITE:
					l_doSendRes = true;
					m_WRITECmdsSent++;
					break;
				case e_BankCommandType::WRITEA:
					l_doSendRes = true;
					m_WRITECmdsSent++;
					break;
				case e_BankCommandType::PRE:
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
