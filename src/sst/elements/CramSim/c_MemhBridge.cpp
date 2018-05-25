// Copyright 2009-2016 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//SST includes
#include "sst_config.h"

#include <assert.h>
#include <iostream>


//local includes
#include "c_MemhBridge.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "memReqEvent.hpp"
#include "c_TxnGen.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace CramSim;

c_MemhBridge::c_MemhBridge(ComponentId_t x_id, Params& x_params) :
	c_TxnGenBase(x_id, x_params) {

	/*---- LOAD PARAMS ----*/

	//used for reading params
	bool l_found = false;

	// internal params
	int verbosity = x_params.find<int>("verbose", 0);
	output = new SST::Output("CramSim.memhbridge[@f:@l:@p] ",
							 verbosity, 0, SST::Output::STDOUT);

	// set up txn trace output
	k_printTxnTrace = (uint32_t) x_params.find<uint32_t>("boolPrintTxnTrace", 0, l_found);

	k_txnTraceFileName = (std::string) x_params.find<std::string>("strTxnTraceFile", "-", l_found);
	//k_txnTraceFileName.pop_back(); // remove trailing newline (??)
	if (k_printTxnTrace) {
		if (k_txnTraceFileName.compare("-") == 0) {// set output to std::cout
			std::cout << "Setting txn trace output to std::cout" << std::endl;
			m_txnTraceStreamBuf = std::cout.rdbuf();
		} else { // open the file and direct the txnTraceStream to it
			std::cout << "Setting txn trace output to " << k_txnTraceFileName << std::endl;
			m_txnTraceOFStream.open(k_txnTraceFileName);
			if (m_txnTraceOFStream) {
				m_txnTraceStreamBuf = m_txnTraceOFStream.rdbuf();
			} else {
				std::cerr << "Failed to open txn trace output file " << k_txnTraceFileName << ", redirecting to stdout";
				m_txnTraceStreamBuf = std::cout.rdbuf();
			}
		}
		m_txnTraceStream = new std::ostream(m_txnTraceStreamBuf);
	}



	/*---- CONFIGURE LINKS ----*/

	// CPU links
	m_linkCPU = configureLink( "cpuLink");

}

c_MemhBridge::~c_MemhBridge() {
}



void c_MemhBridge::createTxn() {
   
    uint64_t l_cycle = m_simCycle;
    
    SST::Event* e = 0;
    while((e = m_linkCPU->recv())) {
        MemReqEvent *event = dynamic_cast<MemReqEvent *>(e);

        c_Transaction *mTxn;
        ulong addr = event->getAddr();

        if (event->getIsWrite())
                mTxn = new c_Transaction(event->getReqId(), e_TransactionType::WRITE, addr, 1);
        else
                mTxn = new c_Transaction(event->getReqId(), e_TransactionType::READ, addr, 1);

        
        std::pair<c_Transaction*, uint64_t > l_entry = std::make_pair(mTxn,l_cycle);
        m_txnReqQ.push_back(l_entry);
     
        if(k_printTxnTrace)
            printTxn(event->getIsWrite(),addr);
    }
}


void c_MemhBridge::readResponse() {
	if (m_txnResQ.size() > 0) {
                c_Transaction* l_txn = m_txnResQ.front();
                
		MemRespEvent *event = new MemRespEvent(l_txn->getSeqNum(), l_txn->getAddress(), 0);

		l_txn->print(output,"[memhbridge.readResponse]", m_simCycle);

		m_linkCPU->send( event );
		delete l_txn;

                m_txnResQ.pop_front();
	}
}


void c_MemhBridge::printTxn(bool x_isWrite, uint64_t x_addr){
    std::string l_txnType;

    uint64_t l_currentCycle = m_simCycle;
    if(x_isWrite)
        l_txnType="P_MEM_WR";
    else
        l_txnType="P_MEM_RD";

    (*m_txnTraceStream) << "0x" << std::hex <<x_addr
                        << " " <<l_txnType
                        << " " << std::dec<<l_currentCycle
                        <<std::endl;
		
}

// Element Libarary / Serialization stuff
