// Copyright 2012-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2012-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _CPU_H
#define _CPU_H

#include <sst/core/component.h>
#include <sst/elements/VaultSimC/memReqEvent.h>
#include <sst/core/rng/sstrng.h>
#include <sst/core/output.h>


using namespace std;

namespace SST { 
namespace VaultSim {

#ifndef VAULTSIMC_DBG
#define VAULTSIMC_DBG 0
#endif

//#define STUPID_DEBUG 

class cpu : public Component {

public: // functions

    SST_ELI_REGISTER_COMPONENT(
                               cpu,
                               "VaultSimC",
                               "cpu",
                               SST_ELI_ELEMENT_VERSION(1,0,0),
                               "A simple 'cpu' ",
                               COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
                            {"clock",              "Simple CPU Clock Rate."},
                            {"threads",            "Number of simulated threads in cpu."},
                            {"app",                "Synthetic Application. 0:miniMD-like 1:phdMesh-like. (See app.cpp for details)."},
                            {"bwlimit",            "Maximum number of memory instructions issued by the processor per cycle. Note, each thread can only have at most 2 outstanding memory references at a time. "},
                            {"seed",               "Optional random number generator seed. If not defined or 0, uses srandomdev()."}
                            )

    SST_ELI_DOCUMENT_PORTS(
                           {"toMem", "Link to the memory system", {"memEvent",""}}
                           )

  cpu( ComponentId_t id, Params& params );
  void finish();
  
private: // types
  
  typedef SST::Link memChan_t;
  typedef set<uint64_t> memSet_t;
  typedef vector<memSet_t> thrSet_t;
  typedef vector<int> coreVec_t;

private: 
  
  cpu( const cpu& c );
  bool clock( Cycle_t );
  
  SST::RNG::SSTRandom* rng;
  memChan_t *toMem;
  unsigned int outstanding;
  unsigned long long memOps;
  unsigned long long inst;
  int threads;
  int app;
  int bwlimit;
  thrSet_t thrOutstanding;
  coreVec_t coreAddr;

  MemReqEvent *getInst(int cacheLevel, int app, int core);

protected:
  Output &out;
};

}
}

#endif
