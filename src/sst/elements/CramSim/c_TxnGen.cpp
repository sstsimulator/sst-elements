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

//SST includes
#include "sst_config.h"

#include <assert.h>
#include <iostream>
#include <string>
#include <ctime>

//local includes
#include "c_TxnGen.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"



using namespace SST;
using namespace SST::n_Bank;

c_TxnGenBase::c_TxnGenBase(ComponentId_t x_id, Params& x_params) :
        Component(x_id) {

    /*---- LOAD PARAMS ----*/

    //used for reading params
    bool l_found = false;

    // internal params
    m_seqNum = 0;
    m_resReadCount = 0;
    m_resWriteCount = 0;
    m_numOutstandingReqs = 0;
    m_numTxns = 0;


    k_numTxnPerCycle =  x_params.find<std::uint32_t>("numTxnPerCycle", 1, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: numTxnPerCycle is missing... exiting"
                  << std::endl;
        exit(-1);
    }

    k_maxOutstandingReqs =  x_params.find<std::uint32_t>("maxOutstandingReqs", 0, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: maxOutstandingReqs is missing... No limit to the maximum number of outstanding transactions"
                  << std::endl;
    }

    k_maxTxns =  x_params.find<std::uint32_t>("maxTxns", 0, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: maxTxns is missing... No limit to the maximum number of transactions"
                  << std::endl;
        //exit(-1);
    }

    /*---- CONFIGURE LINKS ----*/

    // request-related links
    //// send to txn unit
    //// send token chg to txn unit
    m_lowLink = configureLink(
            "lowLink",
            new Event::Handler<c_TxnGenBase>(this, &c_TxnGenBase::handleResEvent));

    // get configured clock frequency
    std::string l_controllerClockFreqStr = (std::string)x_params.find<std::string>("strControllerClockFrequency", "1GHz", l_found);

    //set our clock
    registerClock(l_controllerClockFreqStr,
                  new Clock::Handler<c_TxnGenBase>(this, &c_TxnGenBase::clockTic));

    // Statistics
    s_readTxnsCompleted = registerStatistic<uint64_t>("readTxnsCompleted");
    s_writeTxnsCompleted = registerStatistic<uint64_t>("writeTxnsCompleted");

}

c_TxnGenBase::~c_TxnGenBase() {
}

c_TxnGenBase::c_TxnGenBase() :
        Component(-1) {
    // for serialization only
}


bool c_TxnGenBase::clockTic(Cycle_t) {
    createTxn();

    for(int i=0;i<k_numTxnPerCycle;i++) {
        if(k_maxOutstandingReqs==0 || m_numOutstandingReqs<k_maxOutstandingReqs) {

            readResponse();
            
            if(sendRequest()==false)
                break;
            
            m_numOutstandingReqs++;
            m_numTxns++;
        } else
            break;

        if(k_maxTxns>0 && m_numTxns>=k_maxTxns) {
            primaryComponentOKToEndSim();
            return true;
        }
    }
    return false;
}



void c_TxnGenBase::handleResEvent(SST::Event* ev) {

    c_TxnResEvent* l_txnResEventPtr = dynamic_cast<c_TxnResEvent*> (ev);

    if (l_txnResEventPtr)
    {
        c_Transaction *l_txn=l_txnResEventPtr->m_payload;
        if (l_txn->getTransactionMnemonic()
            == e_TransactionType::READ) {
            s_readTxnsCompleted->addData(1);
            m_resReadCount++;
        } else {
            s_writeTxnsCompleted->addData(1);
            m_resWriteCount++;
        }

        m_numOutstandingReqs--;
        assert(m_numOutstandingReqs>=0);
    

	m_txnResQ.push_back(l_txnResEventPtr->m_payload);
        
        delete l_txnResEventPtr;

    } else {
        std::cout << std::endl << std::endl << "TxnGen:: "
                  << __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
                  << std::endl;
    }
}



