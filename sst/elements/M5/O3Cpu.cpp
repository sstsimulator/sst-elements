#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <cpu/o3/cpu.hh>
#include <cpu/func_unit.hh>
#include <params/FUPool.hh>

#include <system.h>
#include <dummyPhysicalMemory.h>
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
#else
    #error What ISA
#endif

static void initDerivO3CPUParams( DerivO3CPUParams&, const Params&, System* );
static void initBaseCPUParams( DerivO3CPUParams& cpu, const Params&, System* );
static FUDesc* newFUDesc( vector<OpDesc*> opV, int count, string name ); 
static OpDesc* newOpDesc( Enums::OpClass opClass, int opLat, 
                                    int issueLat, string name );
static FUPool* newFUPool( string );
template<class type> static type* newTLB( string name, const Params& );
static Trace::InstTracer* newTracer( string name );

extern "C" {
SimObject* create_O3Cpu( Component*, string name, Params& sstParams );
}

SimObject* create_O3Cpu( Component*, string name, Params& sstParams )
{
    DerivO3CPUParams*  params     = new DerivO3CPUParams;

    params->name = name;

    Addr start = sstParams.find_integer( "physicalMemory.start" );
    Addr end = sstParams.find_integer( "physicalMemory.end", 0 );

    DBGC( 1, "%s.physicalMemory.start %#lx\n",name.c_str(),start);
    DBGC( 1, "%s.physicalMemory.end %#lx\n",name.c_str(),end);
     
    PhysicalMemory* physmem = 
            create_DummyPhysicalMemory( name + ".dummyPhysical", start, end );

    System* system = create_System( name + ".system", physmem, Enums::timing );

    // system and physmem are not needed after startup how do free them

    initDerivO3CPUParams( *params, sstParams, system );

    return static_cast<BaseCPU*>(static_cast<void*>(params->create()));
}

static Trace::InstTracer* newTracer( string name )
{
    ExeTracerParams* exeTracer  = new ExeTracerParams;
    exeTracer->name = name + ".tracer";
    return exeTracer->create();
}

static void initBaseCPUParams( DerivO3CPUParams& cpu, const Params& sstParams,
                                                    System* system )
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

    INIT_INT( cpu, sstParams, clock );
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
                                    system );

    cpu.numThreads            = 1;

}

static void initDerivO3CPUParams( DerivO3CPUParams& cpu, 
                    const Params& sstParams, System* system)
{
    initBaseCPUParams( cpu, sstParams.find_prefix_params("base."), system );

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
	fuPool->FUList.push_back( newFUDesc( opV, 6, fuName) );

	// IntMultDiv  *****************************
	opV.clear();
	fuName = prefix + "1";
	opV.push_back( newOpDesc( IntMultOp, 3, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( IntDivOp, 20, 19, fuName  + ".opList1" ) );

	fuPool->FUList.push_back( newFUDesc( opV, 2, fuName) );

	// FP_ALU  *****************************
	opV.clear();
	fuName = prefix + "2";
	opV.push_back( newOpDesc( FloatAddOp, 2, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( FloatCmpOp, 2, 1, fuName + ".opList1" ) );
	opV.push_back( newOpDesc( FloatCvtOp, 2, 1, fuName + ".opList2" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 4, fuName) );

	// FP_MultDiv  *****************************
	opV.clear();
	fuName = prefix + "3";
	opV.push_back( newOpDesc( FloatMultOp, 4, 1, fuName + ".opList0" ) );
	opV.push_back( newOpDesc( FloatDivOp, 12, 12, fuName + ".opList1" ) );
	opV.push_back( newOpDesc( FloatSqrtOp, 24, 24, fuName + ".opList2" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 2, fuName) );

	// ReadPort *****************************
	opV.clear();
	fuName = prefix + "4";
	opV.push_back( newOpDesc( MemReadOp, 1, 1, fuName + ".opList" ) );
	fuPool->FUList.push_back( newFUDesc( opV, 2, fuName) );


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
	fuPool->FUList.push_back( newFUDesc( opV, 4, fuName) );

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
