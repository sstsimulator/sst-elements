// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
using namespace SST::CramSim;
using namespace std;


c_TxnScheduler::c_TxnScheduler(SST::ComponentId_t id, SST::Params& x_params, Output* out, unsigned channels, c_TxnConverter* converter, c_CmdScheduler* scheduler) :
    SubComponent(id), output(out), m_numChannels(channels), m_txnConverter(converter), m_cmdScheduler(scheduler) {
    build(x_params);
}

void c_TxnScheduler::build(Params& x_params) {
    //initialize member variables
    assert(m_numChannels>0);
    
    // Create output object
    m_out = new Output("", 0, 0, SST::Output::STDOUT);
    
    bool l_found=false;

    string l_txnSchedulingPolicy= (string) x_params.find<std::string>("txnSchedulingPolicy","FCFS", l_found);
    if(!l_found) {
        m_out->output("txnSchedulingPolicy value is missing... FCFS policy will be used\n");
    }

    if(l_txnSchedulingPolicy=="FCFS")
    {
        k_txnSchedulingPolicy=e_txnSchedulingPolicy::FCFS;
    }
    else if(l_txnSchedulingPolicy=="FRFCFS")
    {
        k_txnSchedulingPolicy=e_txnSchedulingPolicy::FRFCFS;
    } else
    {
        m_out->fatal(CALL_INFO, 1, "unsupported txnSchedulingPolicy (%s),, exit\n", l_txnSchedulingPolicy.c_str());
    }


    k_numTxnQEntries = (unsigned) x_params.find<unsigned>("numTxnQEntries", 32, l_found);
    if (!l_found) {
        m_out->output("numTxnQEntries value is missing... it will be 32 (default)\n");
    }

    k_isReadFirstScheduling = (unsigned) x_params.find<unsigned>("boolReadFirstTxnScheduling",0,l_found);
    if (!l_found) {
        m_out->output("boolReadFirstTxnScheduling value is missing... disabled\n");
    }


    //initialize per-channel transaction queues
    if(!k_isReadFirstScheduling)
        m_txnQ.resize(m_numChannels);
    else {
        m_txnReadQ.resize(m_numChannels);
        m_txnWriteQ.resize(m_numChannels);

        k_maxPendingWriteThreshold = (float) x_params.find<float>("maxPendingWriteThreshold", 1, l_found);
        if (!l_found) {
            m_out->output("maxPendingWriteThreshold value is missing... it will be 1.0 (default)\n");
        } else {
            if (k_maxPendingWriteThreshold > 1) {
                m_out->fatal(CALL_INFO, 1, "maxPendingWriteThreshold value should be greater than 0 and less than (or equal to) one\n");
            }
        }

        k_minPendingWriteThreshold = (float) x_params.find<float>("minPendingWriteThreshold", 0.2, l_found);
        if (!l_found) {
            m_out->output("minPendingWriteThreshold value is missing... it will be 0.2 (default)\n");
        } else {
            if (k_minPendingWriteThreshold > k_maxPendingWriteThreshold) {
                m_out->fatal(CALL_INFO, 1, "minPendingWriteThreshold value should be smaller than maxPendingWriteThreshold\n");
            }
        }

        m_maxNumPendingWrite = (unsigned) ((float) k_numTxnQEntries * k_maxPendingWriteThreshold);
        m_minNumPendingWrite = (unsigned) ((float) k_numTxnQEntries * k_minPendingWriteThreshold);
        m_flushWriteQueue = false;
    }
}

c_TxnScheduler::~c_TxnScheduler() {

}


void c_TxnScheduler::run(SimTime_t simCycle){


    for(int l_channelID=0; l_channelID<m_numChannels; l_channelID++) {

        //0. select queue
        TxnQueue* l_queue= nullptr;
        if(!k_isReadFirstScheduling) {
            l_queue = &(m_txnQ[l_channelID]);
        } else {
            if (m_txnWriteQ[l_channelID].size() >= m_maxNumPendingWrite || m_txnReadQ[l_channelID].size()==0)
                m_flushWriteQueue = true;
            else if (m_txnWriteQ[l_channelID].size() < m_minNumPendingWrite && m_txnReadQ[l_channelID].size()!=0)
                m_flushWriteQueue = false;


            if(m_flushWriteQueue) {
                    l_queue = &(m_txnWriteQ[l_channelID]);
            }
            else
                l_queue = &(m_txnReadQ[l_channelID]);
        }

        assert(l_queue!=nullptr);

        //1. select a transaction from the transaction queue
        c_Transaction* l_nextTxn=nullptr;
        if(l_queue->size())
            l_nextTxn=getNextTxn(*l_queue, l_channelID);
          //1.1. With read-first scheduling, we change the queue if there are no issuable transactions in the selected queue
        if(k_isReadFirstScheduling && l_nextTxn== nullptr)
        {
            if(m_flushWriteQueue ==true)
                l_queue = &m_txnReadQ[l_channelID];
            else
                l_queue = &m_txnWriteQ[l_channelID];
            if(l_queue->size())
                l_nextTxn=getNextTxn(*l_queue, l_channelID);
        }

        //2. send the selected transaction to transaction converter
        if(l_nextTxn!=nullptr) {
            if(m_cmdScheduler->getToken(l_nextTxn->getHashedAddress())>=3) {

                // send the selected transaction
                m_txnConverter->push(l_nextTxn);

                #ifdef __SST_DEBUG_OUTPUT__
                l_nextTxn->print(output, "[c_TxnScheduler]",simCycle);
                #endif

                // pop it from inputQ
                popTxn(*l_queue, l_nextTxn);

            }
        }
    }
}


