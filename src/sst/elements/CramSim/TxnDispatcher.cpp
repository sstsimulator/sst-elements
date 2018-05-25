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

#include "sst_config.h"
#include "TxnDispatcher.hpp"

using namespace std;
using namespace SST;
using namespace SST::n_Bank;

c_TxnDispatcher::c_TxnDispatcher(ComponentId_t id, Params &params) {
    //*------ get parameters ----*//
    uint32_t k_numLanes= (uint32_t) params.find<uint32_t>("numLanes", 1, l_found);
    if (!l_found) {
        std::cout << "numLanes param value is missing... exiting"
                  << std::endl;
        exit(-1);
    }
    std::string l_laneIdxString = (std::string) params.find<std::string>("laneIdxPosition", "[13:12]", l_found);
    if (!l_found) {
        std::cout << "the bit position of lane index is not specified... it should be [end:start]"
                  << std::endl;
        exit(-1);
    }
    else
    {
        l_laneIdxString.erase(remove(l_laneIdxString.begin(), l_laneIdxstring.end(), '['), l_laneIdxString.end());
        l_laneIdxString.erase(remove(l_laneIdxString.begin(), l_laneIdxstring.end(), ']'), l_laneIdxString.end());
        l_laneIdxString.erase(remove(l_laneIdxString.begin(), l_laneIdxstring.end(), '\n'), l_laneIdxString.end());
        l_laneIdxString.erase(remove(l_laneIdxString.begin(), l_laneIdxstring.end(), ' '), l_laneIdxString.end());
        string s;
        vector<string> strings;
        while(getline(l_laneIdxString,s,','))
        {
            count << s <<endl;
            strings.push_back(s);
        }
        if(strings.size()!=2)
        {
            std::cout<<"laneIdxPosition error! =>"<<l_laneIdxString<<std::end;
            exit(-1);
        }
        else {
            m_laneIdxStart = strings[0].to_Integer();
            m_laneIdxEnd = strings[1].to_Integer();
        }
    }


    //*---- configure link ----*//
    m_txnGenLink = configureLink("txnGen",new Event::Handler<c_TxnDispatcher>(this,&c_TxnDispatcher::handleTxnGenEvent));
    if(!l_link) {
        std::cout<<"txnGen link is not found.. exit";
        exit(-1);
    }

    for (int i = 0; i < numLanes; i++) {
        std::string l_linkName = "lane_" + std::string.to_string(i);
        Link *l_link = configureLink(l_linkName);

        if (l_link) {
            m_outLaneLinks.push_back(l_link,  new Event::Handler<c_TxnDispatcher>(this,
                                                                               &c_TxnDispatcher::handleCtrlEvent));
            std::cout<<l_linkName<<" is connected"<<std::endl;
        } else {
            std::cout<<l_linkName<<" is not found.. exit"<<std::endl;
            exit(-1);
        }
    }
}

void c_TxnDispatcher::handleTxnGenEvent(SST::Event *ev)
{

}

void c_TxnDispatcher::handleCtrlEvent(SST::Event *ev)
{

}
