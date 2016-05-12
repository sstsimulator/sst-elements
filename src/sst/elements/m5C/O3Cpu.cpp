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
#include <sst/core/params.h>
#include <sst/core/output.h> // Must be loaded before any Gem5 includes (overwrites 'fatal')

#include <cpu/o3/cpu.hh>
#include <cpu/func_unit.hh>
#include <params/FUPool.hh>

#include <m5.h>
#include <system.h>
#include <physical2.h>
#include <process.h>
#include <debug.h>

#include <cpu/simple/atomic.hh>

using namespace SST;
using namespace SST::M5;
using namespace std;

class Component;

namespace SST {
namespace M5 {

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

static void initDerivO3CPUParams( DerivO3CPUParams&, const Params&, System*, 
            SST::Component* );
static void initBaseCPUParams( DerivO3CPUParams& cpu, const Params&, System*,
            SST::Component* );
static FUDesc* newFUDesc( vector<OpDesc*> opV, int count, string name ); 
static OpDesc* newOpDesc( Enums::OpClass opClass, int opLat, 
                                    int issueLat, string name );
static FUPool* newFUPool( string );
template<class type> static type* newTLB( string name, const Params& );
static Trace::InstTracer* newTracer( string name );

}
}

extern "C" {
SimObject* create_O3switchCpu( SST::Component*, string name, Params& sstParams );
SimObject* create_O3Cpu( SST::Component*, string name, Params& sstParams );
}