c_Transaction* c_TxnScheduler::getNextTxn(TxnQueue& x_queue, int x_ch)
{

    assert(x_queue.size()!=0);

        //get the next transaction
        c_Transaction* l_nxtTxn = nullptr;

        //FCFS
        if(k_txnSchedulingPolicy == e_txnSchedulingPolicy::FCFS) {
            if(m_cmdScheduler->getToken(x_queue.front()->getHashedAddress())>=3) {
                if(hasDependancy(x_queue.front(), x_ch)==false)
                    l_nxtTxn = x_queue.front();
            }
        }//FRFCFS
        else if(k_txnSchedulingPolicy == e_txnSchedulingPolicy::FRFCFS) {
            for (auto &l_txn: x_queue) {
                if (m_cmdScheduler->getToken(l_txn->getHashedAddress()) >= 3) {
                    if(hasDependancy(l_txn, x_ch)==false)
                        l_nxtTxn = l_txn;
                    else
                        continue;

                    c_BankInfo *l_bankInfo = m_txnConverter->getBankInfo(l_txn->getHashedAddress().getBankId());

                    if (l_bankInfo->isRowOpen()
                        && l_bankInfo->getOpenRowNum() == l_txn->getHashedAddress().getRow()) {
                        break;
                    }
                }
            }
        }
        else
        {
            m_out->fatal(CALL_INFO, 1, "unsupported transaction scheduling policy.. exit(1)\n");
        }

        return l_nxtTxn;
}


void c_TxnScheduler::popTxn(TxnQueue &x_txnQ, c_Transaction* x_Txn)
{
    x_txnQ.remove(x_Txn);
}

bool c_TxnScheduler::push(c_Transaction* newTxn)
{
    int l_channelId=newTxn->getHashedAddress().getChannel();
    bool l_success=false;
    if(!k_isReadFirstScheduling)
    {
        if (m_txnQ.at(l_channelId).size() < k_numTxnQEntries) {
            m_txnQ.at(l_channelId).push_back(newTxn);
            l_success=true;
        } else
            l_success=false;
    }else
    {
        if(newTxn->isRead())
        {
            if(m_txnReadQ[l_channelId].size()< k_numTxnQEntries) {
                m_txnReadQ[l_channelId].push_back(newTxn);
                l_success = true;
            }
            else
                l_success=false;
        } else
        {
            if(m_txnWriteQ[l_channelId].size()< k_numTxnQEntries) {
                l_success=true;
                m_txnWriteQ[l_channelId].push_back(newTxn);
            }else
                l_success=false;
        }
    }
    return l_success;
}


//Check if read transactions get data from the transaction queue
bool c_TxnScheduler::isHit(c_Transaction* x_txn)
{
    int l_channelId=x_txn->getHashedAddress().getChannel();
    TxnQueue* l_queue=nullptr;
    bool l_isRead = x_txn->isRead();
    bool l_isHit = false;

    if(l_isRead)
    {
        if(!k_isReadFirstScheduling) {
            l_queue = &m_txnQ.at(l_channelId);

        } else{
            l_queue = &m_txnWriteQ.at(l_channelId);
        }

        //traverse the transaction queue in reverse order
        for (TxnQueue::reverse_iterator l_txnItr = l_queue->rbegin(); l_txnItr != l_queue->rend(); ++l_txnItr) {
            c_Transaction *l_txn = *l_txnItr;

            if (l_txn->isWrite() && (l_txn->getAddress() == x_txn->getAddress())) {
                l_isHit = true;
                break;
            }
        }
    }

    return l_isHit;
}

bool c_TxnScheduler::hasDependancy(c_Transaction *x_txn, int x_ch)
{
    TxnQueue* l_queue= nullptr;
    bool l_hasDependancy = false;

    if(!k_isReadFirstScheduling)
        l_queue=&m_txnQ[x_ch];
    else
    {
        if(x_txn->isRead())
            l_queue = &m_txnWriteQ[x_ch];
        else
            l_queue= &m_txnReadQ[x_ch];
    }

    for(auto &l_txn: *l_queue)
    {
        if(l_txn->getAddress()==x_txn->getAddress()
           && l_txn->getSeqNum()<x_txn->getSeqNum())
            l_hasDependancy = true;

        if(l_txn->getSeqNum()>=x_txn->getSeqNum())
            break;
    }

    return l_hasDependancy;
}


