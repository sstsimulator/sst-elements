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

// Copyright 2016 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef C_CMDSCHEDULER_HPP
#define C_CMDSCHEDULER_HPP

#include "c_BankCommand.hpp"
#include "c_DeviceDriver.hpp"
#include "c_HashedAddress.hpp"
#include "c_Controller.hpp"

namespace SST{
    namespace n_Bank {
        class c_DeviceDriver;
        class c_Controller;

        class c_CmdScheduler : public SubComponent{
        public:

            SST_ELI_REGISTER_SUBCOMPONENT(
                c_CmdScheduler,
                "CramSim",
                "c_CmdScheduler",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Command Scheduler",
                "SST::CramSim::Controller::CmdScheduler"
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"numCmdQEntries", "The number of entries in command scheduler's command queue"},
            )

            SST_ELI_DOCUMENT_PORTS(
            )

            SST_ELI_DOCUMENT_STATISTICS(
            )

            c_CmdScheduler(Component *comp, Params &x_params);
            ~c_CmdScheduler();

            void run();
            bool push(c_BankCommand* x_cmd);
            unsigned getToken(const c_HashedAddress &x_addr);


        private:
            enum e_SchedulingPolicy {BANK, RANK};
            typedef std::deque<c_BankCommand*> c_CmdQueue;

            c_Controller* m_owner;
            c_DeviceDriver* m_deviceController;

            std::vector<std::vector<c_CmdQueue>> m_cmdQueues;  //per-bank command queue for each channel
            std::vector<unsigned> m_nextCmdQIdx;                //index for command queue scheduling (Round Robin)

            Output* output;
            unsigned m_numBanks;
            unsigned m_numChannels;
            unsigned m_numRanksPerChannel;
            unsigned m_numBanksPerChannel;
            unsigned m_numBanksPerRank;
            unsigned k_numCmdQEntries;

            enum e_SchedulingPolicy m_schedulingPolicy;
        };
    }
}
#endif //SRC_C_CMDSCHEDULER_HPP
