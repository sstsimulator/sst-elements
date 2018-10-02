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

#ifndef C_TXNDISPATCHER_HPP
#define C_TXNDISPATCHER_HPP
//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/elementinfo.h>
#include "c_Transaction.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"

namespace SST{
    namespace n_Bank{

         class c_TxnDispatcher : public SST::Component {
         public:

             SST_ELI_REGISTER_COMPONENT(
                c_TxnDispatcher,
                "CramSim",
                "c_TxnDispatcher",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Transaction dispatcher",
                COMPONENT_CATEGORY_UNCATEGORIZED
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"numLanes", "Total number of lanes", NULL},
                {"laneIdxPosition", "Bit posiiton of the lane index in the address.. [End:Start]", NULL},
            )

            SST_ELI_DOCUMENT_PORTS(
                { "txnGen", "link to/from a transaction generator", {"MemEvent"} },
                { "lane_%(lanes)d", "link to/from lanes",  {"MemEvent"} },
            )

             c_TxnDispatcher( ComponentId_t id, Params& params);
             ~c_TxnDispatcher();

         private:
             c_TxnDispatcher();

             virtual bool clockTic(Cycle_t);
             void handleTxnGenEvent(SST::Event *ev);
             void handleCtrlEvent(SST::Event *ev);
             void sendRequest(c_TxnReqEvent *ev);
             void sendResponse(c_TxnResEvent *ev);

             uint32_t getLaneIdx(uint64_t x_addr);

             SimTime_t m_simCycle;

             SST::Link *m_txnGenLink;
             std::vector<SST::Link*> m_laneLinks;

             std::deque<c_TxnReqEvent*> m_reqQ;
             std::deque<c_TxnResEvent*> m_resQ;

             uint32_t m_laneIdxStart;
             uint32_t m_laneIdxEnd;
             uint64_t m_laneIdxMask;

             uint32_t k_numLanes;
             Output dbg;
         };
    }
}

#endif //C_TXNDISPATCHER_HPP
