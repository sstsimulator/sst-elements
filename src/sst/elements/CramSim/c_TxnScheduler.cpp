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
    m_controller = dynamic_cast<c_Controller *>(owner);
    m_txnConverter = m_controller->getTxnConverter();
    m_cmdScheduler = m_controller->getCmdScheduler();

    output = m_controller->getOutput();

    //initialize member variables
    m_numChannels = m_controller->getDeviceDriver()->getNumChannel();
    assert(m_numChannels>0);

    bool l_found=false;

    string l_txnSchedulingPolicy= (string) x_params.find<std::string>("txnSchedulingPolicy","FCFS", l_found);
    if(!l_found) {
        std::cout << "txnSchedulingPolicy value is missing... FCFS policy will be used" << std::endl;
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
        std::cout << "unsupported txnSchedulingPolicy ("<<l_txnSchedulingPolicy<<"),, exit"<< std::endl;
        exit(1);
    }


    k_numTxnQEntries = (unsigned) x_params.find<unsigned>("numTxnQEntries", 32, l_found);
    if (!l_found) {
        std::cout << "numTxnQEntries value is missing... it will be 32 (default)" << std::endl;
    }

    k_isReadFirstScheduling = (unsigned) x_params.find<unsigned>("boolReadFirstTxnScheduling",0,l_found);
    if (!l_found) {
        std::cout << "boolReadFirstTxnScheduling value is missing... it will be 32 (default)" << std::endl;
    }


    //initialize per-channel transaction queues
    if(!k_isReadFirstScheduling)
        m_txnQ.resize(m_numChannels);
    else {
        k_pendingWriteThreshold = (float) x_params.find<float>("pendingWriteThreshold", 1, l_found);
        if (!l_found) {
            std::cout << "pendedWriteThreshold value is missing... it will be 1.0 (default)" << std::endl;
        } else {
            if (k_pendingWriteThreshold > 1) {
                std::cout << "pendedWriteThreshold value should be greater than 0 and less than (or equal to) one"
                          << std::endl;
                exit(1);
            }
        }
        m_maxNumPendingWrite = (unsigned) ((float) k_numTxnQEntries * k_pendingWriteThreshold);
        m_txnReadQ.resize(m_numChannels);
        m_txnWriteQ.resize(m_numChannels);
    }
}

c_TxnScheduler::~c_TxnScheduler() {

}


void c_TxnScheduler::run(){


    for(int l_channelID=0; l_channelID<m_numChannels; l_channelID++) {


        //1. select a transaction from the transaction queue
        c_Transaction* l_nextTxn=nullptr;
        //select queue
        TxnQueue* l_queue= nullptr;

        if(!k_isReadFirstScheduling)
        {
            l_queue = &(m_txnQ[l_channelID]);
        } else
        {
            if(m_txnWriteQ[l_channelID].size() >=m_maxNumPendingWrite || m_txnReadQ.empty())
                l_queue = &(m_txnWriteQ[l_channelID]);
            else
                l_queue = &(m_txnReadQ[l_channelID]);
        }

        assert(l_queue!=nullptr);
        if(l_queue->size())
            l_nextTxn=getNextTxn(*l_queue);

        //2. send the selected transaction to transaction converter
        if(l_nextTxn!=nullptr) {
            if(m_cmdScheduler->getToken(l_nextTxn->getHashedAddress())>=3) {

                // send the selected transaction
                m_txnConverter->push(l_nextTxn);

                #ifdef __SST_DEBUG_OUTPUT__
                l_nextTxn->print(output, "[c_TxnScheduler]");
                #endif

                // pop it from inputQ
                popTxn(*l_queue, l_nextTxn);

            }
        }
    }
}


c_Transaction* c_TxnScheduler::getNextTxn(TxnQueue& x_queue)
{
    assert(x_queue.size()!=0);

        //get the next transaction
        c_Transaction* l_nxtTxn = nullptr;


        //FCFS
        if(k_txnSchedulingPolicy == e_txnSchedulingPolicy::FCFS) {
            if(m_cmdScheduler->getToken(x_queue.front()->getHashedAddress())>=3)
                l_nxtTxn= x_queue.front();
        }//FRFCFS
        else if(k_txnSchedulingPolicy == e_txnSchedulingPolicy::FRFCFS) {
            for (auto &l_txn: x_queue) {
                if (m_cmdScheduler->getToken(l_txn->getHashedAddress()) >= 3) {
                    l_nxtTxn = l_txn;

                    //pick a transaction going to an open bank
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

void c_TxnScheduler::popTxn(TxnQueue &x_txnQ, c_Transaction* x_Txn)
{
    x_txnQ.remove(x_Txn);
}

bool c_TxnScheduler::push(c_Transaction* newTxn)
{
    int l_channelId=newTxn->getHashedAddress().getChannel();

    if(!k_isReadFirstScheduling)
    {
        if (m_txnQ.at(l_channelId).size() < k_numTxnQEntries) {
            m_txnQ.at(l_channelId).push_back(newTxn);
            return true;
        } else
            return false;
    }else
    {
        if(newTxn->isRead())
        {
            if(m_txnReadQ[l_channelId].size()< k_numTxnQEntries)
                m_txnReadQ[l_channelId].push_back(newTxn);
            else
                return false;
        } else
        {
            if(m_txnWriteQ[l_channelId].size()< k_numTxnQEntries)
                m_txnWriteQ[l_channelId].push_back(newTxn);
            else
                return false;
        }
    }
}

