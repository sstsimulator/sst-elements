
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

#ifndef C_TRACEFILEREADER_HPP
#define C_TRACEFILEREADER_HPP

#include <stdint.h>
#include <queue>
#include <iostream>
#include <string>
#include <fstream>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/elementinfo.h>

//local includes
#include "c_Transaction.hpp"
#include "c_TxnGen.hpp"


namespace SST {
    namespace n_Bank {
        class c_TraceFileReader: public c_TxnGenBase {
        public:

            SST_ELI_REGISTER_COMPONENT(
                c_TraceFileReader,
                "CramSim",
                "c_TraceFileReader",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Test Trace file Generator",
                COMPONENT_CATEGORY_UNCATEGORIZED
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
                {"numTxnPerCycle", "The number of transactions generated per cycle", NULL},
                {"traceFile", "Location of trace file to read", NULL},
                {"traceFileType", "Trace file type (DEFAULT or USIMM)",NULL},
            )

            SST_ELI_DOCUMENT_PORTS(
                { "memLink", "link to memory-side components (txn dispatcher or controller)", {"c_TxnReqEvent", "c_TxnResEvent"} },
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

            c_TraceFileReader(SST::ComponentId_t x_id, SST::Params& x_params);
            ~c_TraceFileReader(){}
        private:
            enum e_TracefileType{
                DEFAULT,   //DRAMsim2 type
                USIMM
            };
            virtual void createTxn();

            //params for internal microarcitecture
            std::string m_traceFileName;
            std::ifstream *m_traceFileStream;

            e_TracefileType m_traceType;
        };
    }
}

#endif //SRC_C_TRACEFILEREADER_HPP
