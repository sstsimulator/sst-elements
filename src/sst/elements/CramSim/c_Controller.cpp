// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

#include "c_Controller.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_HashedAddress.hpp"

using namespace SST;
using namespace SST::CramSim;

c_Controller::c_Controller(ComponentId_t id, Params &params) :
        Component(id){

    int verbosity = params.find<int>("verbose", 0);
    debug = new SST::Output("CramSim.Controller[@f:@l:@p] ",
                             verbosity, 0, SST::Output::STDOUT);
    output = new SST::Output("", verbosity, 0, SST::Output::STDOUT);

    m_simCycle=0;

    /** Get subcomponent parameters*/
    bool l_found;
    std::string l_subCompName;

    // set device controller
    using std::placeholders::_1;
    m_deviceDriver = loadUserSubComponent<c_DeviceDriver>("DeviceDriver", ComponentInfo::SHARE_NONE, debug, std::bind(&SST::CramSim::c_Controller::sendCommand, this, _1));
    if (!m_deviceDriver) {
        l_subCompName  = params.find<std::string>("DeviceDriver", "CramSim.c_DeviceDriver",l_found);
        if(l_found){
            output->output("%s, Warning: loading DeviceDriver subcomponent from params. In the future use 'DeviceDriver' subcomponent slot instead\n", getName().c_str());
        }
        m_deviceDriver = loadAnonymousSubComponent<c_DeviceDriver>(l_subCompName, "DeviceDriver", 0, ComponentInfo::INSERT_STATS, params,
                debug, std::bind(&SST::CramSim::c_Controller::sendCommand, this, _1));
    }

    // set cmd schduler
    m_cmdScheduler = loadUserSubComponent<c_CmdScheduler>("CmdScheduler", ComponentInfo::SHARE_NONE, debug, m_deviceDriver);
    if (!m_cmdScheduler) {
        l_subCompName = params.find<std::string>("CmdScheduler", "CramSim.c_CmdScheduler", l_found);
        if (l_found) {
            output->output("%s, Warning: loading CmdScheduler subcomponent from params. In the future use 'CmdScheduler' subcomponent slot instead\n", getName().c_str());
        }
        m_cmdScheduler = loadAnonymousSubComponent<c_CmdScheduler>(l_subCompName, "CmdScheduler", 0, ComponentInfo::INSERT_STATS, params, debug, m_deviceDriver);
    }

    // set transaction converter
    m_txnConverter = loadUserSubComponent<c_TxnConverter>("TxnConverter", ComponentInfo::SHARE_NONE, debug, m_deviceDriver->getTotalNumBank(), m_cmdScheduler);
    if (!m_txnConverter) {
        l_subCompName = params.find<std::string>("TxnConverter", "CramSim.c_TxnConverter",l_found);
        if(l_found){
            output->output("%s, Warning: loading TxnConverter subcomponent from params. In the future use 'TxnConverter' subcomponent slot instead\n", getName().c_str());
        }
        m_txnConverter = loadAnonymousSubComponent<c_TxnConverter>(l_subCompName, "TxnConverter", 0, ComponentInfo::INSERT_STATS, params, debug,
                m_deviceDriver->getTotalNumBank(), m_cmdScheduler);
    }

    // set transaction scheduler
    m_txnScheduler = loadUserSubComponent<c_TxnScheduler>("TxnScheduler", ComponentInfo::SHARE_NONE, debug, m_deviceDriver->getNumChannel(),
            m_txnConverter, m_cmdScheduler);
    if (!m_txnScheduler) {
        l_subCompName = params.find<std::string>("TxnScheduler", "CramSim.c_TxnScheduler",l_found);
        if(l_found){
            output->output("%s, Warning: loading TxnScheduler subcomponent from params. In the future use 'TxnScheduler' subcomponent slot instead\n", getName().c_str());
        }
        m_txnScheduler = loadAnonymousSubComponent<c_TxnScheduler>(l_subCompName, "TxnScheduler", 0, ComponentInfo::INSERT_STATS, params, debug,
                m_deviceDriver->getNumChannel(), m_txnConverter, m_cmdScheduler);
    }

    // set address hasher
    m_addrHasher = loadUserSubComponent<c_AddressHasher>("AddrMapper", ComponentInfo::SHARE_NONE, debug, m_deviceDriver->getNumChannel(),
            m_deviceDriver->getNumRanksPerChannel(), m_deviceDriver->getNumBankGroupsPerRank(), m_deviceDriver->getNumBanksPerBankGroup(),
            m_deviceDriver->getNumRowsPerBank(), m_deviceDriver->getNumColPerBank(), m_deviceDriver->getNumPChPerChannel());
    if (!m_addrHasher) {
        l_subCompName = params.find<std::string>("AddrMapper", "CramSim.c_AddressHasher",l_found);
        if(l_found){
            output->output("%s, Warning: loading AddrMapper subcomponent from params. In the future use 'AddrMapper' subcomponent slot instead\n", getName().c_str());
        }
        m_addrHasher = loadAnonymousSubComponent<c_AddressHasher>(l_subCompName, "AddrMapper", 0, ComponentInfo::INSERT_STATS, params, debug,
                m_deviceDriver->getNumChannel(), m_deviceDriver->getNumRanksPerChannel(), m_deviceDriver->getNumBankGroupsPerRank(),
                m_deviceDriver->getNumBanksPerBankGroup(), m_deviceDriver->getNumRowsPerBank(), m_deviceDriver->getNumColPerBank(), m_deviceDriver->getNumPChPerChannel());
    }

    k_enableQuickResponse = (uint32_t)params.find<uint32_t>("boolEnableQuickRes", 0,l_found);
    if (!l_found) {
        output->output("boolEnableQuickRes param value is missing... disabled\n");
    }

    // get configured clock frequency
    k_controllerClockFreqStr = (std::string)params.find<std::string>("strControllerClockFrequency", "1GHz", l_found);

    //configure SST link
    configure_link();

    //set our clock
    registerClock(k_controllerClockFreqStr,
                  new Clock::Handler<c_Controller>(this, &c_Controller::clockTic));



}

