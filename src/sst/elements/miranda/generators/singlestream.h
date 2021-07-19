// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_SINGLE_STREAM_GEN
#define _H_SST_MIRANDA_SINGLE_STREAM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class SingleStreamGenerator : public RequestGenerator {

public:
      SingleStreamGenerator( ComponentId_t id, Params& params );
      void build(Params& params);
      ~SingleStreamGenerator();
      void generate(MirandaRequestQueue<GeneratorRequest*>* q);
      bool isFinished();
      void completed();

      SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
            SingleStreamGenerator,
            "miranda",
            "SingleStreamGenerator",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Creates a single ordered stream of accesses to/from memory",
            SST::Miranda::RequestGenerator
         )

      SST_ELI_DOCUMENT_PARAMS(
            { "verbose",      "Sets the verbosity of the output", "0" },
            { "count",        "Total number of requests", "1000" },
            { "length",       "Sets the length of the request", "8" },
            { "startat",      "Sets the start address of the array", "0" },
            { "max_address",  "Maximum address allowed for generation", "524288" },
            { "memOp",        "All reqeusts will be of this type, [Read/Write]", "Read" },
         )

   private:
      uint64_t reqLength;
      uint64_t maxAddr;
      uint64_t issueCount;
      uint64_t nextAddr;
      uint64_t startAddr;

      Output*  out;
      ReqOperation memOp;

};

}
}

#endif
