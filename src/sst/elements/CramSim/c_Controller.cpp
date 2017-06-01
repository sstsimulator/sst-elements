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




#include "c_AddressHasher.hpp"
#include "c_DeviceController.hpp"
#include "c_Controller.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"
#include "c_TokenChgEvent.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"

using namespace SST::n_Bank;

c_Controller::c_Controller(ComponentId_t id, Params &params) :
        Component(id){

    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("CramSim.Controller[@f:@l:@p] ",
                             verbosity, 0, SST::Output::STDOUT);
    /** Get subcomponent parameters*/
    bool l_found;
    // set address hasher
    std::string l_subCompName = params.find<std::string>("AddrHasher", "CramSim.c_AddressHasher",l_found);
    if(!l_found){
        output->output("AddrHasher is not specified... AddressHasher (default) will be used\n");
    }
    m_addrHasher= dynamic_cast<c_AddressHasher*>(loadSubComponent(l_subCompName.c_str(),this,params));

    // set device controller
    l_subCompName = params.find<std::string>("TransConverter", "CramSim.c_TxnUnit",l_found);
    if(!l_found){
        output->output("Transaction Converter is not specified... c_TxnUnit (default) will be used\n");
    }
    m_transConverter= dynamic_cast<c_TxnUnit*>(loadSubComponent(l_subCompName.c_str(),this,params));

    // set device controller
    l_subCompName = params.find<std::string>("DeviceController", "CramSim.c_CmdUnit",l_found);
    if(!l_found){
        output->output("Device Controller is not specified... c_CmdUnit (default) will be used\n");
    }
    m_deviceController= dynamic_cast<c_CmdUnit*>(loadSubComponent(l_subCompName.c_str(),this,params));

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
    //configure SST link
    configure_link();

    //set our clock
    registerClock("1GHz",
                  new Clock::Handler<c_Controller>(this, &c_Controller::clockTic));

}

c_Controller::~c_Controller(){}


// configure links
void c_Controller::configure_link() {

    // CmdUnit <-> TxnUnit Links
    // TxnUnit <- TxnGen (Res) (Token)
    m_inTxnGenResQTokenChgLink = configureLink("inTxnGenResQTokenChg",
                                               new Event::Handler<c_Controller>(this,
                                                                                &c_Controller::handleInTxnGenResQTokenChgEvent));
    // TxnUnit -> TxnGen (Req) (Token)
    m_outTxnGenReqQTokenChgLink = configureLink("outTxnGenReqQTokenChg",
                                                new Event::Handler<c_Controller>(this,
                                                                                 &c_Controller::handleOutTxnGenReqQTokenChgEvent));
    // TxnGen --> TxnUnit (Req) (Cmd)
    m_inTxnGenReqPtrLink = configureLink("inTxnGenReqPtr",
                                         new Event::Handler<c_Controller>(this,
                                                                          &c_Controller::handleIncomingTransaction));
    //// CmdUnit -> TxnUnit (Res) (Cmd)
    m_outTxnGenResPtrLink = configureLink("outTxnGenResPtr",
                                          new Event::Handler<c_Controller>(this,
                                                                           &c_Controller::handleOutTxnGenResPtrEvent));
    // CmdUnit <-> Bank Links
    // TxnUnit <- Bank (Req) (Token)
    m_inDeviceReqQTokenChgLink = configureLink("inDeviceReqQTokenChg",
                                               new Event::Handler<c_Controller>(this,
                                                                                &c_Controller::handleInDeviceReqQTokenChgEvent));
    //// CmdUnit -> Bank (Req) (Cmd)
    m_outDeviceReqPtrLink = configureLink("outDeviceReqPtr",
                                        new Event::Handler<c_Controller>(this,
                                                                         &c_Controller::handleOutDeviceReqPtrEvent));
    //// CmdUnit <- Bank (Res) (Cmd)
    m_inDeviceResPtrLink = configureLink("inDeviceResPtr",
                                       new Event::Handler<c_Controller>(this,
                                                                        &c_Controller::handleInDeviceResPtrEvent));
}


// clock event handler
bool c_Controller::clockTic(SST::Cycle_t clock) {
    std::vector<c_BankCommand*> l_linkToDeviceCtrl;
    std::vector<c_BankCommand*> l_linkToDevice;

    m_thisCycleReqQTknChg = 0;

    // store the current number of entries in the queue to later compute change
    m_thisCycleReqQTknChg = m_txnReqQ.size();

    //m_transConverter->pushTransaction();
    l_linkToDeviceCtrl= m_transConverter->convertTransaction();

    if(l_linkToDeviceCtrl.size()>0) {
        m_deviceController->pushCommand(l_linkToDeviceCtrl);
        //todo: scheduler
        //m_cmdScheduler->schedule();
    }
    l_linkToDevice = m_deviceController->issueCommand();

    if(l_linkToDevice.size()>0)
        handleOutgoingCommand(l_linkToDevice);

    sendResponse();
    //Todo: fix tocken handling method
    //m_thisCycleReqQTknChg -= m_txnReqQ.size();
    m_thisCycleReqQTknChg=1;
    sendTokenChg();



    return false;
}

void c_Controller::finish(){};



