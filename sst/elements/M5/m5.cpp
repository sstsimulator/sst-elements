#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <inttypes.h>
#include <python/swig/pyobject.hh>
#include <sim/simulate.hh>
#include <util.h>
#include <debug.h>
#include <barrier.h>

#include <sim/stat_control.hh>
#include <base/stats/types.hh>
#include <sim/sim_object.hh>
#include <sim/init.cc>
#include <vector>

#include "m5.h"

using namespace SST;

boost::mpi::communicator world;

static void enableDebug( std::string name );
static void print_cpu_usagecounts(usage_counts &perf_usage, int i);

M5::M5( ComponentId_t id, Params_t& params ) :
    IntrospectedComponent( id),
    m_armed( false ),
    m_event( * new Event() ),
    m_exitEvent( NULL ),
    m_numRegisterExits(0),
    m_barrier( new BarrierAction ),
    params (params)  //for power
{
    // M5 variable
    want_info = false;

    // a flag for telling if fastforwarding is used 
    FastForwarding_flag=false;

    if ( params.find( "debug" ) != params.end() ) {
        int level = strtol( params["debug"].c_str(), NULL, 0 );
       
        if ( level ) { 
            _dbg.level( level );
        }
    }
    if ( params.find( "M5debug" ) != params.end() ) {
        enableDebug( params[ "M5debug" ] );
    }

    if ( params.find( "info" ) != params.end() ) {
        if ( params["info"].compare("yes") == 0 ) {
            _info.enable();
        } 
    }

    std::ostringstream tmp;

    tmp << world.rank();
    // libm5 src/base/trace.cc references "RANK"
    setenv("RANK", tmp.str().c_str(), 1 );

    _dbg.prepend(  tmp.str() + ":");
    _info.prepend(  tmp.str() + ":");

//    TRACE_SET_PREFIX( tmp.str() + ":" );

    std::string configFile = "";
    if ( params.find( "configFile" ) != params.end() ) {
      configFile = params["configFile"];
    }

    INFO( "configFile `%s`\n", configFile.c_str() );

    buildConfig( this, "m5", configFile, params );

    setClockFrequency(1000000000000);

    m_tc = registerTimeBase("1ps");
    assert( m_tc );

    m_self = configureSelfLink("self", m_tc,
            new SST::Event::Handler<M5>(this,&M5::selfEvent));

    assert( m_self );

    if ( params.find( "registerExit" ) != params.end() ) {
        if( ! params["registerExit"].compare("yes") ) {
            INFO("registering exit\n");
            IntrospectedComponent::registerExit();
        }
    }

    //
    // for power and thermal modeling
    //
    #ifdef M5_WITH_POWER
	//0: O3, 1: inorder
	if ( params.find( "machine_type" ) != params.end() ) {
	        if ( params["machine_type"].compare("0") == 0 ) 
        	    machineType = 0 ;
		else
		    machineType = 1;
        	
    	}

	frequency = "";
	if ( params.find( "samplingFreq" ) != params.end() ) {
		frequency = params["samplingFreq"];
	}
	// for power introspection
	registerClock(frequency, new Clock::Handler<M5>
		(this, &M5::pushData));
    #endif


    DBGX( 2, "returning\n" );
}

M5::~M5()
{
    // do we really need to cleanup?
    if ( ! m_armed ) {
        delete &m_event;
    }
}

