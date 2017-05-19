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


#ifndef _LOGICLAYER_H
#define _LOGICLAYER_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>
#include "vaultGlobals.h"

using namespace std;

namespace SST {
namespace VaultSim {

//#define STUPID_DEBUG 

class logicLayer : public Component {
  
public: // functions
  
  logicLayer( ComponentId_t id, Params& params );
  int Finish();
  void init(unsigned int phase);
  
private: // types
  
  typedef SST::Link memChan_t;
  typedef vector<memChan_t*> memChans_t;
  
private: 
  
  logicLayer( const logicLayer& c );
  bool clock( Cycle_t );
  // determine if we 'own' a given address
  bool isOurs(unsigned int addr) {
    return ((((addr >> LL_SHIFT) & LL_MASK) == llID)
	    || (LL_MASK == 0));
  }

  Output dbg;
  memChans_t m_memChans;
  SST::Link *toMem;
  SST::Link *toCPU;
  int bwlimit;
  unsigned int LL_MASK;
  unsigned int llID;
  unsigned long long memOps;

    Statistic<uint64_t>*  bwUsedToCpu[2]; 
    Statistic<uint64_t>*  bwUsedToMem[2]; 
};

}
}

#endif
