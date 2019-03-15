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

#ifndef _MEMHBRIDGE_H
#define _MEMHBRIDGE_H

#include <stdint.h>
#include <queue>
#include <iostream>
#include <fstream>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

//local includes
#include "c_Transaction.hpp"
#include "c_TxnGen.hpp"

namespace SST {
namespace n_Bank {
    
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
    
class c_MemhBridge: public c_TxnGenBase {


public:

    SST_ELI_REGISTER_COMPONENT(
        c_MemhBridge,
        "CramSim",
        "c_MemhBridge",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Bridge to communicate with MemoryHierarchy",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
    )

    SST_ELI_DOCUMENT_PORTS(
        { "cpuLink", "link to/from CPU", {"c_CPUevent"} },
        { "memLink", "link to memory-side components (txn dispatcher or controller)", { "c_TxnReqEvent", "c_TxnResEvent"} },
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"readTxnsSent", "Number of read transactions sent", "reads", 1}, // Name, Desc, Units, Enable Level
        {"writeTxnsSent", "Number of write transactions sent", "writes", 1}, // Name, Desc, Units, Enable Level
        {"readTxnsCompleted", "Number of read transactions completed", "reads", 1}, // Name, Desc, Units, Enable Level
        {"writeTxnsCompleted", "Number of write transactions completed", "writes", 1},
        {"txnsPerCycle", "Transactions Per Cycle", "Txns/Cycle", 1},
        {"readTxnsLatency", "Average latency of read transactions", "cycles", 1},
        {"writeTxnsLatency", "Average latency of write transactions", "cycles", 1},
        {"txnsLatency", "Average latency of (read/write) transactions", "cycles", 1},
    )

	c_MemhBridge(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_MemhBridge();


private:
	void createTxn();
	void readResponse(); //read from res q to output

        void printTxn(bool isWrite, uint64_t addr);
	
        //Debug
	Output *output;


	//link to/from CPU
	SST::Link *m_linkCPU;


	bool k_printTxnTrace;
	std::string k_txnTraceFileName;
	std::filebuf m_txnTraceFileBuf;
	std::streambuf *m_txnTraceStreamBuf;
	std::ofstream m_txnTraceOFStream;
	std::ostream *m_txnTraceStream;


};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} // namespace n_Bank
} // namespace SST

#endif  /* _TXNGENRAND_H */