int M5::Setup()
{
   // SimObject construction and initialization is changed in the new version of M5 
    // Once SimObject hierarchy is created following the configuration file
    // The primary steps in this process should be:

    // 1. Call init() on each SimObject:
    //    This provides the initializations that depend on all SimObjects 
    //	  being fully constructed.	

    DBGX( 2, "call initAll\n" );
    initAll();
    
    // 2. Call regStats() on each SimObject:
    //    This is not needed for some of the objects in case we consider collecting the 
    //	  statistics in printAllStats function.

    DBGX( 2, "call regAllStats\n" );
    regAllStats();

    // 3. Call initState() on each SimObject
    //    This provides a hook for state initializations.

    // 4. Call startup() on each SimObject
    //    This is the point where SimObjects that do self-initiated processing 
    //	  should schedule internal events

    DBGX( 2, "call SimStartup\n" );
    SimStartup();

    DBGX( 2, "SimStartup done\n" );

    arm( 0 );

     //
    // for power modeling in SST-M5
    //
    #ifdef M5_WITH_POWER
	power = new Power(getId());

	//set up floorplan and thermal tiles
	power->setChip(params);

	//set up architecture parameters
	power->setTech(getId(), params, CACHE_IL1, McPAT);
	power->setTech(getId(), params, CACHE_DL1, McPAT);
	power->setTech(getId(), params, CACHE_L2, McPAT);
	power->setTech(getId(), params, RF, McPAT);
	power->setTech(getId(), params, EXEU_ALU, McPAT);
	power->setTech(getId(), params, LSQ, McPAT);
	power->setTech(getId(), params, IB, McPAT);
	power->setTech(getId(), params, INST_DECODER, McPAT);
	if (machineType == 0){
	    power->setTech(getId(), params, LOAD_Q, McPAT);
	    power->setTech(getId(), params, RENAME_U, McPAT);
	    power->setTech(getId(), params, SCHEDULER_U, McPAT);
	    power->setTech(getId(), params, BTB, McPAT);
	    power->setTech(getId(), params, BPRED, McPAT);
	}


	//reset all counts to zero
	power->resetCounts(&mycounts);
	power->resetCounts(&tempcounts);

	/*registerIntrospector(pushIntrospector); TODO
	registerMonitor("router_delay", new MonitorPointer<SimTime_t>(&router_totaldelay));
	registerMonitor("local_message", new MonitorPointer<uint64_t>(&num_local_message));
	registerMonitor("current_power", new MonitorPointer<I>(&pdata.currentPower));
	registerMonitor("leakage_power", new MonitorPointer<I>(&pdata.leakagePower));
	registerMonitor("runtime_power", new MonitorPointer<I>(&pdata.runtimeDynamicPower));
	registerMonitor("total_power", new MonitorPointer<I>(&pdata.totalEnergy));
	registerMonitor("peak_power", new MonitorPointer<I>(&pdata.peak));*/
	  

    #endif

    return 0;
}

void M5::registerExit(void)
{
    DBGX(2,"m_numRegisterExits %d\n",m_numRegisterExits);
    if ( m_numRegisterExits == 0) {
        IntrospectedComponent::registerExit();
    } 
    ++m_numRegisterExits;
}

void M5::exit( int status )
{
    DBGX(2,"M5::exit() status=%d\n",status);
    --m_numRegisterExits;
    if ( m_numRegisterExits == 0) {
        ::Event *event = new SimLoopExitEvent("Exit M5", 0);
        mainEventQueue.schedule(event, curTick());
    } 
}

bool M5::catchup( SST::Cycle_t time ) 
{
    DBGX( 5, "SST-time=%lu, M5-time=%lu simulate(%lu)\n",
                                    time, curTick(), time - curTick() ); 

    m_exitEvent = simulate( time - curTick() );

    m_event.cycles = m_event.time - time;

    return ( m_exitEvent );
}

void M5::arm( SST::Cycle_t now )
{
    if ( ! m_armed && ! mainEventQueue.empty() )  {
        m_event.cycles = mainEventQueue.nextTick() - now;
        m_event.time = mainEventQueue.nextTick();

        DBGX( 5, "nextTick=%lu cycles=%lu\n", m_event.time, m_event.cycles );
        assert( ! ( now != 0 && m_event.cycles == 0 ) );

        DBGX( 5, "send %lu\n", now + m_event.cycles );
        m_self->Send( m_event.cycles, &m_event );

        m_armed = true;
    }
}

void M5::selfEvent( SST::Event* )
{
    m_armed = false;

    DBGX( 5, "currentTime=%lu cycles=%lu\n", m_event.time, m_event.cycles );

    if ( ! m_exitEvent ) {
	////std::cout << "m_event.cycles = " << m_event.cycles << std::endl;

        m_exitEvent = simulate( m_event.cycles );
    }

    if ( ! m_exitEvent ) {
        arm( m_event.time );
    }  
    //
    // for fast-forwarding to reache the ROI at a faster speed in real benchmarks
    // the exitcode is set to 100 to distinguish from regular exit
    else if(m_exitEvent->getCode()==100){
	std::cout << "I entered getCode = 100" << std::endl;

	FastForwarding_flag=true;
	SimObject::SST_FF();
	if(simulate()->getCode()>=0){
		unregisterExit();
	}
    }
    else {
        // bug what if we didn't call registerExit()
        unregisterExit();
        INFO( "exiting: time=%lu cause=`%s` code=%d\n", m_event.time,
                m_exitEvent->getCause().c_str(), m_exitEvent->getCode() );
    }
}

