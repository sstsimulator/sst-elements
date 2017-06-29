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
#include "sst_config.h"

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
    output=dynamic_cast<c_Controller*>(comp)->getOutput();
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

    for(auto &x_cmdQueue: m_cmdQueues)
        if(!x_cmdQueue->empty()) {

            c_BankCommand *l_cmdPtr = x_cmdQueue->front();
            //  l_cmdPtr->print(m_debugOutput);

            if (m_nextSubComponent->isCmdAllowed(l_cmdPtr)) {
                m_outputQ.push_back(l_cmdPtr);
                x_cmdQueue->pop_front();

#ifdef __SST_DEBUG_OUTPUT__
                output->debug(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\n",
                          Simulation::getSimulation()->getCurrentSimCycle(),
                          l_cmdPtr->getCommandString().c_str(),
                          l_cmdPtr->getHashedAddress()->getChannel(),
                          l_cmdPtr->getHashedAddress()->getPChannel(),
                          l_cmdPtr->getHashedAddress()->getRank(),
                          l_cmdPtr->getHashedAddress()->getBankGroup(),
                          l_cmdPtr->getHashedAddress()->getBank(),
                          l_cmdPtr->getHashedAddress()->getRow(),
                          l_cmdPtr->getHashedAddress()->getCol(),
                              l_cmdPtr->getHashedAddress()->getBankId(),
                         l_cmdPtr->getSeqNum());
#endif

            }
        }

  /*  if(m_outputQ.empty())
    {
        while(!m_inputQ.empty()) {
            m_outputQ.push_back(m_inputQ.front());
            m_inputQ.pop_front();
        }
    }*/
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


bool c_CmdScheduler::push(c_BankCommand* x_cmd) {
    uint l_bankID=x_cmd->getHashedAddress()->getBankId();
    assert(l_bankID<m_numBanks);

    //todo: modify to configure the size of command queues
    if (m_cmdQueues.at(l_bankID)->size() < k_numCtrlIntQEntries) {
        m_cmdQueues.at(l_bankID)->push_back(x_cmd);
        return true;
        //   printf("[push]sim_cycle:%lld cmdseq:%d bankid:%d size:%d\n",Simulation::getSimulation()->getCurrentSimCycle(), m_cmdQueues.at(l_bankID)->front()->getSeqNum(),  m_cmdQueues.at(l_bankID)->front()->getHashedAddress()->getBankId(), m_cmdQueues.at(l_bankID)->size());
    } else
        return false;

    //m_cmdQueue.push_back(x_cmd);
}


unsigned c_CmdScheduler::getToken(const c_HashedAddress &x_addr)
{
   // return k_numCtrlIntQEntries-m_cmdQueue.size();
   unsigned l_bankID=x_addr.getBankId();
    assert(l_bankID<m_numBanks);

    return k_numCtrlIntQEntries-m_cmdQueues.at(l_bankID)->size();

}
