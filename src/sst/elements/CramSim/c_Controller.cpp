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

#include "c_Controller.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_HashedAddress.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_Controller::c_Controller(ComponentId_t id, Params &params) :
        Component(id){

    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("CramSim.Controller[@f:@l:@p] ",
                             verbosity, 0, SST::Output::STDOUT);


    /** Get subcomponent parameters*/
    bool l_found;

    // set device controller
    std::string l_subCompName  = params.find<std::string>("DeviceDriver", "CramSim.c_DeviceDriver",l_found);
    if(!l_found){
        output->output("Device Controller is not specified... c_DeviceDriver (default) will be used\n");
    }
    m_deviceDriver= dynamic_cast<c_DeviceDriver*>(loadSubComponent(l_subCompName.c_str(),this,params));

    // set cmd schduler
    l_subCompName = params.find<std::string>("CmdScheduler", "CramSim.c_CmdScheduler",l_found);
    if(!l_found){
        output->output("Command Scheduler is not specified... c_CmdScheduler (default) will be used\n");
    }

    m_cmdScheduler= dynamic_cast<c_CmdScheduler*>(loadSubComponent(l_subCompName.c_str(),this,params));

    // set transaction converter
    l_subCompName = params.find<std::string>("TxnConverter", "CramSim.c_TxnConverter",l_found);
    if(!l_found){
        output->output("Transaction Converter is not specified... c_TxnConverter (default) will be used\n");
    }
    m_txnConverter= dynamic_cast<c_TxnConverter*>(loadSubComponent(l_subCompName.c_str(),this,params));


    // set transaction scheduler
    l_subCompName = params.find<std::string>("TxnScheduler", "CramSim.c_TxnScheduler",l_found);
    if(!l_found){
        output->output("Transaction Scheduler is not specified... c_TxnScheduler (default) will be used\n");
    }
    m_txnScheduler= dynamic_cast<c_TxnScheduler*>(loadSubComponent(l_subCompName.c_str(), this, params));

    // set address hasher
    l_subCompName = params.find<std::string>("AddrHasher", "CramSim.c_AddressHasher",l_found);
    if(!l_found){
        output->output("AddrHasher is not specified... AddressHasher (default) will be used\n");
    }
    m_addrHasher= dynamic_cast<c_AddressHasher*>(loadSubComponent(l_subCompName.c_str(),this,params));


    /** Get SST link parameters*/
    k_txnReqQEntries = (uint32_t)params.find<uint32_t>("numCtrlReqQEntries", 100,
                                                         l_found);
    if (!l_found) {
        std::cout << "numCtrlReqQEntries param value is missing... default(100)"
                  << std::endl;
    }

    k_txnResQEntries = (uint32_t)params.find<uint32_t>("numCtrlResQEntries", 100,
                                                         l_found);
    if (!l_found) {
        std::cout << "numCtrlResQEntries param value is missing... default(100)"
                  << std::endl;
    }

   //load neighboring component's params
    k_txnGenResQEntries = (uint32_t)params.find<uint32_t>("numTxnGenResQEntries", 100,
                                                            l_found);
    if (!l_found) {
        std::cout << "numTxnGenResQEntries param value is missing... exiting"
                  << std::endl;
        exit(-1);
    }
    m_txnGenResQTokens = k_txnGenResQEntries;
    m_ReqQTokens= k_txnReqQEntries;

    k_enableQuickResponse = (uint32_t)params.find<uint32_t>("boolEnableQuickRes", 0,l_found);
    if (!l_found) {
        std::cout << "boolEnableQuickRes param value is missing... disabled"
                  << std::endl;
    }

    // get configured clock frequency
    k_controllerClockFreqStr = (string)params.find<string>("strControllerClockFrequency", "1GHz", l_found);
    
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

    // TxnUnit <--> Controller Links
    // TxnGen -> Controller (Res) (Token)
    m_inTxnGenResQTokenChgLink = configureLink("inTxnGenResQTokenChg",
                                               new Event::Handler<c_Controller>(this,
                                                                                &c_Controller::handleInTxnGenResQTokenChgEvent));
    // TxnGen <- Controller (Req) (Token)
    m_outTxnGenReqQTokenChgLink = configureLink("outTxnGenReqQTokenChg",
                                                new Event::Handler<c_Controller>(this,
                                                                                 &c_Controller::handleOutTxnGenReqQTokenChgEvent));
    // TxnGen -> Controller (Req) (Txn)
    m_inTxnGenReqPtrLink = configureLink("inTxnGenReqPtr",
                                         new Event::Handler<c_Controller>(this,
                                                                          &c_Controller::handleIncomingTransaction));
    // TxnUnit <- Controller (Res) (Txn)
    m_outTxnGenResPtrLink = configureLink("outTxnGenResPtr",
                                          new Event::Handler<c_Controller>(this,
                                                                           &c_Controller::handleOutTxnGenResPtrEvent));
    // DeviceDriver <-> Bank Links
    // Controller <- Device (Req) (Token)
    m_inDeviceReqQTokenChgLink = configureLink("inDeviceReqQTokenChg",
                                               new Event::Handler<c_Controller>(this,
                                                                                &c_Controller::handleInDeviceReqQTokenChgEvent));
    // Controller -> Device (Req) (Cmd)
    m_outDeviceReqPtrLink = configureLink("outDeviceReqPtr",
                                        new Event::Handler<c_Controller>(this,
                                                                         &c_Controller::handleOutDeviceReqPtrEvent));
    // Controller <- Device (Res) (Cmd)
    m_inDeviceResPtrLink = configureLink("inDeviceResPtr",
                                       new Event::Handler<c_Controller>(this,
                                                                        &c_Controller::handleInDeviceResPtrEvent));
}


