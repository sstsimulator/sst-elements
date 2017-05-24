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

#ifndef _TXNGENRAND_H
#define _TXNGENRAND_H

#include <stdint.h>
#include <queue>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

//local includes
#include "c_Transaction.hpp"

namespace SST {
namespace n_Bank {
class c_TxnGenRand: public SST::Component {

public:
	c_TxnGenRand(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_TxnGenRand();

	void setup() {
	}
	void finish() {
		printf("Total Read-Txns Responses received: %lu\n", m_resReadCount);
		printf("Total Write-Txns Responses received: %lu\n", m_resWriteCount);
		printf("Component Finished.\n");
	}

private:
	c_TxnGenRand(); //for serialization only
	c_TxnGenRand(const c_TxnGenRand&); //do not implement
	void operator=(const c_TxnGenRand&);

	uint64_t getNextAddress();
	void createTxn();

	//FIXME: Remove the word unit from members once class is complete to keep in line with code convention
	//txn to/from events
	void handleOutTxnGenReqPtrEvent(SST::Event *ev); // we do not need this function for functionality
	void handleInTxnUnitResPtrEvent(SST::Event *ev); //receive txn res ptr from Transaction unit

	//token chg to/from events
	void handleInTxnUnitReqQTokenChgEvent(SST::Event *ev); //receive change in tokens in txn unit req queue
	void handleOutTxnGenResQTokenChgEvent(SST::Event *ev); // we do not need this function for functionality

	void sendTokenChg(); //should happen at the end of every cycle
	void sendRequest(); //send out txn req ptr to Transaction unit
	void readResponse(); //read from res q to output

	virtual bool clockTic(SST::Cycle_t); //called every cycle

	//Transaction info
	uint64_t m_prevAddress;
	ulong m_seqNum;

	//request-related links
	SST::Link* m_outTxnGenReqPtrLink; //outgoing txn gen req ptr
	SST::Link* m_inTxnUnitReqQTokenChgLink; //incoming change in txn unit req q tokens

	//response links
	SST::Link* m_inTxnUnitResPtrLink; //incoming txn unit res ptr
	SST::Link* m_outTxnGenResQTokenChgLink; //outgoing change in txn gen res q tokens

	//params for internal microarcitecture
	int k_txnGenReqQEntries;
	int k_txnGenResQEntries;
	double k_readWriteTxnRatio;
        unsigned int k_randSeed;

	//param for receiver
	int k_txnUnitReqQEntries;

	//TODO: implement txn width if necessary

	// token change in this unit this cycle
	// beginning of every cycle this variable is reset
	// sendTokenChg() function sends the contents of this variable
	int m_thisCycleResQTknChg;

	//token changes from Txn unit
	int m_txnUnitReqQTokens;

	// used to keep track of the response types being received
	unsigned long m_resReadCount;
	unsigned long m_resWriteCount;

	std::queue<c_Transaction*> m_txnReqQ; //outgoing
	std::queue<c_Transaction*> m_txnResQ; //incoming

  // Statistics
  Statistic<uint64_t>* s_readTxnsCompleted;
  Statistic<uint64_t>* s_writeTxnsCompleted;

};

} // namespace n_Bank
} // namespace SST

#endif  /* _TXNGENRAND_H */
