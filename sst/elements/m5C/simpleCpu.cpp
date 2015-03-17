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

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/output.h> // Must be loaded before any Gem5 includes (overwrites 'fatal')

#include <cpu/simple/timing.hh>
#include <cpu/exetrace.hh>

#include <system.h>
#include <physical2.h>
#include <process.h>
#include <debug.h>

using namespace SST;
using namespace std;

class Component;

#if THE_ISA == SPARC_ISA
    #define ISA SparcISA
    #define TLBParams SparcTLBParams
#elif THE_ISA == ALPHA_ISA
    #define ISA AlphaISA
    #define TLBParams AlphaTLBParams
#elif THE_ISA == X86_ISA
    #define ISA X86ISA
    #define TLBParams X86TLBParams
#else
    #error What ISA
#endif

namespace SST {
namespace M5 {

static void initBaseCPUParams( TimingSimpleCPUParams& cpu,
                             const Params&, System*, SST::Component* );
template<class type> static type* newTLB( string name, const Params& );
static Trace::InstTracer* newTracer( string name );

}
}

extern "C" {
SimObject* create_SimpleCpu( SST::Component*, string name, Params& sstParams );
}

SimObject* create_SimpleCpu( SST::Component* comp, string name, Params& sstParams )
{
    TimingSimpleCPUParams*  params     = new TimingSimpleCPUParams;

    params->name = name;

    Addr start = sstParams.find_integer( "physicalMemory.start" );
    Addr end = sstParams.find_integer( "physicalMemory.end", 0 );

    DBGC( 1, "%s.physicalMemory.start %#lx\n",name.c_str(),start);
    DBGC( 1, "%s.physicalMemory.end %#lx\n",name.c_str(),end);
     
    PhysicalMemoryParams& PMparams   = *new PhysicalMemoryParams;
    PMparams.range.start = start;
    PMparams.range.end = end;
    PMparams.name = name + ".physmem";

    System* system = create_System( name + ".system", 
                new SST::M5::PhysicalMemory2( & PMparams ), Enums::timing );

    // system and physmem are not needed after startup how do free them

	SST::M5::initBaseCPUParams( *params, sstParams.find_prefix_params("base."), system, comp );

    return static_cast<SimObject*>(static_cast<void*>(params->create()));
}

namespace SST {
namespace M5 {


static Trace::InstTracer* newTracer( string name )
{
    ExeTracerParams* exeTracer  = new ExeTracerParams;
    exeTracer->name = name + ".tracer";
    return exeTracer->create();
}

static void initBaseCPUParams( TimingSimpleCPUParams& cpu,
            const Params& sstParams, System* system, SST::Component* comp )
{
    cpu.dtb                   = newTLB<ISA::TLB>( cpu.name + ".dtb", 
                                    sstParams.find_prefix_params("dtb.") );
    cpu.itb                   = newTLB<ISA::TLB>( cpu.name + ".itb", 
                                    sstParams.find_prefix_params("itb.") );

    cpu.checker               = NULL;

    INIT_INT( cpu, sstParams, max_insts_all_threads );
    INIT_INT( cpu, sstParams, max_insts_any_thread );
    INIT_INT( cpu, sstParams, max_loads_all_threads );
    INIT_INT( cpu, sstParams, max_loads_any_thread );

    cpu.system                = system;

    INIT_CLOCK( cpu, sstParams, clock );
    INIT_INT( cpu, sstParams, function_trace_start );
    INIT_INT( cpu, sstParams, phase );
    INIT_INT( cpu, sstParams, progress_interval );

    cpu.tracer                = newTracer( cpu.name + ".tracer" );

    INIT_INT( cpu, sstParams, defer_registration );
    INIT_INT( cpu, sstParams, do_checkpoint_insts );
    INIT_INT( cpu, sstParams, do_statistics_insts );
    INIT_INT( cpu, sstParams, function_trace );
    INIT_INT( cpu, sstParams, cpu_id );

    cpu.workload.resize(1);
    cpu.workload[0]           = newProcess( cpu.name + ".workload", 
                                    sstParams.find_prefix_params( "process." ),
                                    system, comp );

    cpu.numThreads            = 1;

}

template<class type> static type* newTLB( string name, const Params& params )
{
	TLBParams& tlb			= *new TLBParams;

	tlb.name = name;

    INIT_INT( tlb, params, size );

	return tlb.create();
}

}
}
