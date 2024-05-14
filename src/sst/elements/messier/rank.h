// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


#ifndef _H_SST_NVM_RANK
#define _H_SST_NVM_RANK

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include <map>
#include <list>

#include "bank.h"
// Defines the state of the bank

// This class structure represents NVM memory Bank
class RANK
{

    // This determines the rank busy until time, this is used to enforce timing parameters, such as maximum number of bank activation per unit of time
    // It also covers the time when the shared circutary between banks is used (e.g., data output circuits of NVM chips)
    uint64_t BusyUntil;
    uint32_t num_banks;

    BANK * banks;

public:
    RANK(uint32_t numBanks) {BusyUntil = 0; num_banks = numBanks; banks = new BANK[numBanks];}

    // Get a specific bank inside this rank
    BANK * getBank(uint32_t ind) { return &banks[ind];}
    void setBusyUntil(uint64_t x) {BusyUntil = x;}
    uint64_t getBusyUntil() { return BusyUntil; }

};

#endif
