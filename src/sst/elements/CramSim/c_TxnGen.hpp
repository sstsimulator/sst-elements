
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

#ifndef _TXNGEN_H
#define _TXNGEN_H

#include <stdint.h>
#include <queue>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/elementinfo.h>

//local includes
#include "c_Transaction.hpp"

namespace SST {
    namespace n_Bank {
        class c_TxnGenBase: public SST::Component {

        public:
            c_TxnGenBase (SST::ComponentId_t x_id, SST::Params& x_params);
            ~c_TxnGenBase();

            void setup() {
            }
            void finish();

        protected:
            c_TxnGenBase(); //for serialization only
            virtual void createTxn()=0;
            virtual void handleResEvent(SST::Event *ev); //handleEvent
            virtual bool sendRequest(); //send out txn req ptr to Transaction unit
            virtual void readResponse(); //read from res q to output
            virtual bool clockTic(SST::Cycle_t); //called every cycle

            //Simulation cycles
            SimTime_t m_simCycle;

            //internal microarchitecture
            std::deque<std::pair<c_Transaction*, uint64_t>> m_txnReqQ;
            std::deque<c_Transaction*> m_txnResQ;
            std::map<uint64_t, uint64_t> m_outstandingReqs; //(txn_id, birth time)
            uint32_t m_numOutstandingReqs;
            uint64_t m_numTxns;
            uint32_t m_seqNum;
            uint32_t m_sizeOffset;

            //links
            SST::Link* m_memLink;

            //input parameters
            uint32_t k_numTxnPerCycle;
            uint32_t k_maxOutstandingReqs;
            uint64_t k_maxTxns;

            // used to keep track of the response types being received
            uint64_t m_resReadCount;
            uint64_t m_resWriteCount;
            uint64_t m_reqReadCount;
            uint64_t m_reqWriteCount;


            // Statistics
            Statistic<uint64_t>* s_readTxnSent;
            Statistic<uint64_t>* s_writeTxnSent;
            Statistic<uint64_t>* s_readTxnsCompleted;
            Statistic<uint64_t>* s_writeTxnsCompleted;

            Statistic<double>* s_txnsPerCycle;
            Statistic<uint64_t>* s_readTxnsLatency;
            Statistic<uint64_t>* s_writeTxnsLatency;
            Statistic<uint64_t>* s_txnsLatency;

            // Debug Output
            Output* output;

        };

        class c_TxnGen: public c_TxnGenBase{
        public:

            SST_ELI_REGISTER_COMPONENT(
                c_TxnGen,
                "CramSim",
                "c_TxnGen",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Test Txn Generator",
                COMPONENT_CATEGORY_UNCATEGORIZED
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"maxOutstandingReqs", "Maximum number of the outstanding requests", NULL},
                {"numTxnPerCycle", "The number of transactions generated per cycle", NULL},
                {"readWriteRatio", "Ratio of read txn's to generate : write txn's to generate", NULL},
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

            c_TxnGen (SST::ComponentId_t x_id, SST::Params& x_params);
        private:
            enum e_TxnMode{
                RAND,
                SEQ
            };

            virtual void createTxn();
            uint64_t getNextAddress();

            uint64_t m_prevAddress;
            e_TxnMode m_mode;

            double k_readWriteTxnRatio;
            unsigned int k_randSeed;
        };

    } // namespace n_Bank
} // namespace SST

#endif  /* _TXNGEN_H */