bool c_TxnGenBase::sendRequest()
{
    assert(k_maxOutstandingReqs==0 || m_numOutstandingReqs<=k_maxOutstandingReqs);
    if(!m_txnReqQ.empty())
    {
        uint64_t l_cycle=Simulation::getSimulation()->getCurrentSimCycle();

        // confirm that interval timer has run out before contiuing
        if (m_txnReqQ.front().second > l_cycle) {
            // std::cout << __PRETTY_FUNCTION__ << ": Interval timer for front txn is not done"
            // << std::endl;
            // m_txnReqQ.front().first->print();
            // std::cout << " - Interval:" << m_txnReqQ.front().second << std::endl;
            return false;
        }

        if (m_txnReqQ.front().first->getTransactionMnemonic()
            == e_TransactionType::READ)
            m_reqReadCount++;
        else
            m_reqWriteCount++;

        c_TxnReqEvent* l_txnReqEvPtr = new c_TxnReqEvent();
        l_txnReqEvPtr->m_payload = m_txnReqQ.front().first;
        m_txnReqQ.pop_front();

        assert(m_lowLink!=NULL);
        m_lowLink->send(l_txnReqEvPtr);
        return true;
    }
    else
        return false;
}


void c_TxnGenBase::readResponse() {
	if (m_txnResQ.size() > 0) {
		c_Transaction* l_txn = m_txnResQ.front();
		delete l_txn;

		m_txnResQ.pop_front();
		// std::cout << "TxnGen::readResponse() Transaction printed: Addr-"
		// 		<< l_txn->getAddress() << std::endl;
	} else {
		// std::cout << "TxnGen::readResponse(): No transactions in res q to read"
		// 		<< std::endl;
	}
}




c_TxnGen::c_TxnGen(ComponentId_t x_id, Params& x_params) :
        c_TxnGenBase(x_id,x_params) {

    /*---- LOAD PARAMS ----*/

    //used for reading params
    bool l_found = false;

    // internal params
    m_prevAddress = 0;

    //ratio of read txn's : write txn's to generate
    k_readWriteTxnRatio
            = x_params.find<float>("readWriteRatio", 0.5, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: readWriteRatio value is missing... it will be 0.5 (default)"
                  << std::endl;
        //exit(-1);
    }

    //set mode (random or sequential)
    std::string l_mode = x_params.find<std::string>("mode", "rand", l_found);
    if (!l_found) {
        std::cout << "TxnGen:: mode is missing... exiting"
                  << std::endl;
        exit(-1);
    } else {
        if (l_mode == "rand")
            m_mode = e_TxnMode::RAND;
        else if (l_mode == "seq")
            m_mode = e_TxnMode::SEQ;
        else {
            std::cout << "TxnGen:: mode (" << l_mode
                      << ").. It should be \"rand\" or \"seq\" .. exiting"
                      << std::endl;
            exit(-1);
        }
    }

    // initialize the random seed
    std::string l_randSeedStr = x_params.find<std::string>("randomSeed","0", l_found);
    l_randSeedStr.pop_back(); // remove trailing newline (??)
    if(l_randSeedStr.compare("-") == 0) { // use a random seed
        k_randSeed = (SimTime_t)time(nullptr);
    } else {
        k_randSeed = (SimTime_t)std::strtoul(l_randSeedStr.c_str(),NULL,0);
    }
    std::srand(k_randSeed);

}




uint64_t c_TxnGen::getNextAddress() {
    if(m_mode==e_TxnMode::RAND)
        return (rand());
    else
        return (m_prevAddress+1);
}


void c_TxnGen::createTxn() {

    uint64_t l_cycle = Simulation::getSimulation()->getCurrentSimCycle();

    while(m_txnReqQ.size()<k_numTxnPerCycle)
    {
        uint64_t addr = getNextAddress();
        m_seqNum++;

        double l_read2write = ((double) rand() / (RAND_MAX));

        //TODO: Default value for dataWidth?
        c_Transaction* mTxn;
        if (l_read2write < k_readWriteTxnRatio)
            mTxn = new c_Transaction(m_seqNum, e_TransactionType::READ, addr, 1);
        else
            mTxn = new c_Transaction(m_seqNum, e_TransactionType::WRITE, addr, 1);

        m_prevAddress = addr;

        std::pair<c_Transaction*, uint64_t > l_entry = std::make_pair(mTxn,l_cycle);
        m_txnReqQ.push_back(l_entry);
    }
}


// Element Libarary / Serialization stuff
