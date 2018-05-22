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


#ifndef _H_SST_MIRANDA_STAKE
#define _H_SST_MIRANDA_STAKE

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

// Spike required headers
#include "sim.h"
#include "mmu.h"
#include "remote_bitbang.h"
#include "cachesim.h"
#include "extension.h"
#include <dlfcn.h>
#include <fesvr/option_parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>

namespace SST {
namespace Miranda {

class Stake : public RequestGenerator {

public:
	Stake( Component* owner, Params& params );
	~Stake();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

        void StakeRequest(uint64_t addr, uint32_t RegLen,
                          bool Read, bool Write, bool Atomic, bool Custom,
                          uint32_t Code );

	SST_ELI_REGISTER_SUBCOMPONENT(
                Stake,
                "miranda",
                "Stake",
                SST_ELI_ELEMENT_VERSION(1,0,0),
		"Instantiates a RISC-V Spike instance to drive memory traffic",
               	"SST::Miranda::RequestGenerator"
       	)

       	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose",          "Sets the verbosity output of the generator", "0" },
    		{ "cores",            "Sets the number of cores in the spike instance", "1" },
    		{ "mem_size",         "Sets the RISC-V Spike memory subsystem size", "2048" },
    		{ "log",              "Generate a log of the execution", "0" },
    		{ "isa",              "Set the respective RISC-V ISA", "RV64IMAFDC" },
    		{ "pc",               "Override the default ELF entry point", "0x80000000"},
    		{ "proxy_kernel",     "Set the default proxy kernel", "pk"},
    		{ "bin",              "Set the RISC-V ELF binary", "NULL"},
    		{ "args",             "Set the RISC-V binary arguments", "NULL"},
    		{ "extension",        "Specify the RoCC extension", "NULL" },
    		{ "extlib",           "Shared library to load", "NULL" }
        )

private:
        bool done;          // holds the completion code of the sim
        bool log;           // log the Spike execution to a file
        int rtn;            // return code from the simulator
        size_t cores;       // number of RISC-V cores
        uint64_t pc;        // starting pc
        std::string msize;  // size of the memory subsystem
        std::string isa;    // isa string
        std::string pk;     // proxy kernel
        std::string bin;    // ELF binary
        std::string args;   // Binary args
        std::string ext;    // RoCC extension
        std::string extlib; // extension library

        MirandaRequestQueue<GeneratorRequest*>* MQ; // memory request queue
        std::vector<std::pair<reg_t, mem_t*>> make_mems(const char* arg);

	Output*  out;     // output handle

        sim_t *spike;     // spike simulator instance
};

}
}

#ifdef __cplusplus
extern "C"{
#endif
void SR(uint64_t addr, uint32_t RegLen,
        bool Read, bool Write, bool Atomic, bool Custom,
        uint32_t Code );
#ifdef __cplusplus
}
#endif // __cplusplus

#endif