static void enableDebug( std::string name )
{
    bool all = false;
    if ( name.find( "all") != std::string::npos) { 
        all = true;
    }
       
    if ( all || name.find( "SST") != std::string::npos) {
        SST_M5_debug = true;
    }
    Trace::enabled = true;

    extern void enableDebugFlags( std::string );
    enableDebugFlags( name );
}

// added to get the usage counts for McPAT power calculation July 1st, 2011
#ifdef M5_WITH_POWER
// Get and push power at a frequency 
bool
M5::pushData(Cycle_t current)
{
	usage_counts McPAT_usage_counts;

    //Get statistics from M5 and store the 
    //values in the structure 
    if(FastForwarding_flag){
    	SimObject::printROIStats(McPAT_usage_counts);
    }
    else{
    	SimObject::printAllStats(McPAT_usage_counts);
   }


	std::cout << " It is time (" <<current << ") to push power " << std::endl;
	for (unsigned int i=0; i<McPAT_usage_counts.total_instructions.size(); i++){
	  printf("\n		Total_instructions	: %ld\n",   (long)McPAT_usage_counts.total_instructions[i]);
    	  printf("		Total_cycles		: %ld\n",   (long)McPAT_usage_counts.total_cycles[i]);
    	  printf("		Idle_cycles		: %ld\n",   (long)McPAT_usage_counts.idle_cycles[i]);
    	  printf("		Busy_cycles		: %ld\n",   (long)McPAT_usage_counts.busy_cycles[i]);
	  //icache
	  mycounts.il1_read[i] = McPAT_usage_counts.icache_read_accesses[i] - 
				McPAT_usage_counts.icache_read_misses[i] - tempcounts.il1_read[i];
  	  mycounts.il1_readmiss[i] = McPAT_usage_counts.icache_read_misses[i] - tempcounts.il1_readmiss[i];
	  std::cout << "il1_read[" << i << "] = " << mycounts.il1_read[i] << std::endl;
	  //dcache
	  mycounts.dl1_read[i] = McPAT_usage_counts.dcache_read_accesses[i] - 
				McPAT_usage_counts.dcache_read_misses[i] - tempcounts.dl1_read[i];
	  mycounts.dl1_readmiss[i] = McPAT_usage_counts.dcache_read_misses[i] - tempcounts.dl1_readmiss[i];
	  mycounts.dl1_write[i] = McPAT_usage_counts.dcache_write_accesses[i] - 
				McPAT_usage_counts.dcache_write_misses[i] - tempcounts.dl1_write[i];
	  mycounts.dl1_writemiss[i] = McPAT_usage_counts.dcache_write_misses[i] - tempcounts.dl1_writemiss[i];
	  //RF
	  mycounts.int_regfile_reads[i] = McPAT_usage_counts.int_regfile_reads[i] - tempcounts.int_regfile_reads[i];
    	  mycounts.int_regfile_writes[i] = McPAT_usage_counts.int_regfile_writes[i] - tempcounts.int_regfile_writes[i]; 
	  mycounts.float_regfile_reads[i] = McPAT_usage_counts.float_regfile_writes[i] - tempcounts.float_regfile_reads[i];
	  mycounts.float_regfile_writes[i] = McPAT_usage_counts.float_regfile_writes[i] - tempcounts.float_regfile_writes[i];
	  //ALU
	  mycounts.alu_access[i] = McPAT_usage_counts.ialu_accesses[i] - tempcounts.alu_access[i];
    	  mycounts.fpu_access[i] = McPAT_usage_counts.fpu_accesses[i] - tempcounts.fpu_access[i] ;
	  //LSQ
	  mycounts.LSQ_read[i] = (McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i])*2 - tempcounts.LSQ_read[i];
	  mycounts.LSQ_write[i] = (McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i])*2 - tempcounts.LSQ_write[i];
	  //IB
	  mycounts.IB_read[i] = McPAT_usage_counts.total_instructions[i] - tempcounts.IB_read[i];
	  mycounts.IB_write[i] = McPAT_usage_counts.total_instructions[i] - tempcounts.IB_write[i];
	  //INST_DECODER
	  mycounts.ID_inst_read[i] = McPAT_usage_counts.total_instructions[i] - tempcounts.ID_inst_read[i];
	  mycounts.ID_operand_read[i] = McPAT_usage_counts.total_instructions[i] - tempcounts.ID_operand_read[i];
	  mycounts.ID_misc_read[i] = McPAT_usage_counts.total_instructions[i] - tempcounts.ID_misc_read[i];
	  /*Below is for O3*/
	 if (machineType == 0){
	  //LOAD_Q
	  mycounts.loadQ_read[i] = McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i] - tempcounts.loadQ_read[i];
	  mycounts.loadQ_write[i] = McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i] - tempcounts.loadQ_write[i];
	  //scheduler_u
	  mycounts.int_win_read[i] = McPAT_usage_counts.inst_window_reads[i] - tempcounts.int_win_read[i];
    	  mycounts.int_win_write[i] = McPAT_usage_counts.inst_window_writes[i] - tempcounts.int_win_write[i];
    	  mycounts.int_win_search[i] = McPAT_usage_counts.inst_window_wakeup_accesses[i] - tempcounts.int_win_search[i];
    	  mycounts.fp_win_read[i] = McPAT_usage_counts.fp_inst_window_reads[i] - tempcounts.fp_win_read[i];
    	  mycounts.fp_win_write[i] = McPAT_usage_counts.fp_inst_window_writes[i] - tempcounts.fp_win_write[i];
    	  mycounts.fp_win_search[i] = McPAT_usage_counts.fp_inst_window_wakeup_accesses[i] - tempcounts.fp_win_search[i];
	  mycounts.ROB_read[i] = McPAT_usage_counts.ROB_reads[i] - tempcounts.ROB_read[i];
	  mycounts.ROB_write[i] = McPAT_usage_counts.ROB_writes[i] - tempcounts.ROB_write[i]; 
	  //rename_u
	  mycounts.iFRAT_read[i] = McPAT_usage_counts.rename_reads[i] - tempcounts.iFRAT_read[i];
   	  mycounts.iFRAT_write[i] = 0; //rename_write
	  mycounts.iFRAT_search[i] = 0;  //N/A
	  mycounts.fFRAT_read[i] = McPAT_usage_counts.fp_rename_reads[i] - tempcounts.fFRAT_read[i] ;  
	  mycounts.fFRAT_write[i] = 0; //fp_rename_write
	  mycounts.fFRAT_search[i] = 0; //N/A
	  mycounts.iRRAT_read[i] = 16*(McPAT_usage_counts.context_switches[i] + McPAT_usage_counts.branch_mispredictions[i]) - tempcounts.iRRAT_read[i];
	  mycounts.iRRAT_write[i] = 0; //rename_write
	  mycounts.fRRAT_read[i] = 16*(McPAT_usage_counts.context_switches[i] + McPAT_usage_counts.branch_mispredictions[i]) - tempcounts.fRRAT_read[i] ;
	  mycounts.fRRAT_write[i] = 0; //rename_write
	  mycounts.ifreeL_read[i] =  McPAT_usage_counts.rename_reads[i] - tempcounts.ifreeL_read[i];
	  mycounts.ifreeL_write[i] = 0; //rename_write
	  mycounts.ffreeL_read[i] =  McPAT_usage_counts.fp_rename_reads[i] - tempcounts.ffreeL_read[i];
	  mycounts.ffreeL_write[i] = 0; //fp_rename_write
	  mycounts.idcl_read[i] = 3*4*4*McPAT_usage_counts.rename_reads[i] - tempcounts.idcl_read[i]; //decodeW = 4 in Clovertown
	  mycounts.fdcl_read[i] = 3*4*4*McPAT_usage_counts.fp_rename_reads[i] - tempcounts.fdcl_read[i]; //fp_issueW = 4 in Clovertown
	  //BTB
	  mycounts.BTB_read[i] = McPAT_usage_counts.BTB_read_accesses[i] - tempcounts.BTB_read[i];
	  mycounts.BTB_write[i] = McPAT_usage_counts.BTB_write_accesses[i] - tempcounts.BTB_write[i];
	  //BPT
	  mycounts.branch_read[i] = McPAT_usage_counts.branch_instructions[i] - tempcounts.branch_read[i];
	  mycounts.branch_write[i] = McPAT_usage_counts.branch_mispredictions[i] + 0.1*McPAT_usage_counts.branch_instructions[i] - tempcounts.branch_write[i];  
	  } //end machineType	
	}

	for (unsigned int i = 0; i < McPAT_usage_counts.l2cache_read_accesses.size(); i++){
	  //L2
	  mycounts.L2_read[i] = McPAT_usage_counts.l2cache_read_accesses[i] - tempcounts.L2_read[i];
	  mycounts.L2_readmiss[i] = McPAT_usage_counts.l2cache_read_misses[i] - tempcounts.L2_readmiss[i];
	  mycounts.L2_write[i] = McPAT_usage_counts.l2cache_write_accesses[i] - tempcounts.L2_write[i];
	  mycounts.L2_writemiss[i] = McPAT_usage_counts.l2cache_write_misses[i] - tempcounts.L2_writemiss[i];
	  ////std:cout << "read_access = " << McPAT_usage_counts.l2cache_read_accesses[i] << ", read_miss = " << 
	  ////			McPAT_usage_counts.l2cache_read_misses[i] << ", temp read = " << tempcounts.L2_read[i] << endl;

	  std::cout << "mycounts.L2_read[" << i << "] = " << mycounts.L2_read[i] << std::endl;

	}



       pdata= power->getPower(this, CACHE_IL1, mycounts);
	pdata= power->getPower(this, CACHE_DL1, mycounts);
	pdata= power->getPower(this, CACHE_L2, mycounts);
	pdata= power->getPower(this, RF, mycounts);
	pdata= power->getPower(this, EXEU_ALU, mycounts);
	pdata= power->getPower(this, LSQ, mycounts);
	pdata= power->getPower(this, IB, mycounts);
	pdata= power->getPower(this, INST_DECODER, mycounts);
	if (machineType == 0){ 
	    pdata= power->getPower(this, LOAD_Q, mycounts);
	    pdata= power->getPower(this, SCHEDULER_U, mycounts);
	    pdata= power->getPower(this, RENAME_U, mycounts);
	    pdata= power->getPower(this, BTB, mycounts);
	    pdata= power->getPower(this, BPRED, mycounts);
	}
       power->compute_temperature(getId());
       regPowerStats(pdata);

      //retrieve statistics during the time step
      for (unsigned int i=0; i<McPAT_usage_counts.total_instructions.size(); i++){
      //icache
	  tempcounts.il1_read[i] = McPAT_usage_counts.icache_read_accesses[i] - 
				McPAT_usage_counts.icache_read_misses[i];
  	  tempcounts.il1_readmiss[i] = McPAT_usage_counts.icache_read_misses[i];
	  //dcache
	  tempcounts.dl1_read[i] = McPAT_usage_counts.dcache_read_accesses[i] - 
				McPAT_usage_counts.dcache_read_misses[i];
	  tempcounts.dl1_readmiss[i] = McPAT_usage_counts.dcache_read_misses[i];
	  tempcounts.dl1_write[i] = McPAT_usage_counts.dcache_write_accesses[i] - 
				McPAT_usage_counts.dcache_write_misses[i];
	  tempcounts.dl1_writemiss[i] = McPAT_usage_counts.dcache_write_misses[i];
	  //RF
	  tempcounts.int_regfile_reads[i] = McPAT_usage_counts.int_regfile_reads[i];
    	  tempcounts.int_regfile_writes[i] = McPAT_usage_counts.int_regfile_writes[i]; 
	  tempcounts.float_regfile_reads[i] = McPAT_usage_counts.float_regfile_writes[i];
	  tempcounts.float_regfile_writes[i] = McPAT_usage_counts.float_regfile_writes[i];
	  //ALU
	  tempcounts.alu_access[i] = McPAT_usage_counts.ialu_accesses[i];
    	  tempcounts.fpu_access[i] = McPAT_usage_counts.fpu_accesses[i];
	  //LSQ
	  tempcounts.LSQ_read[i] = (McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i])*2;
	  tempcounts.LSQ_write[i] = (McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i])*2;
	  //IB
	  tempcounts.IB_read[i] = McPAT_usage_counts.total_instructions[i];
	  tempcounts.IB_write[i] = McPAT_usage_counts.total_instructions[i];
	  //INST_DECODER
	  tempcounts.ID_inst_read[i] = McPAT_usage_counts.total_instructions[i];
	  tempcounts.ID_operand_read[i] = McPAT_usage_counts.total_instructions[i];
	  tempcounts.ID_misc_read[i] = McPAT_usage_counts.total_instructions[i];
	  /*Below is for O3*/
	  if (machineType == 0){
	  //LOAD_Q
	  tempcounts.loadQ_read[i] = McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i];
	  tempcounts.loadQ_write[i] = McPAT_usage_counts.load_instructions[i] + McPAT_usage_counts.store_instructions[i];
	  //scheduler_u
	  tempcounts.int_win_read[i] = McPAT_usage_counts.inst_window_reads[i];
    	  tempcounts.int_win_write[i] = McPAT_usage_counts.inst_window_writes[i];
    	  tempcounts.int_win_search[i] = McPAT_usage_counts.inst_window_wakeup_accesses[i];
    	  tempcounts.fp_win_read[i] = McPAT_usage_counts.fp_inst_window_reads[i];
    	  tempcounts.fp_win_write[i] = McPAT_usage_counts.fp_inst_window_writes[i];
    	  tempcounts.fp_win_search[i] = McPAT_usage_counts.fp_inst_window_wakeup_accesses[i];
	  tempcounts.ROB_read[i] = McPAT_usage_counts.ROB_reads[i];
	  tempcounts.ROB_write[i] = McPAT_usage_counts.ROB_writes[i]; 
	  //rename_u
	  tempcounts.iFRAT_read[i] = McPAT_usage_counts.rename_reads[i];
   	  tempcounts.iFRAT_write[i] = 0; //rename_write
	  tempcounts.iFRAT_search[i] = 0;  //N/A
	  tempcounts.fFRAT_read[i] = McPAT_usage_counts.fp_rename_reads[i];  
	  tempcounts.fFRAT_write[i] = 0; //fp_rename_write
	  tempcounts.fFRAT_search[i] = 0; //N/A
	  tempcounts.iRRAT_read[i] = 16*(McPAT_usage_counts.context_switches[i] + McPAT_usage_counts.branch_mispredictions[i]);
	  tempcounts.iRRAT_write[i] = 0; //rename_write
	  tempcounts.fRRAT_read[i] = 16*(McPAT_usage_counts.context_switches[i] + McPAT_usage_counts.branch_mispredictions[i]);
	  tempcounts.fRRAT_write[i] = 0; //rename_write
	  tempcounts.ifreeL_read[i] =  McPAT_usage_counts.rename_reads[i];
	  tempcounts.ifreeL_write[i] = 0; //rename_write
	  tempcounts.ffreeL_read[i] =  McPAT_usage_counts.fp_rename_reads[i];
	  tempcounts.ffreeL_write[i] = 0; //fp_rename_write
	  tempcounts.idcl_read[i] = 3*4*4*McPAT_usage_counts.rename_reads[i]; //decodeW = 4 in Clovertown
	  tempcounts.fdcl_read[i] = 3*4*4*McPAT_usage_counts.fp_rename_reads[i]; //fp_issueW = 4 in Clovertown
	  //BTB
	  tempcounts.BTB_read[i] = McPAT_usage_counts.BTB_read_accesses[i];
	  tempcounts.BTB_write[i] = McPAT_usage_counts.BTB_write_accesses[i];
	  //BPT
	  tempcounts.branch_read[i] = McPAT_usage_counts.branch_instructions[i];
	  tempcounts.branch_write[i] = McPAT_usage_counts.branch_mispredictions[i] + 0.1*McPAT_usage_counts.branch_instructions[i];  
	  } //end machineType	
	}

	for (unsigned int i = 0; i < McPAT_usage_counts.l2cache_read_accesses.size(); i++){
	  //L2
	  tempcounts.L2_read[i] = McPAT_usage_counts.l2cache_read_accesses[i];
	  tempcounts.L2_readmiss[i] = McPAT_usage_counts.l2cache_read_misses[i];
	  tempcounts.L2_write[i] = McPAT_usage_counts.l2cache_write_accesses[i];
	  tempcounts.L2_writemiss[i] = McPAT_usage_counts.l2cache_write_misses[i];

	}


      //reset all counts to zero for next power query
      power->resetCounts(&mycounts);
	
