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


#ifndef C_TXNSCHEDULER_HPP
#define C_TXNSCHEDULER_HPP

#include "c_Transaction.hpp"
#include "c_TxnConverter.hpp"
#include "c_Controller.hpp"


namespace SST {
    namespace n_Bank {
        class c_AddressHasher;
        class c_TxnConverter;
        class c_Controller;

        enum class e_txnSchedulingPolicy {FCFS, FRFCFS};
        typedef std::list<c_Transaction*> TxnQueue;

        class c_TxnScheduler: public SubComponent{
        public:

            SST_ELI_REGISTER_SUBCOMPONENT(
                c_TxnScheduler,
                "CramSim",
                "c_TxnScheduler",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Transaction Scheduler",
                "SST::CramSim::Controller::TxnScheduler"
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"txnSchedulingPolicy", "Transaction scheduling policy", NULL},
                {"numTxnQEntries", "The number of transaction queue entries", NULL},
            )

            SST_ELI_DOCUMENT_PORTS(
            )

            SST_ELI_DOCUMENT_STATISTICS(
            )

            c_TxnScheduler(SST::Component *comp, SST::Params &x_params);
            ~c_TxnScheduler();

            virtual void run();
            virtual bool push(c_Transaction* newTxn);
            virtual bool isHit(c_Transaction* newTxn);


        private:
            virtual c_Transaction* getNextTxn(TxnQueue& x_queue, int x_ch);
            virtual bool hasDependancy(c_Transaction* x_txn, int x_ch);
            virtual void popTxn(TxnQueue& x_queue, c_Transaction* x_txn);

            //**Controller
            c_Controller * m_controller;
            //**transaction converter
            c_TxnConverter* m_txnConverter;
            //**command Scheduler
            c_CmdScheduler* m_cmdScheduler;

            //**per-channel transaction queue
            std::vector<TxnQueue> m_txnQ;      // unified queue
            //**per-channel tranaction queues for read-first scheduling
            std::vector<TxnQueue> m_txnReadQ;  // read queue for read-first scheduling
            std::vector<TxnQueue> m_txnWriteQ; // write queue for read-first scheduling
            unsigned m_maxNumPendingWrite;
            unsigned m_minNumPendingWrite;

            Output *output;
            unsigned m_numChannels;
            bool m_flushWriteQueue;

            //parameters
            e_txnSchedulingPolicy k_txnSchedulingPolicy;
            unsigned k_numTxnQEntries;
            float k_maxPendingWriteThreshold;
            float k_minPendingWriteThreshold;
            bool k_isReadFirstScheduling;

        };
    }
}

#endif //C_TRANSACTIONSCHEDULER_HPP
