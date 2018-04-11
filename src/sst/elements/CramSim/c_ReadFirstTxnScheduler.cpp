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
#include "c_ReadFirstTxnScheduler.hpp"
#include "c_ReadFirstTxnScheduler.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_ReadFirstTxnScheduler::c_ReadFirstTxnScheduler(SST::Component *owner, SST::Params& x_params) : c_TxnScheduler(owner, x_params) {
    //initialize per-channel transaction queues
    m_txnReadQ.resize(m_numChannels);
    m_txnWriteQ.resize(m_numChannels);

    bool l_found;

    k_maxPendingWriteThreshold = (float) x_params.find<float>("maxPendingWriteThreshold", 1, l_found);
    if (!l_found) {
        std::cout << "maxPendingWriteThreshold value is missing... it will be 1.0 (default)" << std::endl;
    } else {
        if (k_maxPendingWriteThreshold > 1) {
            std::cout << "maxPendingWriteThreshold value should be greater than 0 and less than (or equal to) one"
                        << std::endl;
            exit(1);
        }
    }

    k_minPendingWriteThreshold = (float) x_params.find<float>("minPendingWriteThreshold", 0.2, l_found);
    if (!l_found) {
        std::cout << "minPendingWriteThreshold value is missing... it will be 1.0 (default)" << std::endl;
    } else {
        if (k_minPendingWriteThreshold > k_maxPendingWriteThreshold) {
            std::cout << "minPendingWriteThreshold value should be smaller than maxPendingWriteThreshold"
                        << std::endl;
            exit(1);
        }
    }

    m_maxNumPendingWrite = (unsigned) ((float) k_numTxnQEntries * k_maxPendingWriteThreshold);
    m_minNumPendingWrite = (unsigned) ((float) k_numTxnQEntries * k_minPendingWriteThreshold);
    m_flushWriteQueue = false;
}

c_ReadFirstTxnScheduler::~c_ReadFirstTxnScheduler() {

}


void c_ReadFirstTxnScheduler::run(){
    
    std::cout << getName() << " active channels: " << m_activeChannels.size() << std::endl;

    for(std::set<int>::iterator it = m_activeChannels.begin(); it != m_activeChannels.end();) {
        //int l_channelID=0; l_channelID<m_numChannels; l_channelID++) {
        int l_channelID = *it;

        //0. select queue
        TxnQueue* l_queue= nullptr;
        if (m_txnWriteQ[l_channelID].size() >= m_maxNumPendingWrite || m_txnReadQ[l_channelID].size()==0) {
            m_flushWriteQueue = true;
            l_queue = &(m_txnWriteQ[l_channelID]);
        } else if (m_txnWriteQ[l_channelID].size() < m_minNumPendingWrite && m_txnReadQ[l_channelID].size()!=0) {
            m_flushWriteQueue = false;
            l_queue = &(m_txnReadQ[l_channelID]);
        }

        //1. select a transaction from the transaction queue
        c_Transaction* l_nextTxn=nullptr;
        if(l_queue->size())
            l_nextTxn=getNextTxn(*l_queue, l_channelID);
          //1.1. With read-first scheduling, we change the queue if there are no issuable transactions in the selected queue
        if(l_nextTxn== nullptr)
        {
            if(m_flushWriteQueue ==true)
                l_queue = &m_txnReadQ[l_channelID];
            else
                l_queue = &m_txnWriteQ[l_channelID];
            if(l_queue->size())
                l_nextTxn=getNextTxn(*l_queue, l_channelID);
        }

        std::set<int>::iterator next = it++;
        //2. send the selected transaction to transaction converter
        if(l_nextTxn!=nullptr) {
            if(m_cmdScheduler->getToken(l_nextTxn->getHashedAddress())>=3) {

                // send the selected transaction
                m_txnConverter->push(l_nextTxn);

                #ifdef __SST_DEBUG_OUTPUT__
                l_nextTxn->print(output, "[c_ReadFirstTxnScheduler]",m_controller->getSimCycle());
                #endif

                // pop it from inputQ
                popTxn(*l_queue , l_nextTxn);
                if (m_txnWriteQ[l_channelID].empty() && m_txnReadQ[l_channelID].empty())
                    m_activeChannels.erase(it);
            }
        }
        it = next;
    }
}


c_Transaction* c_ReadFirstTxnScheduler::getNextTxn(TxnQueue& x_queue, int x_ch)
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
            printf("unsupported transaction scheduling policy.. exit(1)");
            exit(1);
        }

        return l_nxtTxn;
}


void c_ReadFirstTxnScheduler::popTxn(TxnQueue &x_txnQ, c_Transaction* x_Txn)
{
    x_txnQ.remove(x_Txn);
}

bool c_ReadFirstTxnScheduler::push(c_Transaction* newTxn)
{
    int l_channelId=newTxn->getHashedAddress().getChannel();
    bool l_success=false;
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
    if (l_success)
        m_activeChannels.insert(l_channelId);
    return l_success;
}


//Check if read transactions get data from the transaction queue
bool c_ReadFirstTxnScheduler::isHit(c_Transaction* x_txn)
{
    int l_channelId=x_txn->getHashedAddress().getChannel();
    TxnQueue* l_queue=nullptr;
    bool l_isRead = x_txn->isRead();
    bool l_isHit = false;

    if(l_isRead)
    {
        l_queue = &m_txnWriteQ.at(l_channelId);

        //traverse the transaction queue in reverse order
        for (TxnQueue::reverse_iterator l_txnItr = l_queue->rbegin(); l_txnItr != l_queue->rend(); ++l_txnItr) {
            c_Transaction *l_txn = *l_txnItr;

            if ((l_txn->getAddress() == x_txn->getAddress())) {
                l_isHit = true;
                break;
            }
        }
    }

    return l_isHit;
}

bool c_ReadFirstTxnScheduler::hasDependancy(c_Transaction *x_txn, int x_ch)
{
    TxnQueue* l_queue= nullptr;
    bool l_hasDependancy = false;

    if(x_txn->isRead())
        l_queue = &m_txnWriteQ[x_ch];
    else
        l_queue= &m_txnReadQ[x_ch];

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


