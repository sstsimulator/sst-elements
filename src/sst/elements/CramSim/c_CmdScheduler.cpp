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
#include "c_DeviceDriver.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_CmdScheduler::c_CmdScheduler(Component *comp, Params &x_params) : c_CtrlSubComponent <c_BankCommand*,c_BankCommand*> (comp, x_params){
    m_deviceController=dynamic_cast<c_Controller*>(comp)->getDeviceDriver();
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



void c_CmdScheduler::run(){

       for(auto &l_cmdQueue: m_cmdQueues)
           if(!l_cmdQueue->empty()) {
                c_BankCommand *l_cmdPtr = l_cmdQueue->front();

                if (m_deviceController->isCmdAllowed(l_cmdPtr)) {
                   bool isSuccess=m_deviceController->push(l_cmdPtr);
                    if(isSuccess)
                       l_cmdQueue->pop_front();

                    #ifdef __SST_DEBUG_OUTPUT__
                    output->verbose(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\n",
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
}


bool c_CmdScheduler::push(c_BankCommand* x_cmd) {
    uint l_bankID=x_cmd->getHashedAddress()->getBankId();
    assert(l_bankID<m_numBanks);

    //todo: modify to configure the size of command queues
    if (m_cmdQueues.at(l_bankID)->size() < k_numCtrlIntQEntries) {
        m_cmdQueues.at(l_bankID)->push_back(x_cmd);
        return true;
    } else
        return false;

}


unsigned c_CmdScheduler::getToken(const c_HashedAddress &x_addr)
{
   unsigned l_bankID=x_addr.getBankId();
    assert(l_bankID<m_numBanks);

    return k_numCtrlIntQEntries-m_cmdQueues.at(l_bankID)->size();

}