SimObject* create_O3switchCpu( SST::Component* comp, string name, Params& sstParams )
{

    AtomicSimpleCPUParams*  atomicparams     = new AtomicSimpleCPUParams;
    atomicparams->name ="base"+ name;

    Addr start = sstParams.find_integer( "physicalMemory.start" );
    Addr end = sstParams.find_integer( "physicalMemory.end", 0 );

    DBGC( 1, "%s.physicalMemory.start %#lx\n",name.c_str(),start);
    DBGC( 1, "%s.physicalMemory.end %#lx\n",name.c_str(),end);

    PhysicalMemoryParams& PMparams   = *new PhysicalMemoryParams;
    PMparams.range.start = start;
    PMparams.range.end = end;
    PMparams.name = name + ".physmem";

    System* system = create_System( name + ".system",
                new PhysicalMemory2( & PMparams ), Enums::atomic);

    const Params baseSSTParams = sstParams.find_prefix_params("base.");
    AtomicSimpleCPUParams& BaseParams = *atomicparams;


    // system and physmem are not needed after startup how do free them
    BaseParams.dtb = newTLB<ISA::TLB>( BaseParams.name + ".dtb",
                                    baseSSTParams.find_prefix_params("dtb.") );
    BaseParams.itb = newTLB<ISA::TLB>( BaseParams.name + ".itb",
                                    baseSSTParams.find_prefix_params("itb.") );

    BaseParams.checker = NULL;

    INIT_INT( BaseParams, baseSSTParams, max_insts_all_threads );
    INIT_INT( BaseParams, baseSSTParams, max_insts_any_thread );
    INIT_INT( BaseParams, baseSSTParams, max_loads_all_threads );
    INIT_INT( BaseParams, baseSSTParams, max_loads_any_thread );
#if 0
    INIT_INT( BaseParams, baseSSTParams, max_insts_FF);
#endif

    BaseParams.system = system;

    INIT_CLOCK( BaseParams, baseSSTParams, clock );
    INIT_INT( BaseParams, baseSSTParams, width );
    INIT_INT( BaseParams, baseSSTParams, function_trace_start );
    INIT_INT( BaseParams, baseSSTParams, phase );
    INIT_INT( BaseParams, baseSSTParams, progress_interval );

    BaseParams.tracer = newTracer( BaseParams.name + ".tracer" );

    INIT_INT( BaseParams, baseSSTParams, defer_registration );
    INIT_INT( BaseParams, baseSSTParams, do_checkpoint_insts );
    INIT_INT( BaseParams, baseSSTParams, do_statistics_insts );
    INIT_INT( BaseParams, baseSSTParams, function_trace );
    INIT_INT( BaseParams, baseSSTParams, cpu_id );

    BaseParams.workload.resize(1);
    BaseParams.workload[0] = newProcess( BaseParams.name + ".workload",
                             baseSSTParams.find_prefix_params( "process." ),
                             system, comp );

    BaseParams.numThreads = 1;


    DerivO3CPUParams* o3params = new DerivO3CPUParams;
    DerivO3CPUParams& O3Params = *o3params;

    o3params->name = "o3"+name;

    O3Params.system = system;
    O3Params.clock = BaseParams.clock;
    O3Params.workload.resize(1);
    O3Params.workload[0] = BaseParams.workload[0];
    O3Params.numThreads = 1;
    O3Params.checker = NULL;
    O3Params.fuPool = newFUPool( O3Params.name + ".fuPool" );

    const Params O3SSTParams = sstParams.find_prefix_params( "o3cpu." );

    // system and physmem are not needed after startup how do free them
    O3Params.dtb = newTLB<ISA::TLB>( O3Params.name + ".dtb",
                                    baseSSTParams.find_prefix_params("dtb.") );
    O3Params.itb = newTLB<ISA::TLB>( O3Params.name + ".itb",
                                    baseSSTParams.find_prefix_params("itb.") );

    INIT_INT(O3Params, O3SSTParams, fetchTrapLatency);
    INIT_INT(O3Params, O3SSTParams, trapLatency);
    INIT_INT(O3Params, O3SSTParams, smtIQThreshold);
    INIT_INT(O3Params, O3SSTParams, smtLSQThreshold);
    INIT_INT(O3Params, O3SSTParams, smtROBThreshold);
    INIT_STR(O3Params, O3SSTParams, predType);
    INIT_STR(O3Params, O3SSTParams, smtCommitPolicy);
    INIT_STR(O3Params, O3SSTParams, smtFetchPolicy);
    INIT_STR(O3Params, O3SSTParams, smtIQPolicy);
    INIT_STR(O3Params, O3SSTParams, smtLSQPolicy);
    INIT_STR(O3Params, O3SSTParams, smtROBPolicy);
    INIT_INT(O3Params, O3SSTParams, BTBEntries);
    INIT_INT(O3Params, O3SSTParams, BTBTagSize);
    INIT_INT(O3Params, O3SSTParams, LFSTSize);
    INIT_INT(O3Params, O3SSTParams, LQEntries);
    INIT_INT(O3Params, O3SSTParams, RASSize);
    INIT_INT(O3Params, O3SSTParams, SQEntries);
    INIT_INT(O3Params, O3SSTParams, SSITSize);
    INIT_INT(O3Params, O3SSTParams, store_set_clear_period);
    INIT_INT(O3Params, O3SSTParams, activity);
    INIT_INT(O3Params, O3SSTParams, backComSize);
    INIT_INT(O3Params, O3SSTParams, cachePorts);
    INIT_INT(O3Params, O3SSTParams, choiceCtrBits);
    INIT_INT(O3Params, O3SSTParams, choicePredictorSize);
    INIT_INT(O3Params, O3SSTParams, commitToDecodeDelay);
    INIT_INT(O3Params, O3SSTParams, commitToFetchDelay);
    INIT_INT(O3Params, O3SSTParams, commitToIEWDelay);
    INIT_INT(O3Params, O3SSTParams, commitToRenameDelay);
    INIT_INT(O3Params, O3SSTParams, commitWidth);
    INIT_INT(O3Params, O3SSTParams, decodeToFetchDelay);
    INIT_INT(O3Params, O3SSTParams, decodeToRenameDelay);
    INIT_INT(O3Params, O3SSTParams, decodeWidth);
    INIT_INT(O3Params, O3SSTParams, dispatchWidth);
    INIT_INT(O3Params, O3SSTParams, fetchToDecodeDelay);
    INIT_INT(O3Params, O3SSTParams, fetchWidth);
    INIT_INT(O3Params, O3SSTParams, forwardComSize);
    INIT_INT(O3Params, O3SSTParams, globalCtrBits);
    INIT_INT(O3Params, O3SSTParams, globalHistoryBits);
    INIT_INT(O3Params, O3SSTParams, globalPredictorSize);
    INIT_INT(O3Params, O3SSTParams, iewToCommitDelay);
    INIT_INT(O3Params, O3SSTParams, iewToDecodeDelay);
    INIT_INT(O3Params, O3SSTParams, iewToFetchDelay);
    INIT_INT(O3Params, O3SSTParams, iewToRenameDelay);
    INIT_INT(O3Params, O3SSTParams, instShiftAmt);
    INIT_INT(O3Params, O3SSTParams, issueToExecuteDelay);
    INIT_INT(O3Params, O3SSTParams, issueWidth);
    INIT_INT(O3Params, O3SSTParams, localCtrBits);
    INIT_INT(O3Params, O3SSTParams, localHistoryBits);
    INIT_INT(O3Params, O3SSTParams, localHistoryTableSize);
    INIT_INT(O3Params, O3SSTParams, localPredictorSize);
    INIT_INT(O3Params, O3SSTParams, numIQEntries);
    INIT_INT(O3Params, O3SSTParams, numPhysFloatRegs);
    INIT_INT(O3Params, O3SSTParams, numPhysIntRegs);
    INIT_INT(O3Params, O3SSTParams, numROBEntries);
    INIT_INT(O3Params, O3SSTParams, numRobs);
    INIT_INT(O3Params, O3SSTParams, renameToDecodeDelay);
    INIT_INT(O3Params, O3SSTParams, renameToFetchDelay);
    INIT_INT(O3Params, O3SSTParams, renameToIEWDelay);
    INIT_INT(O3Params, O3SSTParams, renameToROBDelay);
    INIT_INT(O3Params, O3SSTParams, renameWidth);
    INIT_INT(O3Params, O3SSTParams, smtNumFetchingThreads);
    INIT_INT(O3Params, O3SSTParams, squashWidth);
    INIT_INT(O3Params, O3SSTParams, wbDepth);
    INIT_INT(O3Params, O3SSTParams, wbWidth);
    INIT_INT(O3Params, O3SSTParams, defer_registration );
    INIT_INT(O3Params, O3SSTParams, max_insts_all_threads );
    INIT_INT(O3Params, O3SSTParams, max_insts_any_thread );
    INIT_INT(O3Params, O3SSTParams, max_loads_all_threads );
    INIT_INT(O3Params, O3SSTParams, max_loads_any_thread );
    INIT_INT(O3Params, O3SSTParams, max_loads_any_thread );
    INIT_INT(O3Params, O3SSTParams, LSQDepCheckShift);
    INIT_BOOL(O3Params, O3SSTParams, LSQCheckLoads );

    return static_cast<SimObject*>(static_cast<void*>(BaseParams.create()));
}




