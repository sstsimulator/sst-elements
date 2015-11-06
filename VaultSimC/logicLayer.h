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

    /**
     *
     */
    void init(unsigned int phase);

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
    bool isOurs(unsigned int addr) {
        return ((((addr >> LL_SHIFT) & LL_MASK) == llID) || (LL_MASK == 0));
    }

private:
    memChans_t memChans;
    SST::Link *toMem;
    SST::Link *toCPU;
    int bwLimit;

    unsigned int LL_MASK;
    unsigned int llID;
    unsigned long long memOps;

    // Statistics
    Statistic<uint64_t>* bwUsedToCpu[2];
    Statistic<uint64_t>* bwUsedToMem[2];

    // Output
    Output dbg;                 // Output, for printing debuging commands
    Output out;                 // Output, for printing always printed info and stats
    int statsFormat;            // Type of Stat output 0:Defualt 1:Macsim (Default Value is set to 0)
};

#endif
