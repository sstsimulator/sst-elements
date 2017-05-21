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

#ifndef _TXNDRIVER_H
#define _TXNDRIVER_H

#include <stdint.h>
#include <queue>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_Transaction.hpp"

namespace SST {
namespace n_TxnDriver {

class c_TxnDriver: public SST::Component {

public:
	c_TxnDriver(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_TxnDriver();

	void setup() {
	}
	void finish() {
		printf("Component Finished.\n");
	}

private:
	c_TxnDriver(); //for serialization only
	c_TxnDriver(const c_TxnDriver&); //do not implement
	void operator=(const c_TxnDriver&);

	//txn to/from events
	void handleInTxnGenReqPtrEvent(SST::Event *ev); //receive txn req ptr from Transaction gen
	void handleOutTxnDrvResPtrEvent(SST::Event *ev); // we do not need this function for functionality

	//token chg to/from events
	void handleInTxnGenResQTokenChgEvent(SST::Event *ev); //receive change in tokens in txn gen res queue
	void handleOutTxnDrvReqQTokenChgEvent(SST::Event *ev); // we do not need this function for functionality

	void sendResponse();
	void sendTokenChg(); //should happen at the end of every cycle

	virtual bool clockTic(SST::Cycle_t); //called every cycle


	//request-related links
	SST::Link* m_inTxnGenReqPtrLink; //incoming txn gen req ptr
	SST::Link* m_outTxnDrvReqQTokenChgLink; //outgoing change in txn driver req q tokens

	//response links
	SST::Link* m_inTxnGenResQTokenChgLink; //incoming change in txn gen res q tokens
	SST::Link* m_outTxnDrvResPtrLink; //outgoing txn drv res ptr

	//params for internal microarcitecture
	int k_txnDrvBufferQEntries;
	int m_thisCycleQTknChg;

	//param for gen
	int k_txnGenResQEntries;

	//token changes from Txn gen
	int m_txnGenResQTokens;

	std::queue<SST::n_Bank::c_Transaction*> m_txnQ;
};

} // namespace n_TxnDriver
} // namespace SST

#endif  /* _TXNDRIVER_H */
