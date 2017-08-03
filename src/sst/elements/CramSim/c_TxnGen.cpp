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

c_TxnGen::c_TxnGen(ComponentId_t x_id, Params& x_params) :
        Component(x_id) {

    /*---- LOAD PARAMS ----*/

    //used for reading params
    bool l_found = false;

    // internal params
    m_prevAddress = 0;
    m_seqNum = 0;
    m_resReadCount = 0;
    m_resWriteCount = 0;

    //ratio of read txn's : write txn's to generate
    k_readWriteTxnRatio
            = x_params.find<float>("readWriteRatio", 1.0, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: readWriteRatio value is missing... exiting"
                  << std::endl;
        exit(-1);
    }

    k_numTxnPerCycle =  x_params.find<std::uint32_t>("numTxnPerCycle", 1, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: numTxnPerCycle is missing... exiting"
                  << std::endl;
        exit(-1);
    }

    k_maxOutstandingReqs =  x_params.find<std::uint32_t>("maxOutstandingReqs", 64, l_found);
    if (!l_found) {
        std::cout << "TxnGen:: maxOutstandingReqs is missing... exiting"
                  << std::endl;
        exit(-1);
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

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    /*---- CONFIGURE LINKS ----*/

    // request-related links
    //// send to txn unit
    //// send token chg to txn unit
    m_lowLink = configureLink(
            "lowLink",
            new Event::Handler<c_TxnGen>(this, &c_TxnGen::handleResEvent));

    // get configured clock frequency
    std::string l_controllerClockFreqStr = (std::string)x_params.find<std::string>("strControllerClockFrequency", "1GHz", l_found);

    //set our clock
    registerClock(l_controllerClockFreqStr,
                  new Clock::Handler<c_TxnGen>(this, &c_TxnGen::clockTic));

    // Statistics
    s_readTxnsCompleted = registerStatistic<uint64_t>("readTxnsCompleted");
    s_writeTxnsCompleted = registerStatistic<uint64_t>("writeTxnsCompleted");
}

c_TxnGen::~c_TxnGen() {
}

c_TxnGen::c_TxnGen() :
        Component(-1) {
    // for serialization only
}

uint64_t c_TxnGen::getNextAddress() {
    return (rand());
}

c_Transaction* c_TxnGen::createTxn() {
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
    return mTxn;

}

bool c_TxnGen::clockTic(Cycle_t) {

    for(int i=0;i<k_numTxnPerCycle;i++) {
        if(m_txnReqQ.size()<k_maxOutstandingReqs) {
            c_Transaction* l_newTxn = createTxn();
            assert(l_newTxn!= nullptr);

            sendRequest(l_newTxn);
        } else
            break;
    }
    return false;
}


void c_TxnGen::handleResEvent(SST::Event* ev) {

    c_TxnResEvent* l_txnResEventPtr = dynamic_cast<c_TxnResEvent*> (ev);
    if (l_txnResEventPtr) {
        c_Transaction *l_txn=l_txnResEventPtr->m_payload;
        if (l_txn->getTransactionMnemonic()
            == e_TransactionType::READ) {
            s_readTxnsCompleted->addData(1);
            m_resReadCount++;
        } else {
            s_writeTxnsCompleted->addData(1);
            m_resWriteCount++;
        }

        m_txnReqQ.erase(l_txn->getSeqNum());

        delete l_txn;
        delete l_txnResEventPtr;

    } else {
        std::cout << std::endl << std::endl << "TxnGen:: "
                  << __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
                  << std::endl;
    }
}

void c_TxnGen::sendRequest(c_Transaction* x_txn) {
    assert(m_txnReqQ.size()<k_maxOutstandingReqs);

    m_txnReqQ[x_txn->getSeqNum()]=x_txn;
    c_TxnReqEvent* l_txnReqEvPtr = new c_TxnReqEvent();
    l_txnReqEvPtr->m_payload = x_txn;

    assert(m_lowLink!=NULL);
    m_lowLink->send(l_txnReqEvPtr);
}

// Element Libarary / Serialization stuff