static System* foo( string name, Params& sstParams )
{
    Addr start = sstParams.find_integer( "physicalMemory.start" );
    Addr end = sstParams.find_integer( "physicalMemory.end", 0 );

    DBGC( 1, "%s.physicalMemory.start %#lx\n",name.c_str(),start);
    DBGC( 1, "%s.physicalMemory.end %#lx\n",name.c_str(),end);
     
    PhysicalMemoryParams& PMparams   = *new PhysicalMemoryParams;
    PMparams.range.start = start;
    PMparams.range.end = end;
    PMparams.name = name + ".physmem";
    PMparams.megsMem = PMparams.range.size() / (1024 * 1024);
    PMparams.ddrConfig = sstParams.find_string( "ddrConfig", "" );
    PMparams.tx_q = sstParams.find_integer( "tx_q", 0 );

    return create_System( name + ".system", 
            new PhysicalMemory2( & PMparams ), Enums::timing );
}


SimObject* create_O3Cpu( SST::Component* comp, string name, Params& sstParams )
{
    DerivO3CPUParams*  params     = new DerivO3CPUParams;
    System* system;

    params->name = name;

    std::string systemName = sstParams.find_string( "system" );
    if ( ! systemName.empty() ) {
		SST::M5::M5* m5comp = static_cast<SST::M5::M5*>(comp);
        printf("system `%s`\n",systemName.c_str());
        objectMap_t::iterator it = m5comp->objectMap().find( systemName );
        if ( it == m5comp->objectMap().end() ) {
            // this is a hack
            m5comp->objectMap()[systemName] =  new Gem5Object;
            m5comp->objectMap()[systemName]->memObject = foo(name, sstParams);
        }
        system = static_cast<System*>( m5comp->objectMap()[systemName]->memObject );
    } else {
        printf("default system\n");
        system = foo(name,sstParams);
    }

    // system and physmem are not needed after startup how do free them

    initDerivO3CPUParams( *params, sstParams, system, comp );

    return static_cast<BaseCPU*>(static_cast<void*>(params->create()));
}