void c_Controller::handleIncomingTransaction(SST::Event *ev){

    c_TxnReqEvent* l_txnReqEventPtr = dynamic_cast<c_TxnReqEvent*>(ev);

    if (l_txnReqEventPtr) {
        //std::cout << "@" << std::dec
        //	  << Simulation::getSimulation()->getCurrentSimCycle() << ": "
        //	  << __PRETTY_FUNCTION__ << l_txnReqEventPtr->m_payload << " ";
        //l_txnReqEventPtr->m_payload->print();
        //std::cout << std::endl;

        //TODO: fix tocken handling method
       // m_txnReqQ.push_back(l_txnReqEventPtr->m_payload);
        m_txnResQ.push_back(l_txnReqEventPtr->m_payload);
        m_transConverter->pushTransaction(ev);
        //delete l_txnReqEventPtr;
    } else {
        std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                  << std::endl;
    }
}

void c_Controller::handleOutgoingCommand(std::vector<c_BankCommand*> issuedCmds)
{
    for(auto & cmd : issuedCmds ){
        c_CmdReqEvent *l_cmdReqEventPtr = new c_CmdReqEvent();
        l_cmdReqEventPtr->m_payload = cmd;
        m_outDeviceReqPtrLink->send(l_cmdReqEventPtr);
    }
}

void c_Controller::handleOutTxnGenResPtrEvent(SST::Event *ev){}

void c_Controller::handleInDeviceResPtrEvent(SST::Event *ev){
    c_CmdResEvent* l_cmdResEventPtr = dynamic_cast<c_CmdResEvent*>(ev);
    if (l_cmdResEventPtr) {
        ulong l_resSeqNum = l_cmdResEventPtr->m_payload->getSeqNum();
        // need to find which txn matches the command seq number in the txnResQ
        c_Transaction* l_txnRes = nullptr;
        for(auto l_txIter : m_txnResQ) {
            if(l_txIter->matchesCmdSeqNum(l_resSeqNum)) {
                l_txnRes = l_txIter;
            }
        }

        if(l_txnRes == nullptr) {
            std::cout << "Error! Couldn't find transaction to match cmdSeqnum " << l_resSeqNum << std::endl;
            exit(-1);
        }

        const unsigned l_cmdsLeft = l_txnRes->getWaitingCommands() - 1;
        l_txnRes->setWaitingCommands(l_cmdsLeft);
        if (l_cmdsLeft == 0)
            l_txnRes->setResponseReady();

        //std::cout << std::endl << "@" << std::dec
        //      << Simulation::getSimulation()->getCurrentSimCycle() << ": "
        //	      << __PRETTY_FUNCTION__ << std::endl;
        //l_txnRes->print();
        //std::cout << std::endl;

        delete l_cmdResEventPtr->m_payload;
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
        // std::cout << std::endl << "@" << std::dec
        // 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
        // 		<< __PRETTY_FUNCTION__ << std::endl;

        m_txnGenResQTokens += l_txnGenResQTokenChgEventPtr->m_payload;

        assert(m_txnGenResQTokens >= 0);
        assert(m_txnGenResQTokens <= k_txnReqQEntries);

        // std::cout << "m_ResQTokens: " << m_txnGenResQTokens << std::endl;
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
        // std::cout << std::endl << "@" << std::dec
        // 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
        // 		<< __PRETTY_FUNCTION__ << std::endl;

        m_deviceReqQTokens += l_cmdUnitReqQTokenChgEventPtr->m_payload;

        assert(m_deviceReqQTokens >= 0);
        assert(m_deviceReqQTokens <= k_txnResQEntries);

        //FIXME: Critical: This pointer is left dangling
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

void c_Controller::sendTokenChg() {
    // only send tokens when space has opened up in queues
    // there are no negative tokens. token subtraction must be performed
    // in the source component immediately after sending an event
    if (m_thisCycleReqQTknChg > 0) {
        // send req q token chg
        // std::cout << "@" << std::dec
        // 		<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
        // 		<< __PRETTY_FUNCTION__ << std::endl;
        // std::cout << "TxnUnit:: Sending req q tokens" << std::endl;
        c_TokenChgEvent* l_txnReqQTokenChgEvent = new c_TokenChgEvent();
        l_txnReqQTokenChgEvent->m_payload = m_thisCycleReqQTknChg;
        m_outTxnGenReqQTokenChgLink->send(l_txnReqQTokenChgEvent);
    }
}

void c_Controller::sendResponse() {

    // sendResponse conditions:
    // - m_txnGenResQTokens > 0
    // - m_txnResQ.size() > 0
    // - m_txnResQ has an element which is response-ready

    //std::cout << "m_txnGenResQTokens " << std::dec << m_txnGenResQTokens << std::endl;
    //std::cout << "m_txnResQ " << std::dec << m_txnResQ.size() << std::endl;
    //printQueues();

    if ((m_txnGenResQTokens > 0) && (m_txnResQ.size() > 0)) {

        c_Transaction* l_txnRes = nullptr;
        for (std::vector<c_Transaction*>::iterator l_it = m_txnResQ.begin();
             l_it != m_txnResQ.end(); ++l_it) {
            if ((*l_it)->isResponseReady()) {
                l_txnRes = *l_it;
                m_txnResQ.erase(l_it);
                break;
            }
        }

        if (l_txnRes != nullptr) {

            //std::cout << "@" << std::dec
            //		<< Simulation::getSimulation()->getCurrentSimCycle()
            //		<< ": " << __PRETTY_FUNCTION__ << std::endl;
            //l_txnRes->print();
            //std::cout << std::endl;

            c_TxnResEvent* l_txnResEvPtr = new c_TxnResEvent();
            l_txnResEvPtr->m_payload = l_txnRes;
            m_outTxnGenResPtrLink->send(l_txnResEvPtr);
            --m_txnGenResQTokens;

        }

    }
} // sendResponse
