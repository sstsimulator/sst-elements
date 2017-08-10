
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

#ifndef _TXNGEN_H
#define _TXNGEN_H

#include <stdint.h>
#include <queue>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

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
            void finish() {
                printf("Total Read-Txns Responses received: %lu\n", m_resReadCount);
                printf("Total Write-Txns Responses received: %lu\n", m_resWriteCount);
                printf("Total Txns Received: %lu\n", m_resReadCount + m_resWriteCount);
                std::cout << "Cycles Per Transaction (CPT) = "
                          << std::dec << static_cast<double>(Simulation::getSimulation()->getCurrentSimCycle())
                                         / static_cast<double>(m_resReadCount + m_resWriteCount) << std::endl;
                printf("Component Finished.\n");
            }

        protected:
            c_TxnGenBase(); //for serialization only
            virtual void createTxn()=0;

            //token chg to/from events
            void handleResEvent(SST::Event *ev); //handleEvent

            bool sendRequest(); //send out txn req ptr to Transaction unit
            void readResponse(); //read from res q to output

            virtual bool clockTic(SST::Cycle_t); //called every cycle

            std::deque<std::pair<c_Transaction*, uint64_t>> m_txnReqQ;
            uint32_t m_numOutstandingReqs;
            uint64_t m_numTxns;
            //Transaction info
            uint32_t m_seqNum;

            uint32_t m_mode;


            //links
            SST::Link* m_lowLink; //incoming txn unit res ptr

            //params for internal microarcitecture
            uint32_t k_numTxnPerCycle;
            uint32_t k_maxOutstandingReqs;
            uint64_t k_maxTxns;

            // used to keep track of the response types being received
            uint64_t m_resReadCount;
            uint64_t m_resWriteCount;

            uint64_t m_reqReadCount;
            uint64_t m_reqWriteCount;


            // Statistics
            Statistic<uint64_t>* s_readTxnsCompleted;
            Statistic<uint64_t>* s_writeTxnsCompleted;

        };

        class c_TxnGen: public c_TxnGenBase{
        public:
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