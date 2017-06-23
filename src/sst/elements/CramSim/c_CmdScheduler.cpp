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
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <map>
#include <assert.h>

#include "c_CmdScheduler.hpp"
#include "c_CmdUnit.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_CmdScheduler::c_CmdScheduler(Component *comp, Params &x_params) : c_CtrlSubComponent <c_BankCommand*,c_BankCommand*> (comp, x_params){
    m_nextSubComponent=dynamic_cast<c_Controller*>(comp)->getDeviceController();

    //create command queue
    m_cmdQueues.clear();
    for(unsigned l_bankIdx=0;l_bankIdx<m_numBanks;l_bankIdx++)
    {

        c_CmdQueue *l_cmdQueue=new c_CmdQueue();
        l_cmdQueue->clear();
        m_cmdQueues.push_back(l_cmdQueue);
    }
}

c_CmdScheduler::~c_CmdScheduler(){
    for(auto &it: m_cmdQueues)
    {
        delete it;
    }
}

bool c_CmdScheduler::clockTic(SST::Cycle_t) {

    run();
    send();

}


void c_CmdScheduler::run(){

    for( auto it=m_inputQ.begin();it!=m_inputQ.end();) {
        c_BankCommand* l_cmd=*it;
        uint l_bankID=l_cmd->getHashedAddress()->getBankId();
        assert(l_bankID<m_numBanks);

        //todo: modify to configure the size of command queues
        if (m_cmdQueues.at(l_bankID)->size() < 16) {
            m_cmdQueues.at(l_bankID)->push_back(l_cmd);
         //   printf("[push]sim_cycle:%lld cmdseq:%d bankid:%d size:%d\n",Simulation::getSimulation()->getCurrentSimCycle(), m_cmdQueues.at(l_bankID)->front()->getSeqNum(),  m_cmdQueues.at(l_bankID)->front()->getHashedAddress()->getBankId(), m_cmdQueues.at(l_bankID)->size());

            it=m_inputQ.erase(it);
        } else
            ++it;//printf("cmd queue is full\n");
    }


    for(auto &x_cmdQueue: m_cmdQueues)
        if(x_cmdQueue->size()>0) {

            c_BankCommand *l_cmdPtr = x_cmdQueue->front();
            //  l_cmdPtr->print(m_debugOutput);

            if (m_nextSubComponent->isCmdAllowed(l_cmdPtr)) {
                m_outputQ.push_back(l_cmdPtr);
                x_cmdQueue->pop_front();
                //   printf("[pop]sim_cycle:%lld cmdseq:%d bankid:%d size:%d\n",Simulation::getSimulation()->getCurrentSimCycle(),x_cmdQueue->front()->getSeqNum(), x_cmdQueue->front()->getHashedAddress()->getBankId(),x_cmdQueue->size());
            }
        }

}


void c_CmdScheduler::send() {
    int token=m_nextSubComponent->getToken();

    while(token>0 && !m_outputQ.empty()) {
        m_nextSubComponent->push(m_outputQ.front());
//        m_outputQ.front()->print(m_debugOutput);
        m_outputQ.pop_front();
        token--;
    }
}

