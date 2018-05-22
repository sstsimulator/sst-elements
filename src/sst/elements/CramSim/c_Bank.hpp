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
#ifndef C_BANK_HPP
#define C_BANK_HPP

// general C++ includes
#include <memory>
#include <string>
#include <stdint.h>
#include <map>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// CramSim includes
//#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {

  class c_BankCommand;
  class c_BankStatistics;

class c_Bank {
public:

	//FIXME: Will this need a reference to the c_Dimm's link?
	c_Bank(SST::Params& x_params,unsigned x_bankid);
	~c_Bank();

	void print();

	void finish() {
		printf("Bank %u\n", m_bankNum);
		printf("\tTotal ACT-Cmd received: %" PRIu32 "\n", m_ACTCmdsReceived);
		printf("\tTotal READ-Cmd received: %" PRIu32 "\n", m_READCmdsReceived);
		printf("\tTotal WRITE-Cmd received: %" PRIu32 "\n", m_WRITECmdsReceived);
		printf("\tTotal PRE-Cmd received: %" PRIu32 "\n", m_PRECmdsReceived);
		printf("\t\tTotal CMDs received: %" PRIu32 "\n", (m_ACTCmdsReceived + m_READCmdsReceived + m_WRITECmdsReceived + m_PRECmdsReceived));
		// printf("\tTotal ACT-Cmd sent: %" PRIu32 "\n", m_ACTCmdsSent);
		// printf("\tTotal READ-Cmd sent: %" PRIu32 "\n", m_READCmdsSent);
		// printf("\tTotal WRITE-Cmd sent: %" PRIu32 "\n", m_WRITECmdsSent);
		// printf("\tTotal PRE-Cmd sent: %" PRIu32 "\n", m_PRECmdsSent);
		printf("\t\tTotal CMDs sent: %" PRIu32 "\n", (m_ACTCmdsSent + m_READCmdsSent + m_WRITECmdsSent + m_PRECmdsSent));
		printf("Component Finished.\n");
	}

	virtual void handleCommand(c_BankCommand* x_bankCommandPtr);
	virtual c_BankCommand* clockTic(); // called every cycle


	inline unsigned nRC() const {
		return (k_nRC);
	}
	inline unsigned nRRD() const {
		return (k_nRRD);
	}
	inline unsigned nRRD_L() const {
		return (k_nRRD_L);
	}
	inline unsigned nRRD_S() const {
		return (k_nRRD_S);
	}
	inline unsigned nRCD() const {
		return (k_nRCD);
	}
	inline unsigned nCCD() const {
		return (k_nCCD);
	}
	inline unsigned nCCD_L() const {
		return (k_nCCD_L);
	}
	inline unsigned nCCD_L_WR() const {
		return (k_nCCD_L_WR);
	}
	inline unsigned nCCD_S() const {
		return (k_nCCD_S);
	}
	inline unsigned nAL() const {
		return (k_nAL);
	}
	inline unsigned nCL() const {
		return (k_nCL);
	}
	inline unsigned nCWL() const {
		return (k_nCWL);
	}
	inline unsigned nWR() const {
		return (k_nWR);
	}
	inline unsigned nWTR() const {
		return (k_nWTR);
	}
	inline unsigned nWTR_L() const {
		return (k_nWTR_L);
	}
	inline unsigned nWTR_S() const {
		return (k_nWTR_S);
	}
	inline unsigned nRTW() const {
		return (k_nRTW);
	}
	inline unsigned nEWTR() const {
		return (k_nEWTR);
	}
	inline unsigned nERTW() const {
		return (k_nERTW);
	}
	inline unsigned nEWTW() const {
		return (k_nEWTW);
	}
	inline unsigned nERTR() const {
		return (k_nERTR);
	}
	inline unsigned nRAS() const {
		return (k_nRAS);
	}
	inline unsigned nRTP() const {
		return (k_nRTP);
	}
	inline unsigned nRP() const {
		return (k_nRP);
	}
	inline unsigned nRFC() const {
		return (k_nRFC);
	}
	inline unsigned nREFI() const {
		return (k_nREFI);
	}
	inline unsigned nFAW() const {
		return (k_nFAW);
	}
	inline unsigned nBL() const {
		return (k_nBL);
	}

  unsigned getBankNum() const {return m_bankNum;}
	void setBankNum(unsigned x_bankNum){m_bankNum=x_bankNum;}
  void acceptStatistics(c_BankStatistics *x_bankStats);
  
private:
	c_Bank(); // for serialization only
	c_Bank(const c_Bank&); // do not implement
	void operator=(const c_Bank&); // do not implement


	bool k_allocateCmdResQACT;
	bool k_allocateCmdResQREAD;
	bool k_allocateCmdResQREADA;
	bool k_allocateCmdResQWRITE;
	bool k_allocateCmdResQWRITEA;
	bool k_allocateCmdResQPRE;

	// tech params
	unsigned k_nRC;
	unsigned k_nRRD;
	unsigned k_nRRD_L;
	unsigned k_nRRD_S;
	unsigned k_nRCD;
	unsigned k_nCCD;
	unsigned k_nCCD_L;
	unsigned k_nCCD_L_WR;
	unsigned k_nCCD_S;
	unsigned k_nAL;
	unsigned k_nCL;
	unsigned k_nCWL;
	unsigned k_nWR;
	unsigned k_nWTR;
	unsigned k_nWTR_L;
	unsigned k_nWTR_S;
	unsigned k_nRTW;
	unsigned k_nEWTR;
	unsigned k_nERTW;
	unsigned k_nEWTW;
	unsigned k_nERTR;
	unsigned k_nRAS;
	unsigned k_nRTP;
	unsigned k_nRP;
	unsigned k_nRFC;
	unsigned k_nREFI;
	unsigned k_nFAW;
	unsigned k_nBL;

	// bank organization params
	unsigned k_numRows;
	unsigned k_numCols;

	// internal microarcitecture
	c_BankCommand* m_cmd;

	static unsigned k_banks;
	unsigned m_bankNum;

	unsigned m_ACTCmdsReceived;
	unsigned m_READCmdsReceived;
	unsigned m_WRITECmdsReceived;
	unsigned m_PRECmdsReceived;

	unsigned m_ACTCmdsSent;
	unsigned m_READCmdsSent;
	unsigned m_WRITECmdsSent;
	unsigned m_PRECmdsSent;

        uint32_t m_prevOpenRow;

  // Statistics
  c_BankStatistics *m_bankStats;
};
} // n_Bank
} // namespace SST

#endif // C_BANK_HPP