c_Controller::~c_Controller(){}

c_Controller::c_Controller() :
        Component(-1) {
    // for serialization only
}
// configure links
void c_Controller::configure_link() {

    // TxnGen <-> Controller (Txn)
    m_txngenLink = configureLink("txngenLink",
                                         new Event::Handler<c_Controller>(this,
                                                                          &c_Controller::handleIncomingTransaction));
    // Controller <-> Device (Cmd)
    m_memLink = configureLink("memLink",
                                       new Event::Handler<c_Controller>(this,
                                                                        &c_Controller::handleInDeviceResPtrEvent));
}


// clock event handler
bool c_Controller::clockTic(SST::Cycle_t clock) {

    m_simCycle++;

    sendResponse();

    // 0. update device driver
    m_deviceDriver->update(m_simCycle);

    // 1. Convert physical address to device address
    // 2. Push transactions to the transaction queue
    for(std::deque<c_Transaction*>::iterator l_it=m_ReqQ.begin() ; l_it!=m_ReqQ.end();)
    {
        c_Transaction* newTxn= *l_it;

        if(newTxn->hasHashedAddress()== false)
        {
            c_HashedAddress l_hashedAddress;
            m_addrHasher->fillHashedAddress(&l_hashedAddress, newTxn->getAddress());
            newTxn->setHashedAddress(l_hashedAddress);
        }

        //If new transaction hits in the transaction queue, send a response immediately and do not access memory
        if(k_enableQuickResponse && m_txnScheduler->isHit(newTxn))
        {
            newTxn->setResponseReady();
            //delete the new transaction from request queue
            l_it=m_ReqQ.erase(l_it);

            #ifdef __SST_DEBUG_OUTPUT__
                newTxn->print(debug,"[TxnQueue hit]",m_simCycle);
            #endif
            continue;
        }
        //insert new transaction into a transaction queue
        if(m_txnScheduler->push(newTxn)) {
            //With fast write response mode, controller sends a response for a write request as soon as it push the request to a transaction queue.
            if(k_enableQuickResponse && newTxn->isWrite())
            {
                //create a response and push it to the response queue.
               c_Transaction* l_txnRes = new c_Transaction(newTxn->getSeqNum(),newTxn->getTransactionMnemonic(),newTxn->getAddress(),newTxn->getDataWidth());
               l_txnRes->setResponseReady();
                m_ResQ.push_back(l_txnRes);
            }


            l_it = m_ReqQ.erase(l_it);

            #ifdef __SST_DEBUG_OUTPUT__
                newTxn->print(debug,"[Controller queues new txn]",m_simCycle);
            #endif
        }
        else
            break;
    }

    // 3. run transaction Scheduler
    m_txnScheduler->run(m_simCycle);

    // 4. run transaction converter
    m_txnConverter->run(m_simCycle);

    // 5, run command scheduler
    m_cmdScheduler->run(m_simCycle);

    // 6. run device driver
    m_deviceDriver->run();


    return false;
}


