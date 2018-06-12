// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
#include <sst/core/elementinfo.h>
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
  
    SST_ELI_REGISTER_COMPONENT(
                               logicLayer,
                               "VaultSimC",
                               "logicLayer",
                               SST_ELI_ELEMENT_VERSION(1,0,0),
                               "A logic layer in a stacked memory",
                               COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
                            {"clock",              "Logic Layer Clock Rate."},
                            {"llID",               "Logic Layer ID (Unique id within chain)"},
                            {"bwlimit",            "Number of memory events which can be processed per cycle per link."},
                            {"LL_MASK",            "Bitmask to determine 'ownership' of an address by a cube. A cube 'owns' an address if ((((addr >> LL_SHIFT) & LL_MASK) == llID) || (LL_MASK == 0)). LL_SHIFT is set in vaultGlobals.h and is 8 by default."},
                            {"terminal",           "Is this the last cube in the chain?"},
                            {"vaults",             "Number of vaults per cube."},
                            {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."}
                                );

   SST_ELI_DOCUMENT_STATISTICS(
      { "BW_recv_from_CPU", "Bandwidth used (recieves from the CPU by the LL) per cycle (in messages)", "reqs/cycle", 1},
      { "BW_send_to_CPU", "Bandwidth used (sends from the CPU by the LL) per cycle (in messages)", "reqs/cycle", 2},
      { "BW_recv_from_Mem", "Bandwidth used (recieves from other memories by the LL) per cycle (in messages)", "reqs/cycle", 3},
      { "BW_send_to_Mem", "Bandwidth used (sends from other memories by the LL) per cycle (in messages)", "reqs/cycle", 4}
          )

    SST_ELI_DOCUMENT_PORTS(
       {"bus_%(vaults)d", "Link to the individual memory vaults", {"memEvent",""}},
       {"toCPU", "Connection towards the processor (directly to the proessor, or down the chain in the direction of the processor)", {"memEvent",""}},    
       {"toMem", "If 'terminal' is 0 (i.e. this is not the last cube in the chain) then this port connects to the next cube.", {"memEvent",""}},
           )

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