#if 1
	using namespace io_interval; std::cout <<"ID " << getId() <<": current total power = " << pdata.currentPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": leakage power = " << pdata.leakagePower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": runtime power = " << pdata.runtimeDynamicPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": total energy = " << pdata.totalEnergy << " J" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": peak power = " << pdata.peak << " W" << std::endl;
#endif


   return false;

} 
#endif


//
// This is for actions after the performance simulation in SST-M5.
// It is added to get the usage counts for McPAT power calculation July 1st, 2011
//

int M5::Finish()
{
    unsigned int i;
    usage_counts McPAT_usage_counts;

    printf("\nPrinting statistic results:\n\n");

     #ifdef M5_WITH_POWER
    power->compute_MTTF();
    #endif

 
    if(FastForwarding_flag){

	cout<<endl<<"FastForwarding"<<endl<<endl;
    	SimObject::printROIStats(McPAT_usage_counts);
    }
    else{

	cout<<endl<<"NOT FastForwarding"<<endl<<endl;
    	SimObject::printAllStats(McPAT_usage_counts);
   }


    for (i=0; i<McPAT_usage_counts.total_instructions.size(); i++){
    	print_cpu_usagecounts(McPAT_usage_counts, i);
    }

    // --- Shared L2cache accesses stats
   for (i=0; i<McPAT_usage_counts.l2cache_read_accesses.size(); i++){

    	printf("\n\n       **************************************************** \n");
   	printf("       *        Performance usage counts for Shared L2 %d    * \n", i);
    	printf("       **************************************************** \n");

    	printf("\n		l2cache_read_accesses	: %ld\n",   (long)McPAT_usage_counts.l2cache_read_accesses[i]);
    	printf("		l2cache_read_misses	: %ld\n",   (long)McPAT_usage_counts.l2cache_read_misses[i]);
    	printf("		l2cache_write_accesses	: %ld\n",   (long)McPAT_usage_counts.l2cache_write_accesses[i]);
    	printf("		l2cache_write_misses	: %ld\n",   (long)McPAT_usage_counts.l2cache_write_misses[i]);
    	printf("		l2cache_conflicts	: %ld\n",   (long)McPAT_usage_counts.l2cache_conflicts[i]);
    }

   
    return 0;
}