namespace SST {
namespace M5 {

static Trace::InstTracer* newTracer( string name )
{
    ExeTracerParams* exeTracer  = new ExeTracerParams;
    exeTracer->name = name + ".tracer";
    return exeTracer->create();
}

static void initBaseCPUParams( DerivO3CPUParams& cpu, const Params& sstParams,
                                    System* system, SST::Component *comp )
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
    cpu.numThreads            = 1;

    std::string processName = sstParams.find_string( "process" );
    if ( ! processName.empty() ) {
		SST::M5::M5* m5comp = static_cast<SST::M5::M5*>(comp);
        printf("process `%s`\n", processName.c_str());
        objectMap_t::iterator it = m5comp->objectMap().find( processName );
        if ( it == m5comp->objectMap().end() ) {
            // this is a hack
            m5comp->objectMap()[processName] = new Gem5Object;
            m5comp->objectMap()[processName]->memObject = 
                        newProcess( cpu.name + ".workload", 
                                    sstParams.find_prefix_params( "process." ),
                                    system, comp );
        }
        cpu.workload[0] = static_cast<Process*>( 
                                    m5comp->objectMap()[processName]->memObject );
    } else {
        printf("default process\n");
        cpu.workload[0] = newProcess( cpu.name + ".workload", 
                                    sstParams.find_prefix_params( "process." ),
                                    system, comp );
    }

}

static void initDerivO3CPUParams( DerivO3CPUParams& cpu, 
          const Params& sstParams, System* system, SST::Component* comp )
{
    initBaseCPUParams( cpu, sstParams.find_prefix_params("base."),
                    system, comp );

    cpu.fuPool = newFUPool( cpu.name + ".fuPool" );

    const Params tmp = sstParams.find_prefix_params( "o3cpu." );

    INIT_INT(cpu,tmp,fetchTrapLatency);
    INIT_INT(cpu,tmp,trapLatency);
    INIT_INT(cpu,tmp,smtIQThreshold);
    INIT_INT(cpu,tmp,smtLSQThreshold);
    INIT_INT(cpu,tmp,smtROBThreshold);
    INIT_STR(cpu,tmp,predType);
    INIT_STR(cpu,tmp,smtCommitPolicy);
    INIT_STR(cpu,tmp,smtFetchPolicy);
    INIT_STR(cpu,tmp,smtIQPolicy);
    INIT_STR(cpu,tmp,smtLSQPolicy);
    INIT_STR(cpu,tmp,smtROBPolicy);
    INIT_INT(cpu,tmp,BTBEntries);
    INIT_INT(cpu,tmp,BTBTagSize);
    INIT_INT(cpu,tmp,LFSTSize);
    INIT_INT(cpu,tmp,LQEntries);
    INIT_INT(cpu,tmp,RASSize);
    INIT_INT(cpu,tmp,SQEntries);
    INIT_INT(cpu,tmp,SSITSize);
    INIT_INT(cpu,tmp,store_set_clear_period);
    INIT_INT(cpu,tmp,activity);
    INIT_INT(cpu,tmp,backComSize);
    INIT_INT(cpu,tmp,cachePorts);
    INIT_INT(cpu,tmp,choiceCtrBits);
    INIT_INT(cpu,tmp,choicePredictorSize);
    INIT_INT(cpu,tmp,commitToDecodeDelay);
    INIT_INT(cpu,tmp,commitToFetchDelay);
    INIT_INT(cpu,tmp,commitToIEWDelay);
    INIT_INT(cpu,tmp,commitToRenameDelay);
    INIT_INT(cpu,tmp,commitWidth);
    INIT_INT(cpu,tmp,decodeToFetchDelay);
    INIT_INT(cpu,tmp,decodeToRenameDelay);
    INIT_INT(cpu,tmp,decodeWidth);
    INIT_INT(cpu,tmp,dispatchWidth);
    INIT_INT(cpu,tmp,fetchToDecodeDelay);
    INIT_INT(cpu,tmp,fetchWidth);
    INIT_INT(cpu,tmp,forwardComSize);
    INIT_INT(cpu,tmp,globalCtrBits);
    INIT_INT(cpu,tmp,globalHistoryBits);
    INIT_INT(cpu,tmp,globalPredictorSize);
    INIT_INT(cpu,tmp,iewToCommitDelay);
    INIT_INT(cpu,tmp,iewToDecodeDelay);
    INIT_INT(cpu,tmp,iewToFetchDelay);
    INIT_INT(cpu,tmp,iewToRenameDelay);
    INIT_INT(cpu,tmp,instShiftAmt);
    INIT_INT(cpu,tmp,issueToExecuteDelay);
    INIT_INT(cpu,tmp,issueWidth);
    INIT_INT(cpu,tmp,localCtrBits);
    INIT_INT(cpu,tmp,localHistoryBits);
    INIT_INT(cpu,tmp,localHistoryTableSize);
    INIT_INT(cpu,tmp,localPredictorSize);
    INIT_INT(cpu,tmp,numIQEntries);
    INIT_INT(cpu,tmp,numPhysFloatRegs);
    INIT_INT(cpu,tmp,numPhysIntRegs);
    INIT_INT(cpu,tmp,numROBEntries);
    INIT_INT(cpu,tmp,numRobs);
    INIT_INT(cpu,tmp,renameToDecodeDelay);
    INIT_INT(cpu,tmp,renameToFetchDelay);
    INIT_INT(cpu,tmp,renameToIEWDelay);
    INIT_INT(cpu,tmp,renameToROBDelay);
    INIT_INT(cpu,tmp,renameWidth);
    INIT_INT(cpu,tmp,smtNumFetchingThreads);
    INIT_INT(cpu,tmp,squashWidth);
    INIT_INT(cpu,tmp,wbDepth);
    INIT_INT(cpu,tmp,wbWidth);
    INIT_INT(cpu,tmp,LSQDepCheckShift);
    INIT_BOOL(cpu,tmp,LSQCheckLoads );
}

static OpDesc* newOpDesc( Enums::OpClass opClass, int opLat, int issueLat,
                                            string name ) 
{
	OpDescParams* opDesc	= new OpDescParams; 

	opDesc->name 		= name;

	opDesc->opClass 	= opClass;
	opDesc->issueLat 	= issueLat;
	opDesc->opLat 		= opLat;

	return opDesc->create();
}

static FUDesc* newFUDesc( vector<OpDesc*> opV, int count, string name )
{
	FUDescParams* fuDesc = new FUDescParams; 

	fuDesc->name	= name;

	fuDesc->count	= count;
	fuDesc->opList	= opV;

	return fuDesc->create();
}

static FUPool* newFUPool( string prefix )
{
	FUPoolParams* fuPool		= new FUPoolParams;

	fuPool->name = "system.cpu.fuPool";

	prefix += ".FUList";

	vector<OpDesc*> opV;
	string fuName;
	string opName; 

	// IntALU  *****************************
	fuName = prefix + "0";
	opV.push_back( newOpDesc( IntAluOp, 1, 1, fuName + ".opList" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 12, fuName) );

	// IntMultDiv  *****************************
	opV.clear();
	fuName = prefix + "1";
	opV.push_back( newOpDesc( IntMultOp, 3, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( IntDivOp, 20, 19, fuName  + ".opList1" ) );

	fuPool->FUList.push_back( newFUDesc( opV, 8, fuName) );

	// FP_ALU  *****************************
	opV.clear();
	fuName = prefix + "2";
	opV.push_back( newOpDesc( FloatAddOp, 1, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( FloatCmpOp, 1, 1, fuName + ".opList1" ) );
	opV.push_back( newOpDesc( FloatCvtOp, 1, 1, fuName + ".opList2" ) );
	opV.push_back( newOpDesc( FloatMultOp, 2, 1, fuName + ".opList3" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 16, fuName) );

	// FP_MultDiv  *****************************
	opV.clear();
	fuName = prefix + "3";
	opV.push_back( newOpDesc( FloatMultOp, 4, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( FloatDivOp, 12, 12, fuName + ".opList1" ) );
	opV.push_back( newOpDesc( FloatSqrtOp, 24, 24, fuName + ".opList2" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 4, fuName) );

    // SIMD_Unit
	opV.clear();
	fuName = prefix + "8";
	opV.push_back( newOpDesc( SimdAddOp, 1, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( SimdAddAccOp, 1, 1, fuName + ".opList1" ) );
	opV.push_back( newOpDesc( SimdAluOp, 1, 1, fuName + ".opList2" ) );
	opV.push_back( newOpDesc( SimdCmpOp, 1, 1, fuName + ".opList3" ) );
	opV.push_back( newOpDesc( SimdCvtOp, 1, 1, fuName + ".opList4" ) );
	opV.push_back( newOpDesc( SimdMiscOp, 1, 1, fuName + ".opList5" ) );
	opV.push_back( newOpDesc( SimdMultOp, 1, 1, fuName + ".opList6" ) );
	opV.push_back( newOpDesc( SimdMultAccOp, 1, 1, fuName + ".opList7" ) );
	opV.push_back( newOpDesc( SimdShiftOp, 1, 1, fuName + ".opList8" ) );
	opV.push_back( newOpDesc( SimdShiftAccOp, 1, 1, fuName + ".opList9" ) );
	opV.push_back( newOpDesc( SimdSqrtOp, 1, 1, fuName + ".opList10" ) );
	opV.push_back( newOpDesc( SimdFloatAddOp, 1, 1, fuName + ".opList11" ) );
	opV.push_back( newOpDesc( SimdFloatAluOp, 1, 1, fuName + ".opList12" ) );
	opV.push_back( newOpDesc( SimdFloatCmpOp, 1, 1, fuName + ".opList13" ) );
	opV.push_back( newOpDesc( SimdFloatCvtOp, 1, 1, fuName + ".opList14" ) );
	opV.push_back( newOpDesc( SimdFloatDivOp, 1, 1, fuName + ".opList15" ) );
	opV.push_back( newOpDesc( SimdFloatMiscOp, 1, 1, fuName + ".opList16" ) );
	opV.push_back( newOpDesc( SimdFloatMultOp, 1, 1, fuName + ".opList17" ) );
	opV.push_back( newOpDesc( SimdFloatMultAccOp, 1, 1, fuName + ".opList18" ));
	opV.push_back( newOpDesc( SimdFloatSqrtOp, 1, 1, fuName + ".opList19" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 4, fuName) );

	// ReadPort *****************************
	opV.clear();
	fuName = prefix + "4";
	opV.push_back( newOpDesc( MemReadOp, 1, 1, fuName + ".opList" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 0, fuName) );


	// WritePort *****************************
	opV.clear();
	fuName = prefix + "5";
	opV.push_back( newOpDesc( MemWriteOp, 1, 1, fuName + ".opList" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 0, fuName) );

	//RdWrPort *****************************
	opV.clear();
	fuName = prefix + "6";
	opV.push_back( newOpDesc( MemReadOp, 1, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( MemWriteOp, 1, 1, fuName + ".opList1" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 8, fuName) );

	// IprPort *****************************
	opV.clear();
	fuName = prefix + "7";
	opV.push_back( newOpDesc( IprAccessOp, 3, 3, fuName + ".opList" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 1, fuName) );

	// FuPool *****************************
	return fuPool->create();
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
