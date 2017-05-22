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

c_Bank::c_Bank(SST::Params& x_params) {
	// read params here

/*	k_nRC = (uint32_t)x_params.find<uint32_t>("nRC", 55, l_found);
	if (!l_found) {
		std::cout << "nRC value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRRD = (uint32_t)x_params.find<uint32_t>("nRRD", 4, l_found);
	if (!l_found) {
		std::cout << "nRRD value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRRD_L = (uint32_t)x_params.find<uint32_t>("nRRD_L", 6, l_found);
	if (!l_found) {
		std::cout << "nRRD_L value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRRD_S = (uint32_t)x_params.find<uint32_t>("nRRD_S", 4, l_found);
	if (!l_found) {
		std::cout << "nRRD_S value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRCD = (uint32_t)x_params.find<uint32_t>("nRCD", 16, l_found);
	if (!l_found) {
		std::cout << "nRRD_L value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCCD = (uint32_t)x_params.find<uint32_t>("nCCD", 4, l_found);
	if (!l_found) {
		std::cout << "nCCD value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCCD_L = (uint32_t)x_params.find<uint32_t>("nCCD_L", 5, l_found);
	if (!l_found) {
		std::cout << "nCCD_L value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCCD_L_WR = (uint32_t)x_params.find<uint32_t>("nCCD_L_WR", 1, l_found);
	if (!l_found) {
		std::cout << "nCCD_L_WR value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCCD_S = (uint32_t)x_params.find<uint32_t>("nCCD_S", 4, l_found);
	if (!l_found) {
		std::cout << "nCCD_S value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nAL = (uint32_t)x_params.find<uint32_t>("nAL", 15, l_found);
	if (!l_found) {
		std::cout << "nAL value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCL = (uint32_t)x_params.find<uint32_t>("nCL", 16, l_found);
	if (!l_found) {
		std::cout << "nCL value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nCWL = (uint32_t)x_params.find<uint32_t>("nCWL", 16, l_found);
	if (!l_found) {
		std::cout << "nCWL value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nWR = (uint32_t)x_params.find<uint32_t>("nWR", 16, l_found);
	if (!l_found) {
		std::cout << "nWR value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nWTR = (uint32_t)x_params.find<uint32_t>("nWTR", 3, l_found);
	if (!l_found) {
		std::cout << "nWTR value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nWTR_L = (uint32_t)x_params.find<uint32_t>("nWTR_L", 9, l_found);
	if (!l_found) {
		std::cout << "nWTR_L value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nWTR_S = (uint32_t)x_params.find<uint32_t>("nWTR_S", 3, l_found);
	if (!l_found) {
		std::cout << "nWTR_S value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRTW = (uint32_t)x_params.find<uint32_t>("nRTW", 4, l_found);
	if (!l_found) {
		std::cout << "nRTW value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nEWTR = (uint32_t)x_params.find<uint32_t>("nEWTR", 6, l_found);
	if (!l_found) {
		std::cout << "nEWTR value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nERTW = (uint32_t)x_params.find<uint32_t>("nERTW", 6, l_found);
	if (!l_found) {
		std::cout << "nERTW value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nEWTW = (uint32_t)x_params.find<uint32_t>("nEWTW", 6, l_found);
	if (!l_found) {
		std::cout << "nEWTW value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nERTR = (uint32_t)x_params.find<uint32_t>("nERTR", 6, l_found);
	if (!l_found) {
		std::cout << "nERTR value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRAS = (uint32_t)x_params.find<uint32_t>("nRAS", 39, l_found);
	if (!l_found) {
		std::cout << "nRAS value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRTP = (uint32_t)x_params.find<uint32_t>("nRTP", 9, l_found);
	if (!l_found) {
		std::cout << "nRTP value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRP = (uint32_t)x_params.find<uint32_t>("nRP", 16, l_found);
	if (!l_found) {
		std::cout << "nRP value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nRFC = (uint32_t)x_params.find<uint32_t>("nRFC", 420, l_found);
	if (!l_found) {
		std::cout << "nRFC value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nREFI = (uint32_t)x_params.find<uint32_t>("nREFI", 9360, l_found);
	if (!l_found) {
		std::cout << "nREFI value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nFAW = (uint32_t)x_params.find<uint32_t>("nFAW", 16, l_found);
	if (!l_found) {
		std::cout << "nFAW value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_nBL = (uint32_t)x_params.find<uint32_t>("nBL", 4, l_found);
	if (!l_found) {
		std::cout << "nBL value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_numRows = (uint32_t)x_params.find<uint32_t>("numRows", 16384, l_found);
	if (!l_found) {
		std::cout << "numRows value is missing ... exiting" << std::endl;
		exit(-1);
	}

	k_numCols = (uint32_t)x_params.find<uint32_t>("numCols", 128, l_found);
	if (!l_found) {
		std::cout << "numCols value is missing ... exiting" << std::endl;
		exit(-1);
	}*/
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

	m_bankNum = k_banks;
	k_banks++;


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

  //std::cout << std::endl << "@" << std::dec
  //<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
  //<< __PRETTY_FUNCTION__ << " " << this << std::endl;
  //std::cout << std::endl;

  //std::cout << "bankNum " << m_bankNum << std::endl;
  if(m_cmd) {
    std::cout << "m_cmd = ";       m_cmd->print();
    std::cout << "x_cmd = ";       x_bankCommandPtr->print();
  }

	assert(nullptr == m_cmd);

	if (nullptr == m_cmd) {

//		std::cout << "Bank accepted this command ";
//		x_bankCommandPtr->print();

		m_cmd = x_bankCommandPtr;

		switch (x_bankCommandPtr->getCommandMnemonic()){
			case e_BankCommandType::ACT:
				m_ACTCmdsReceived++;
				m_bankStats->s_bankACTsRecvd->addData(1);
				break;
			case e_BankCommandType::READ:
			case e_BankCommandType::READA:
				m_READCmdsReceived++;
				m_bankStats->s_bankREADsRecvd->addData(1);
				break;
			case e_BankCommandType::WRITE:
			case e_BankCommandType::WRITEA:
				m_WRITECmdsReceived++;
				m_bankStats->s_bankWRITEsRecvd->addData(1);
				break;
			case e_BankCommandType::PRE:
				m_PRECmdsReceived++;
				m_bankStats->s_bankPREsRecvd->addData(1);
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

//	std::cout << std::endl << "@" << std::dec
//			<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
//			<< __PRETTY_FUNCTION__ << std::endl;
//	std::cout << "m_cmd = " << m_cmd << std::endl;
//	if (nullptr != m_cmd)
//		m_cmd->print();

	c_BankCommand* l_resPtr = nullptr;
	if (m_cmd != nullptr) {

//		m_cmd->print();
//		std::cout<<std::endl;

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

			        //std::cout << __PRETTY_FUNCTION__ << ": Sending cmd:"
				//		<< std::endl;
				//m_cmd->print();
				//std::cout << std::endl;

				l_resPtr = m_cmd;
				m_cmd = nullptr;
			} else {

//				std::cout << __PRETTY_FUNCTION__ << ": Destroying Cmd"
//						<< std::endl;
//				m_cmd->print();
//				std::cout << std::endl;

			        //c_Transaction* l_txnPtr = m_cmd->getTransaction();
				//const unsigned l_cmdsLeft = l_txnPtr->getWaitingCommands() - 1;
				//l_txnPtr->setWaitingCommands(l_cmdsLeft);
				//if (l_cmdsLeft == 0)
				//	l_txnPtr->setResponseReady();

				delete m_cmd;
				m_cmd = nullptr;
			}
		}
	}

//	if (nullptr != l_resPtr) {
//		std::cout << "Returning ptr l_resPtr = ";
//		l_resPtr->print();
//	}

	return l_resPtr;
} // c_BankCommand* c_Bank::clockTic()

void c_Bank::acceptStatistics(c_BankStatistics *x_bankStats) {
  assert(m_bankStats == NULL);
  m_bankStats = x_bankStats;
}

void c_Bank::print() {

}