void c_Controller::sendCommand(c_BankCommand* cmd)
{
     c_CmdReqEvent *l_cmdReqEventPtr = new c_CmdReqEvent();
     l_cmdReqEventPtr->m_payload = cmd;
    //m_outDeviceReqPtrLink->send(l_cmdReqEventPtr);
    m_memLink->send(l_cmdReqEventPtr);
}


void c_Controller::sendResponse() {

    // sendResponse conditions:
    // - m_txnGenResQTokens > 0
    // - m_ResQ.size() > 0
    // - m_ResQ has an element which is response-ready

    if ((m_ResQ.size() > 0)) {
        c_Transaction* l_txnRes = nullptr;
        for (std::deque<c_Transaction*>::iterator l_it = m_ResQ.begin();
             l_it != m_ResQ.end();)  {
            if ((*l_it)->isResponseReady()) {
                l_txnRes = *l_it;
                l_it=m_ResQ.erase(l_it);

                c_TxnResEvent* l_txnResEvPtr = new c_TxnResEvent();
                l_txnResEvPtr->m_payload = l_txnRes;

                m_txngenLink->send(l_txnResEvPtr);
            }
            else
            {
                l_it++;
            }
        }
    }
} // sendResponse



///** Link Event handlers **///
void c_Controller::handleIncomingTransaction(SST::Event *ev){

    c_TxnReqEvent* l_txnReqEventPtr = dynamic_cast<c_TxnReqEvent*>(ev);

    if (l_txnReqEventPtr) {
        c_Transaction* newTxn=l_txnReqEventPtr->m_payload;

        #ifdef __SST_DEBUG_OUTPUT__
        newTxn->print(debug,"[c_Controller.handleIncommingTransaction]",m_simCycle);
        #endif

        m_ReqQ.push_back(newTxn);
        m_ResQ.push_back(newTxn);


        delete l_txnReqEventPtr;
    } else {
        output->output("ERROR:: Bad event type!\n");
    }
}


void c_Controller::handleInDeviceResPtrEvent(SST::Event *ev){
    c_CmdResEvent* l_cmdResEventPtr = dynamic_cast<c_CmdResEvent*>(ev);
    if (l_cmdResEventPtr) {
        ulong l_resSeqNum = l_cmdResEventPtr->m_payload->getSeqNum();
        // need to find which txn matches the command seq number in the txnResQ
        c_Transaction* l_txnRes = nullptr;
        std::deque<c_Transaction*>::iterator l_txIter;

        for(l_txIter=m_ResQ.begin() ; l_txIter!=m_ResQ.end();l_txIter++) {
            if((*l_txIter)->matchesCmdSeqNum(l_resSeqNum)) {
                l_txnRes = *l_txIter;
                break;
            }
        }

        if(l_txnRes == nullptr) {
            output->fatal(CALL_INFO, -1, "Error! Couldn't find transaction to match cmdSeqnum %lu\n", l_resSeqNum);
        }

        const unsigned l_cmdsLeft = l_txnRes->getWaitingCommands() - 1;
        l_txnRes->setWaitingCommands(l_cmdsLeft);
        if (l_cmdsLeft == 0) {
            l_txnRes->setResponseReady();
            // With quick response mode, controller sends a response to a requester for a write request as soon as the request is pushed to a transaction queue
            // So, we don't need to send another response at this time. Just erase the request in the response queue.
            if ( k_enableQuickResponse && !l_txnRes->isRead()) {
                delete l_txnRes;
                m_ResQ.erase(l_txIter);
            }
        }

        delete l_cmdResEventPtr->m_payload;         //now, free the memory space allocated to the commands for a transaction
        delete l_cmdResEventPtr;

    } else {
        output->output("ERROR:: Bad event type!\n");
    }
}

