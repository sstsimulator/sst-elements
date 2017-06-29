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


#include "sst_config.h"

// std includes
#include <iostream>
#include <assert.h>

// local includes
#include "c_TxnScheduler.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_TxnScheduler::c_TxnScheduler(SST::Component *owner, SST::Params& x_params) :  c_CtrlSubComponent <c_Transaction*,c_Transaction*> (owner, x_params) {
    m_txnConverter = dynamic_cast<c_Controller *>(owner)->getTxnConverter();
    m_addrHasher = dynamic_cast<c_Controller *>(owner)->getAddrHasher();
    m_nextChannel=0;
    output = dynamic_cast<c_Controller *>(owner)->getOutput();

    //initialize per-channel transaction queues
    m_txnQ.resize(m_numChannels);
}

c_TxnScheduler::~c_TxnScheduler() {

}


bool c_TxnScheduler::clockTic(SST::Cycle_t){
    run();
    return false;
}


void c_TxnScheduler::run(){

    int l_numSchedTxn=0;
    int l_channelID=m_nextChannel;

    while(l_numSchedTxn<m_numChannels) {

        //1. select a transaction from the transaction queue
        c_Transaction* nextTxn=getNextTxn(l_channelID);

        //2. send the selected transaction to transaction converter
        if(nextTxn!=nullptr) {
            if(m_txnConverter->push(nextTxn)) {
                popTxn(l_channelID, nextTxn);

#ifdef __SST_DEBUG_OUTPUT__
                output->debug(CALL_INFO,1,0,"Cycle:%lld Cmd:%s CH:%d PCH:%d Rank:%d BG:%d B:%d Row:%d Col:%d BankId:%d CmdSeq:%lld\n",
                              Simulation::getSimulation()->getCurrentSimCycle(),
                              nextTxn->getTransactionString().c_str(),
                              nextTxn->getHashedAddress().getChannel(),
                              nextTxn->getHashedAddress().getPChannel(),
                              nextTxn->getHashedAddress().getRank(),
                              nextTxn->getHashedAddress().getBankGroup(),
                              nextTxn->getHashedAddress().getBank(),
                              nextTxn->getHashedAddress().getRow(),
                              nextTxn->getHashedAddress().getCol(),
                              nextTxn->getHashedAddress().getBankId(),
                              nextTxn->getSeqNum());
#endif
            } else {
                //todo: debug message
                //inputQ of txnconverter is full
            }
        }

        l_numSchedTxn++;
        l_channelID = (l_channelID + 1) % m_numChannels;
    }

    m_nextChannel=l_channelID;
}


c_Transaction* c_TxnScheduler::getNextTxn(int x_ch)
{
    //FCFS
    if(!m_txnQ.at(x_ch).empty()) {
        //get the next transaction
        return m_txnQ.at(x_ch).front();
    } else
        return nullptr;

}

void c_TxnScheduler::popTxn(int x_ch, c_Transaction* x_Txn)
{
    m_txnQ.at(x_ch).remove(x_Txn);
}

bool c_TxnScheduler::push(c_Transaction* newTxn)
{
    int l_channelId=newTxn->getHashedAddress().getChannel();

    if(m_txnQ.at(l_channelId).size()<k_numCtrlIntQEntries) {
        m_txnQ.at(l_channelId).push_back(newTxn);
        return true;
    }
    else
        return false;
}

