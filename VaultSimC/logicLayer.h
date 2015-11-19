// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _LOGICLAYER_H
#define _LOGICLAYER_H

#include <sst/core/event.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/output.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include <queue>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include <DRAMSim.h>
#include <AddressMapping.h>

#include "globals.h"
#include "transaction.h"

using namespace std;
using namespace SST;

#ifdef USE_VAULTSIM_HMC

#define TRANS_FOOTPRINT_MAP_OPTIMUM_SIZE 10
#define TRANS_PART_OPTIMUM_SIZE 4
#define ACTIVE_TRANS_OPTIMUM_SIZE 4

struct vaultTouchFootprint_t {
    vector <uint64_t> transId;
    vector <unsigned> bankId;

    vaultTouchFootprint_t() {
        transId.reserve(TRANS_FOOTPRINT_MAP_OPTIMUM_SIZE);
        bankId.reserve(TRANS_FOOTPRINT_MAP_OPTIMUM_SIZE);
    }

    void insert(uint64_t transId_, unsigned bankId_) {
        transId.push_back(transId_);
        bankId.push_back(bankId_);
    }

    unsigned getSize() { return transId.size(); }

    bool isPresent(unsigned bankId_, uint64_t* transId_) {
        vector<uint64_t>::iterator itTrans = transId.begin();
        vector<unsigned>::iterator itBank;
        for (itBank = bankId.begin(); itBank != bankId.end(); ++itBank, ++itTrans)
            if (*itBank == bankId_)
                break;

        if (itBank == bankId.end())
            return false;
        else {
            *transId_ = *itTrans;
            return true;
        }
    }

    bool isEmpty() {
        return transId.empty();
    }

    void removeTrans(uint64_t transId_) {
        for (vector<uint64_t>::iterator itTrans = transId.begin() ; itTrans != transId.end(); ++itTrans) {
            vector<unsigned>::iterator itBank = bankId.begin();
            if (*itTrans == transId_) {
                transId.erase(itTrans);
                bankId.erase(itBank);
            }
            ++itBank;
        }
    }

};

struct transTouchFootprint_t {
    vector <unsigned> vaultId;
    vector <unsigned> bankId;

    transTouchFootprint_t() {
        vaultId.reserve(TRANS_PART_OPTIMUM_SIZE);
        bankId.reserve(TRANS_PART_OPTIMUM_SIZE);
    }

    void insert(unsigned vaultId_, unsigned bankId_) {
        vaultId.push_back(vaultId_);
        bankId.push_back(bankId_);
    }

    unsigned getSize() { return vaultId.size(); }
};


extern unordered_map<unsigned, vaultTouchFootprint_t > vaultTransFootprint;
extern unordered_map<uint64_t, bool> vaultTransActive;
extern queue<uint64_t> vaultDoneTrans;
extern queue<uint64_t> vaultConflictedTransDone;

#endif

class logicLayer : public IntrospectedComponent {
private:
    typedef SST::Link memChan_t;
    typedef vector<memChan_t*> memChans_t;

    #ifdef USE_VAULTSIM_HMC
    typedef unordered_map<uint64_t, transTouchFootprint_t> tId2tTouchFootprint_t;
    typedef unordered_map<uint64_t, vector<MemHierarchy::MemEvent> > tIdQueue_t;
    #endif

public:
    /** 
     * Constructor
     */
    logicLayer(ComponentId_t id, Params& params);

    /**
     * finish
     * Callback function that gets to be called when simulation finishes
     */
    void finish();

private: 
    /** 
     * Constructor
     */
    logicLayer(const logicLayer& c);

    /** 
     * Step call for LogicLayer
     */
    bool clock(Cycle_t);

    // Determine if we 'own' a given address
    inline bool isOurs(unsigned int addr);

    /**
     *  Stats
     */
    // Helper function for printing statistics in MacSim format
    template<typename T>
    void writeTo(ofstream &ofs, string prefix, string name, T count);
    void printStatsForMacSim();

private:
    unsigned int llID;

    // Links to Vaults
    memChans_t memChans;        // SST links to each Vault
    SST::Link *toMem;
    SST::Link *toCPU;
    int reqLimit;
    int numVaults;

    // for VaultId process
    uint64_t CacheLineSize;             // it is used to determine VaultIDs
    unsigned CacheLineSizeLog2;         // bits of CacheLineSize

    // Multi logicLayer support (FIXME)
    unsigned int LL_MASK;

    // Transaction Support
    #ifdef USE_VAULTSIM_HMC
    tId2tTouchFootprint_t tIdFootprint;     //FIXME: currently maintained but not used, global variables are used
    tIdQueue_t tIdQueue;
    queue<uint64_t> transReadyQueue;
    unsigned activeTransactionsLimit;       //FIXME: Not used now
    unordered_set<uint64_t> activeTransactions;
    #endif

    // Statistics
    Statistic<uint64_t>* memOpsProcessed;
    Statistic<uint64_t>* HMCCandidateProcessed;
    Statistic<uint64_t>* HMCOpsProcessed;
    Statistic<uint64_t>* HMCTransOpsProcessed;
    
    Statistic<uint64_t>* reqUsedToCpu[2];
    Statistic<uint64_t>* reqUsedToMem[2];

    // Output
    Output dbg;                 // Output, for printing debuging commands
    Output out;                 // Output, for printing always printed info and stats
    int statsFormat;            // Type of Stat output 0:Defualt 1:Macsim (Default Value is set to 0)
};

#endif