// clock event handler
bool c_Controller::clockTic(SST::Cycle_t clock) {

    sendResponse();

    m_thisCycleTxnQTknChg = m_ReqQ.size();

    // 0. update device driver
    m_deviceDriver->update();

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
            m_ReqQ.erase(l_it);
            #ifdef __SST_DEBUG_OUTPUT__
                newTxn->print(output,"[TxnQueue hit]");
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
                newTxn->print(output,"[Controller queues new txn]");
            #endif
        }
        else
            l_it++;
    }

    // 3. run transaction Scheduler
    m_txnScheduler->run();

    // 4. run transaction converter
    m_txnConverter->run();

    // 5, run command scheduler
    m_cmdScheduler->run();

    // 6. run device driver
    m_deviceDriver->run();

    // 7. send token to the transaction generator
    m_thisCycleTxnQTknChg = m_thisCycleTxnQTknChg-m_ReqQ.size();
    if (m_thisCycleTxnQTknChg > 0) {
        sendTokenChg();
    }

    return false;
}


void c_Controller::sendCommand(c_BankCommand* cmd)
{
     c_CmdReqEvent *l_cmdReqEventPtr = new c_CmdReqEvent();
     l_cmdReqEventPtr->m_payload = cmd;
     m_outDeviceReqPtrLink->send(l_cmdReqEventPtr);
}


void c_Controller::sendTokenChg() {
    // only send tokens when space has opened up in queues
    // there are no negative tokens. token subtraction must be performed
    // in the source component immediately after sending an event

    // send req q token chg
    c_TokenChgEvent* l_txnReqQTokenChgEvent = new c_TokenChgEvent();
    l_txnReqQTokenChgEvent->m_payload = m_thisCycleTxnQTknChg;
    m_outTxnGenReqQTokenChgLink->send(l_txnReqQTokenChgEvent);
}

void c_Controller::sendResponse() {

    // sendResponse conditions:
    // - m_txnGenResQTokens > 0
    // - m_ResQ.size() > 0
    // - m_ResQ has an element which is response-ready

    if ((m_txnGenResQTokens > 0) && (m_ResQ.size() > 0)) {
        c_Transaction* l_txnRes = nullptr;
        for (std::deque<c_Transaction*>::iterator l_it = m_ResQ.begin();
             l_it != m_ResQ.end();)  {
	    if(m_txnGenResQTokens <= 0) {
	      break;
	    }
            if ((*l_it)->isResponseReady()) {
                l_txnRes = *l_it;
                l_it=m_ResQ.erase(l_it);
                //break;
                c_TxnResEvent* l_txnResEvPtr = new c_TxnResEvent();
                l_txnResEvPtr->m_payload = l_txnRes;
                m_outTxnGenResPtrLink->send(l_txnResEvPtr);

                --m_txnGenResQTokens;
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


        m_ReqQ.push_back(newTxn);
        m_ResQ.push_back(newTxn);


        delete l_txnReqEventPtr;
    } else {
        std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                  << std::endl;
    }
}


void c_Controller::handleOutTxnGenResPtrEvent(SST::Event *ev){}


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
            std::cout << "Error! Couldn't find transaction to match cmdSeqnum " << l_resSeqNum << std::endl;
            exit(-1);
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
        //delete l_cmdResEventPtr;

    } else {
        std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                  << std::endl;
    }
}
void c_Controller::handleOutDeviceReqPtrEvent(SST::Event *ev){}


void c_Controller::handleInTxnGenResQTokenChgEvent(SST::Event *ev) {
    c_TokenChgEvent* l_txnGenResQTokenChgEventPtr =
            dynamic_cast<c_TokenChgEvent*>(ev);
    if (l_txnGenResQTokenChgEventPtr) {


        m_txnGenResQTokens += l_txnGenResQTokenChgEventPtr->m_payload;

        assert(m_txnGenResQTokens >= 0);
        assert(m_txnGenResQTokens <= k_txnReqQEntries);

        delete l_txnGenResQTokenChgEventPtr;
    } else {
        std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                  << std::endl;
    }
}

void c_Controller::handleInDeviceReqQTokenChgEvent(SST::Event *ev) {
    c_TokenChgEvent* l_cmdUnitReqQTokenChgEventPtr =
            dynamic_cast<c_TokenChgEvent*>(ev);
    if (l_cmdUnitReqQTokenChgEventPtr) {

        m_deviceReqQTokens += l_cmdUnitReqQTokenChgEventPtr->m_payload;

        assert(m_deviceReqQTokens >= 0);
        assert(m_deviceReqQTokens <= k_txnResQEntries);

        delete l_cmdUnitReqQTokenChgEventPtr;
    } else {
        std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                  << std::endl;
    }
}

void c_Controller::handleOutTxnGenReqQTokenChgEvent(SST::Event *ev) {
    // nothing to do here
    std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
              << std::endl;
}

void c_Controller::setHashedAddress(c_Transaction* newTxn)
{
    c_HashedAddress l_hashedAddress;
    m_addrHasher->fillHashedAddress(&l_hashedAddress, newTxn->getAddress());
    newTxn->setHashedAddress(l_hashedAddress);
}

