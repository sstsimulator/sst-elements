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

#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include "globals.h"
#include "transaction.h"

using namespace std;
using namespace SST;

class logicLayer : public IntrospectedComponent {
private:
    typedef SST::Link memChan_t;
    typedef vector<memChan_t*> memChans_t;

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
    bool isOurs(unsigned int addr);

    /**
     *  Stats
     */
    // Helper function for printing statistics in MacSim format
    template<typename T>
    void writeTo(ofstream &ofs, string prefix, string name, T count);
    void printStatsForMacSim();

private:
    memChans_t memChans;        // SST links to each Vault
    SST::Link *toMem;
    SST::Link *toCPU;
    int reqLimit;

    unsigned int LL_MASK;
    unsigned int llID;

    // Statistics
    Statistic<uint64_t>* memOpsProcessed;
    
    Statistic<uint64_t>* reqUsedToCpu[2];
    Statistic<uint64_t>* reqUsedToMem[2];

    // Output
    Output dbg;                 // Output, for printing debuging commands
    Output out;                 // Output, for printing always printed info and stats
    int statsFormat;            // Type of Stat output 0:Defualt 1:Macsim (Default Value is set to 0)
};

#endif
