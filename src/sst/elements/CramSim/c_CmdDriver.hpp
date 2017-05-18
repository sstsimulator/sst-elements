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

#ifndef _CMDDRIVER_H
#define _CMDDRIVER_H

#include <stdint.h>
#include <queue>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_Transaction.hpp"

namespace SST {
namespace n_CmdDriver {

class c_CmdDriver: public SST::Component {

public:
	c_CmdDriver(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_CmdDriver();

	void setup() {
	}
	void finish() {
		printf("Component Finished.\n");
	}

private:
	c_CmdDriver(); //for serialization only
	c_CmdDriver(const c_CmdDriver&); //do not implement
	void operator=(const c_CmdDriver&);

	// cmd to/from events
	void handleInTxnUnitReqPtrPkgEvent(SST::Event *ev); // receive cmd req ptr from TxnUnit
	void handleOutCmdDrvResPtrEvent(SST::Event *ev); // we do not need this function for functionality

	// token chg to/from events
	void handleOutCmdDrvReqQTokenChgEvent(SST::Event *ev); // we do not need this function for functionality


	void sendResponse();
	void sendTokenChg(); //should happen at the end of every cycle

	virtual bool clockTic(SST::Cycle_t); //called every cycle


	//request-related links
	SST::Link* m_inTxnUnitReqPtrLink; //incoming txn unit req ptr
	SST::Link* m_outCmdDrvReqQTokenChgLink; //outgoing change in cmd driver req q tokens

	//response links
	SST::Link* m_outCmdDrvResPtrLink; //outgoing cmd drv res ptr

	//params for internal microarcitecture
	int k_cmdDrvBufferQEntries;
	int m_thisCycleQTknChg;


	std::queue<SST::n_Bank::c_BankCommand*> m_cmdQ;
};

} // namespace n_CmdDriver
} // namespace SST

#endif  /* _CMDDRIVER_H */
