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
    m_addrHasher = m_controller->getAddrHasher();
    m_cmdScheduler = m_controller->getCmdScheduler();

    output = m_controller->getOutput();

    //initialize member variables
    m_nextChannel=0;
    m_numChannels = m_controller->getDeviceDriver()->getNumChannel();
    assert(m_numChannels>0);

    //initialize per-channel transaction queues
    m_txnQ.resize(m_numChannels);

    bool l_found=false;

    string l_txnSchedulePolicy= (string) x_params.find<std::string>("txnSchedulePolicy","FCFS", l_found);
    if(!l_found) {
        std::cout << "txnSchedulePolicy value is missing... FCFS policy will be used" << std::endl;
    }
    if(l_txnSchedulePolicy=="FCFS")
    {
        k_txnSchedulePolicy=0;
    }
    else if(l_txnSchedulePolicy=="FRFCFS")
    {
        k_txnSchedulePolicy=1;
    } else
    {
        std::cout << "unsupported txnSchedulePolicy ("<<l_txnSchedulePolicy<<")"<< std::endl;
    }


    k_numTxnQEntries = (uint32_t) x_params.find<uint32_t>("numTxnQEntries", 32, l_found);
    if (!l_found) {
        std::cout << "numTxnQEntries:w value is missing... it will be 32 (default)" << std::endl;
    }


}

c_TxnScheduler::~c_TxnScheduler() {

}


void c_TxnScheduler::run(){

    int l_numSchedTxn=0;
    int l_channelID=m_nextChannel;

    while(l_numSchedTxn<m_numChannels) {


        //1. select a transaction from the transaction queue
        c_Transaction* l_nextTxn=getNextTxn(l_channelID);



        //2. send the selected transaction to transaction converter
        if(l_nextTxn!=nullptr) {
            if(m_cmdScheduler->getToken(l_nextTxn->getHashedAddress())>=3) {

                // send the selected transaction
                m_txnConverter->push(l_nextTxn);

                #ifdef __SST_DEBUG_OUTPUT__
                l_nextTxn->print(output, "[c_TxnScheduler]");
                #endif

                // pop it from inputQ
                popTxn(l_channelID, l_nextTxn);

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
        //FCFS
        c_Transaction* l_nxtTxn = nullptr;
        if(k_txnSchedulePolicy==0) {
            l_nxtTxn = m_txnQ.at(x_ch).front();
        }else if(k_txnSchedulePolicy==1){

            //pick a transaction going to an open bank
            for(auto& l_txn: m_txnQ.at(x_ch))
            {

                c_BankInfo* l_bankInfo = m_txnConverter->getBankInfo(l_txn->getHashedAddress().getBankId());

                if(l_bankInfo->isRowOpen()
                   && l_bankInfo->getOpenRowNum()==l_txn->getHashedAddress().getRow()){
                    l_nxtTxn = l_txn;
                    break;
                }
            }

            // pick a transaction at the head of queue if there is no pended transaction going to an open bank
            if(l_nxtTxn==nullptr)
                l_nxtTxn = m_txnQ.at(x_ch).front();
        }

        return l_nxtTxn;
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

    if(m_txnQ.at(l_channelId).size()<k_numTxnQEntries) {
        m_txnQ.at(l_channelId).push_back(newTxn);
        return true;
    }
    else
        return false;
}

