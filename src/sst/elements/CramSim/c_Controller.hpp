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

#ifndef C_CONTROLLER_HPP
#define C_CONTROLLER_HPP


// SST includes
#include <sst/core/component.h>
#include "c_AddressHasher.hpp"
#include "c_CmdUnit.hpp"
#include "c_TxnUnit.hpp"

// local includes
namespace SST {
    namespace n_Bank {
        class c_Controller : public SST::Component {

        public:
            c_Controller(ComponentId_t id, Params &params);
            ~c_Controller();

            void finish();
            c_AddressHasher* getAddrHasher(){return m_addrHasher;}


        private:
            SST::Output *output;

            std::vector<c_Transaction*> m_txnReqQ;
            std::vector<c_Transaction*> m_txnResQ;

            // Subcomponents
            c_TxnUnit *m_transConverter;
           // c_CommandScheduler *m_cmdScheduler;
            c_AddressHasher *m_addrHasher;
            c_CmdUnit *m_deviceController;
            //c_DeviceController *m_deviceController;

            //token changes from Txn gen
            int m_txnGenResQTokens;
            int m_deviceReqQTokens;
            int m_thisCycleReqQTknChg;

            // params for internal architecture
            int k_txnReqQEntries;
            int k_txnResQEntries;
            int k_txnGenResQEntries;



            // Transaction Generator <-> CmdUnit Links
            SST::Link *m_inTxnGenReqPtrLink;
            SST::Link *m_outTxnGenResPtrLink;
            SST::Link *m_outDeviceReqPtrLink;
            SST::Link *m_inDeviceResPtrLink;

            SST::Link *m_inDeviceReqQTokenChgLink;
            SST::Link *m_inTxnGenResQTokenChgLink;
            SST::Link *m_outTxnGenReqQTokenChgLink;

            void configure_link();
            virtual bool clockTic(SST::Cycle_t); // called every cycle

            void sendTokenChg(); // should happen at the end of every cycle
            void sendResponse();
            void sendRequest();

            // Transaction Generator <-> Controller Handlers
            void handleIncomingTransaction(SST::Event *ev);
            void handleOutTxnGenResPtrEvent(SST::Event *ev);
            void handleInTxnGenResQTokenChgEvent(SST::Event *ev);
            void handleOutTxnGenReqQTokenChgEvent(SST::Event *ev);

            // Controller <--> memory devices
            void handleOutgoingCommand(std::vector<c_BankCommand*> issuedCmd);
            void handleOutDeviceReqPtrEvent(SST::Event *ev);
            void handleInDeviceResPtrEvent(SST::Event *ev);
            void handleInDeviceReqQTokenChgEvent(SST::Event *ev);

        };
    }
}


#endif //C_CONTROLLER_HPP
