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


#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/stake.h>

using namespace SST::Miranda;

Stake *__GStake;

Stake::Stake( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

        // default parameters
        spike = NULL;
        rtn = 0;

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("Stake[@p:@l]: ", verbose, 0, Output::STDOUT);

        log = params.find<bool>("log", false);
        cores = params.find<size_t>("cores", 1);
        msize = params.find<std::string>("mem_size", "2048");
        pc = params.find<uint64_t>("pc", 0x80000000);
        isa = params.find<std::string>("isa", "RV64IMAFDC");
        pk = params.find<std::string>("proxy_kernel", "pk");
        bin = params.find<std::string>("bin", "");
        args = params.find<std::string>("args", "");
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

        out->verbose(CALL_INFO, 1, 0, "RISC-V Cores = %" PRIu64 "\n", cores );
        out->verbose(CALL_INFO, 1, 0, "Starting PC = 0x%" PRIx64 "\n", pc );
        out->verbose(CALL_INFO, 1, 0, "ISA = %s\n", isa.c_str() );
        if( ext.length() > 0 ){
          out->verbose(CALL_INFO, 1, 0, "RoCC Extension = %s\n", ext.c_str() );
        }
        if( extlib.length() > 0 ){
          out->verbose(CALL_INFO, 1, 0, "External Library = %s\n", extlib.c_str() );
        }

        done = false;
        __GStake = this;
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

extern "C" void SR(uint64_t addr,
                   uint32_t reqLength,
                   bool Read,
                   bool Write,
                   bool Atomic,
                   bool Custom,
                   uint32_t Code ){
        __GStake->StakeRequest(addr,reqLength,Read,Write,Atomic,Custom,Code);
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
          req = new MemoryOpRequest( addr, reqLength, READ );
          out->verbose(CALL_INFO, 8, 0,
                       "Issuing ATOMIC request for address %" PRIu64 "\n", addr );
        }else if( Custom ){
          req = new MemoryOpRequest( addr, reqLength, READ );
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
        std::vector<std::pair<reg_t, mem_t*>> mems = make_mems(msize.c_str());
        std::vector<std::string> htif_args;
        std::vector<int> hartids; // null hartid vector

        // initiate the spike simulator
        htif_args.push_back(pk);
        htif_args.push_back(bin);

        // split the cli args into tokens
        auto i = 0;
        auto pos = args.find(' ');
        if( pos == std::string::npos ){
          // single argument
          htif_args.push_back(args.substr(i,args.length()));
        }
        while( pos != std::string::npos ){
          htif_args.push_back(args.substr(i,pos-i));
          i = ++pos;
          pos = args.find(' ',pos);
          if( pos == std::string::npos ){
            htif_args.push_back(args.substr(i,args.length()));
          }
        }

        for( unsigned j=0; j<htif_args.size(); j++ ){
          out->verbose(CALL_INFO, 4, 0, "HTIF_ARGS[%d] = %s\n",j,htif_args[j].c_str() );
        }

        spike = new sim_t(isa.c_str(), cores, false, (reg_t)(pc),
                          mems, htif_args, hartids, 2, 0, false );

        // setup the pre-runtime parameters
        spike->set_debug(false);
        spike->set_log(log);
        spike->set_histogram(false);
        spike->set_sst_func((void *)(&SR));

        // run the sim
        rtn = spike->run();
        out->verbose(CALL_INFO, 4, 0, "COMPLETED STAKE EXECUTION; DIGESTING MEMORY REQUESTS\n" );
        done = true;
}

bool Stake::isFinished() {
	return done;
}

void Stake::completed() {
}
