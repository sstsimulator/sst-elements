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


#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/stake.h>

using namespace SST::Miranda;

Stake::Stake( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

        // default parameters
        spike = NULL;
        rtn = 0;

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("Stake[@p:@l]: ", verbose, 0, Output::STDOUT);

        log = params.find<bool>("log", false);
        cores = params.find<size_t>("cores", 1);
        pc = params.find<uint64_t>("pc", 0x80000000);
        isa = params.find<std::string>("isa", "RV64IMAFDC");
        pk = params.find<std::string>("proxy_kernel", "pk");
        bin = params.find<std::string>("bin", "");
        ext = params.find<std::string>("ext", "");
        extlib = params.find<std::string>("extlib","");

        if( log ){
          out->verbose(CALL_INFO, 1, 0, "Logging is enabled\n" );
        }else{
          out->verbose(CALL_INFO, 1, 0, "Logging is disabled\n" );
        }

        if( bin.length() == 0 ){
          out->fatal(CALL_INFO, -1, "Failed to specify the RISC-V binary" );
        }

        out->verbose(CALL_INFO, 1, 0, "RISC-V Cores = %" PRIu32 "\n", cores );
        out->verbose(CALL_INFO, 1, 0, "Starting PC = %" PRIx64 "\n", pc );
        out->verbose(CALL_INFO, 1, 0, "ISA = %s\n", isa );
        if( ext.length() > 0 ){
          out->verbose(CALL_INFO, 1, 0, "RoCC Extension = %s\n", ext );
        }
        if( extlib.length() > 0 ){
          out->verbose(CALL_INFO, 1, 0, "External Library = %s\n", extlib );
        }

        done = false;
}

Stake::~Stake() {
        delete spike;
	delete out;
}

// adapted from the original make_mems source from the spike.cc driver
std::vector<std::pair<reg_t, mem_t*>> Stake::make_mems(const char* arg){
        // handle legacy mem argument
        char* p;
        auto mb = strtoull(arg, &p, 0);
        if (*p == 0) {
          reg_t size = reg_t(mb) << 20;
          if (size != (size_t)size)
            out->fatal(CALL_INFO, -1, "Memsize would overflow size_t" );
          return std::vector<std::pair<reg_t, mem_t*>>(1, std::make_pair(reg_t(DRAM_BASE), new mem_t(size)));
        }

        // handle base/size tuples
        std::vector<std::pair<reg_t, mem_t*>> res;
        while (true) {
          auto base = strtoull(arg, &p, 0);
          if (!*p || *p != ':')
            out->fatal(CALL_INFO, -1, "Failed to parse memory string" );
          auto size = strtoull(p + 1, &p, 0);
          if ((size | base) % PGSIZE != 0)
            out->fatal(CALL_INFO, -1, "Failed to parse memory string" );
          res.push_back(std::make_pair(reg_t(base), new mem_t(size)));
          if (!*p)
            break;
          if (*p != ',')
            out->fatal(CALL_INFO, -1, "Failed to parse memory string" );
          arg = p + 1;
        }
        return res;
}

void Stake::StakeRequest(uint64_t addr,
                         uint32_t reqLength,
                         bool Read,
                         bool Write,
                         bool Atomic,
                         bool Custom,
                         uint32_t Code ){

        MemoryOpRequest *req = NULL;
        if( Read ){
          req = new MemoryOpRequest( addr, reqLength, READ );
          out->verbose(CALL_INFO, 8, 0,
                       "Issuing READ request for address %" PRIu64 "\n", addr );
        }else if( Write ){
          req = new MemoryOpRequest( addr, reqLength, WRITE );
          out->verbose(CALL_INFO, 8, 0,
                       "Issuing WRITE request for address %" PRIu64 "\n", addr );
        }else if( Atomic ){
          out->verbose(CALL_INFO, 8, 0,
                       "Issuing ATOMIC request for address %" PRIu64 "\n", addr );
        }else if( Custom ){
          out->verbose(CALL_INFO, 8, 0,
                       "Issuing CUSTOM request for address %" PRIu64 "\n", addr );
        }else{
          out->fatal(CALL_INFO, -1, "Unkown request type" );
        }

        MQ->push_back(req);
}

void Stake::generate(MirandaRequestQueue<GeneratorRequest*>* q) {

        // save the request queue for later
        MQ = q;

        // setup the input variables
        std::vector<std::pair<reg_t, mem_t*>> mems = make_mems("2048");
        std::vector<std::string> htif_args;
        std::vector<int> hartids; // null hartid vector

        // initiate the spike simulator
        htif_args.push_back(pk);
        htif_args.push_back(bin);
        spike = new sim_t(isa.c_str(), cores, false, (reg_t)(pc),
                          mems, htif_args, hartids, 2 );

        // setup the pre-runtime parameters
        spike->set_debug(false);
        spike->set_log(log);
        spike->set_histogram(false);
        spike->set_sst_func((void *)(&SST::Miranda::Stake::StakeRequest));

        // run the sim
        rtn = spike->run();
}

bool Stake::isFinished() {
	return done;
}

void Stake::completed() {
}
