// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

#include "sst_config.h"
#include "c_TxnDispatcher.hpp"


#include <bitset>

using namespace std;
using namespace SST;
using namespace SST::CramSim;

c_TxnDispatcher::c_TxnDispatcher(ComponentId_t x_id, Params &params):Component(x_id) {
    //*------ get parameters ----*//
    bool l_found=false;

    m_simCycle=0;

    dbg.init("[TxnDispatcher]",params.find<int>("debug_level",0),0,
             (Output::output_location_t)params.find<int>("debug_location",Output::STDOUT));
    output = new Output("", 1, 0, Output::STDOUT);

    k_numLanes= (uint32_t) params.find<uint32_t>("numLanes", 1, l_found);
    if (!l_found) {
        output->fatal(CALL_INFO, -1, "numLanes param value is missing... exiting\n");
    }
    assert(k_numLanes>0);

    string l_laneIdxString = (string) params.find<string>("laneIdxPos", "13:12", l_found);
    if(k_numLanes>1) {
        if (!l_found) {
            output->fatal(CALL_INFO, -1, "the bit position of lane index is not specified... it should be \"end:start\"\n");
        } else {

            stringstream string_stream;
            string_stream.str(l_laneIdxString);
            string item;
            vector<string> strings;
            while (getline(string_stream, item, ':')) {
                strings.push_back(item);
            }

            if (strings.size() != 2) {
                output->fatal(CALL_INFO, -1, "laneIdxPosition error! =>%s\n", l_laneIdxString.c_str());
            } else {
                m_laneIdxEnd = atoi(strings[0].c_str());
                m_laneIdxStart = atoi(strings[1].c_str());
                m_laneIdxMask = ~((int64_t) -1 << (m_laneIdxEnd + 1));
                if (m_laneIdxEnd < m_laneIdxStart) {
                    output->fatal(CALL_INFO, -1, "landIdxPos error!! End position: %d Start position: %d\n",
                            m_laneIdxEnd, m_laneIdxStart);
                }
            }
        }
    }
    std::string l_clockFreqStr = (std::string)params.find<std::string>("ClockFreq", "1GHz", l_found);


    //set our clock
    registerClock(l_clockFreqStr,
                  new Clock::Handler2<c_TxnDispatcher,&c_TxnDispatcher::clockTic>(this));


    //---- configure link ----//
    m_txnGenLink = configureLink("txnGen",new Event::Handler2<c_TxnDispatcher,&c_TxnDispatcher::handleTxnGenEvent>(this));
    if(!m_txnGenLink) {
        output->fatal(CALL_INFO, -1, "txnGen link is not found.. exit\n");
    }

    for (int i = 0; i < k_numLanes; i++) {
        string l_linkName = "lane_" + to_string(i);
        Link *l_link = configureLink(l_linkName,new Event::Handler2<c_TxnDispatcher,&c_TxnDispatcher::handleCtrlEvent>(this));

        if (l_link) {
            m_laneLinks.push_back(l_link);
            output->output("%s is connected\n", l_linkName.c_str());
        } else {
            output->fatal(CALL_INFO, -1, "%s is not found.. exit\n", l_linkName.c_str());
        }
    }

}
c_TxnDispatcher::~c_TxnDispatcher(){}

c_TxnDispatcher::c_TxnDispatcher() :
        Component(-1) {
    // for serialization only
}


bool c_TxnDispatcher::clockTic(Cycle_t clock)
{
    m_simCycle++;

    while(!m_reqQ.empty())
    {
        sendRequest(m_reqQ.front());
        m_reqQ.pop_front();
    }

    while(!m_resQ.empty())
    {
        sendResponse(m_resQ.front());
        m_resQ.pop_front();
    }

    return false;
}


void c_TxnDispatcher::handleTxnGenEvent(SST::Event *ev)
{
    //get a lane index
    c_TxnReqEvent* l_newReq=dynamic_cast<c_TxnReqEvent*>(ev);
    m_reqQ.push_back(l_newReq);

    #ifdef __SST_DEBUG_OUTPUT__
    l_newReq->m_payload->print(&dbg,"[c_TxnDispatcher.handleTxnGenEvent]",m_simCycle);
    #endif
}


void c_TxnDispatcher::handleCtrlEvent(SST::Event *ev)
{
    c_TxnResEvent* l_newRes=dynamic_cast<c_TxnResEvent*>(ev);
    m_resQ.push_back(l_newRes);
}


uint32_t c_TxnDispatcher::getLaneIdx(uint64_t x_addr)
{
    if(k_numLanes==1)
        return 0;
    else
        return (m_laneIdxMask & x_addr) >> m_laneIdxStart;
}


void c_TxnDispatcher::sendRequest(c_TxnReqEvent* x_newReq)
{
    uint64_t l_addr = x_newReq->m_payload->getAddress();
    uint32_t l_laneIdx = getLaneIdx(l_addr);

    assert(l_laneIdx < m_laneLinks.size());

    #ifdef __SST_DEBUG_OUTPUT__
    dbg.verbose(CALL_INFO,1,0," Cycle:%" PRIu64 ", LaneIdxPosition:[%d:%d] Addr: 0x%" PRIx64 " LaneIdx: %" PRIu32 "\n",
             m_simCycle, m_laneIdxEnd, m_laneIdxStart, l_addr, l_laneIdx);
    #endif

    //send the event to a target lane
    m_laneLinks[l_laneIdx]->send(x_newReq);
}


void c_TxnDispatcher::sendResponse(c_TxnResEvent* x_newRes)
{
    m_txnGenLink->send(x_newRes);
}