static void print_cpu_usagecounts(usage_counts &perf_usage, int i)
{

    printf("\n\n       ********************************************************** \n");
    printf("       *        Performance usage counts for %s     	* \n", perf_usage.cpuname[i].c_str());
    printf("       ********************************************************** \n");


    //McPAT system stats: 
    // --- System stats 
    printf("\n		Total_instructions	: %ld\n",   (long)perf_usage.total_instructions[i]);
    printf("		Total_cycles		: %ld\n",   (long)perf_usage.total_cycles[i]);
    printf("		Idle_cycles		: %ld\n",   (long)perf_usage.idle_cycles[i]);
    printf("		Busy_cycles		: %ld\n",   (long)perf_usage.busy_cycles[i]);

    printf("\n		Load_instructions	: %ld\n",   (long)perf_usage.load_instructions[i]);
    printf("		Store_instructions	: %ld\n",   (long)perf_usage.store_instructions[i]);
    printf("		Branch_instructions	: %ld\n",   (long)perf_usage.branch_instructions[i]);

    // only valid for O3 cpu
    if(perf_usage.branch_mispredictions.size()){

    printf("		Branch_mispredictions	: %ld\n",   (long)perf_usage.branch_mispredictions[i]);
    printf("\n		Committed_instructions	: %ld\n",   (long)perf_usage.committed_instructions[i]);
    printf("		Committed_int_insts	: %ld\n",   (long)perf_usage.committed_int_instructions[i]);
    printf("		Committed_fp_insts	: %ld\n",   (long)perf_usage.committed_fp_instructions[i]);

    }

    // --- Instruction queue stats 
    printf("		IQ_int_instructions	: %ld\n",   (long)perf_usage.int_instructions[i]);
    printf("		IQ_fp_instructions	: %ld\n",   (long)perf_usage.fp_instructions[i]);


    // --- Register file stats
    printf("\n		Int_regfile_reads	: %ld\n",   (long)perf_usage.int_regfile_reads[i]);
    printf("		Int_regfile_writes	: %ld\n",   (long)perf_usage.int_regfile_writes[i]);
    printf("		Float_regfile_reads	: %ld\n",   (long)perf_usage.float_regfile_reads[i]);
    printf("		Float_regfile_writes	: %ld\n",   (long)perf_usage.float_regfile_writes[i]);
    printf("		Function_calls		: %ld\n",   (long)perf_usage.function_calls[i]);

    // only valid for O3 cpu
    if(perf_usage.branch_mispredictions.size()){

    printf("		Context_switches	: %ld\n",   (long)perf_usage.context_switches[i]);

    // --- ROB, BTB, and rename stats
    printf("\n		ROB_reads		: %ld\n",   (long)perf_usage.ROB_reads[i]);
    printf("		ROB_writes		: %ld\n",   (long)perf_usage.ROB_writes[i]);
    printf("		BTB_reads		: %ld\n",   (long)perf_usage.BTB_read_accesses[i]);
    printf("		BTB_writes		: %ld\n",   (long)perf_usage.BTB_write_accesses[i]);
    printf("		Int_rename_reads	: %ld\n",   (long)perf_usage.rename_reads[i]);
    printf("		Float_rename_reads	: %ld\n",   (long)perf_usage.fp_rename_reads[i]);

    }

    // --- ALU accesses stats
    printf("\n		Integer_ALU_accesses	: %ld\n",   (long)perf_usage.ialu_accesses[i]);
    printf("		Float_ALU_accesses	: %ld\n",   (long)perf_usage.fpu_accesses[i]);

    // only valid for O3 cpu
    if(perf_usage.branch_mispredictions.size()){

    printf("		Integer_MUL_accesses	: %ld\n",   (long)perf_usage.mul_accesses[i]);

    // --- IQ accesses stats
    printf("\n		Integer_IQ_reads        : %ld\n",   (long)perf_usage.inst_window_reads[i]);
    printf("		Integer_IQ_writes       : %ld\n",   (long)perf_usage.inst_window_writes[i]);
    printf("		Int_IQ__wakeup_accesses : %ld\n",   (long)perf_usage.inst_window_wakeup_accesses[i]);
    printf("		Float_IQ_reads          : %ld\n",   (long)perf_usage.fp_inst_window_reads[i]);
    printf("		Float_IQ_writes         : %ld\n",   (long)perf_usage.fp_inst_window_writes[i]);
    printf("		FP_IQ_wakeup_accesses	: %ld\n",   (long)perf_usage.fp_inst_window_wakeup_accesses[i]);

    }

    // --- Dcache accesses stats
    printf("\n		dcache_read_accesses	: %ld\n",   (long)perf_usage.dcache_read_accesses[i]);
    printf("		dcache_read_misses	: %ld\n",   (long)perf_usage.dcache_read_misses[i]);
    printf("		dcache_write_accesses	: %ld\n",   (long)perf_usage.dcache_write_accesses[i]);
    printf("		dcache_write_misses	: %ld\n",   (long)perf_usage.dcache_write_misses[i]);
    printf("		dcache_conflicts	: %ld\n",   (long)perf_usage.dcache_conflicts[i]);

    // --- Icache accesses stats
    printf("\n		icache_read_accesses	: %ld\n",   (long)perf_usage.icache_read_accesses[i]);
    printf("		icache_read_misses	: %ld\n",   (long)perf_usage.icache_read_misses[i]);
    printf("		icache_conflicts	: %ld\n",   (long)perf_usage.icache_conflicts[i]);

    // --- L2cache accesses stats
    if(perf_usage.l2cache_read_accesses.size()==perf_usage.dcache_read_accesses.size()){
    // only for private L2
    printf("\n		l2cache_read_accesses    : %ld\n",   (long)perf_usage.l2cache_read_accesses[i]);
    printf("		l2cache_read_misses      : %ld\n",   (long)perf_usage.l2cache_read_misses[i]);
    printf("		l2cache_write_accesses   : %ld\n",   (long)perf_usage.l2cache_write_accesses[i]);
    printf("		l2cache_write_misses     : %ld\n",   (long)perf_usage.l2cache_write_misses[i]);
    printf("		l2cache_conflicts        : %ld\n",   (long)perf_usage.l2cache_conflicts[i]);
    }

}

unsigned freq_to_ticks( std::string val )
{

    unsigned cycles = SST::Simulation::getSimulation()->
                    getTimeLord()->getSimCycles(val,__func__);
 //   printf("%s() %s ticks=%lu\n",__func__,val.c_str(),cycles);
    return cycles;
}

unsigned latency_to_ticks( std::string val )
{

    unsigned cycles = SST::Simulation::getSimulation()->
                    getTimeLord()->getSimCycles(val,__func__);
//    printf("%s() %s ticks=%lu\n",__func__,val.c_str(),cycles);
    return cycles;
}
