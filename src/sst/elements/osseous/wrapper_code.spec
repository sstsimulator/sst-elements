    SST_ELI_DOCUMENT_STATISTICS(
        { "read_requests",        "Statistic counts number of read requests", "requests", 1},   // Name, Desc, Enable Level
        { "write_requests",       "Statistic counts number of write requests", "requests", 1},
        { "read_request_sizes",   "Statistic for size of read requests", "bytes", 1},   // Name, Desc, Enable Level
        { "write_request_sizes",  "Statistic for size of write requests", "bytes", 1},
        { "split_read_requests",  "Statistic counts number of split read requests (requests which come from multiple lines)", "requests", 1},
        { "split_write_requests", "Statistic counts number of split write requests (requests which are split over multiple lines)", "requests", 1},
	    { "flush_requests",       "Statistic counts instructions which perform flushes", "requests", 1},
        { "fence_requests",       "Statistic counts instructions which perform fences", "requests", 1}
    )
    //Parameters will mostly be just frequency/clock in the design. User will mention specifically if there could be other parameters for the RTL design which needs to be configured before runtime.  Don't mix RTL input/control signals with SST parameters. SST parameters of RTL design will make the RTL design/C++ model synchronous with rest of the SST full system.   
	SST_ELI_DOCUMENT_PARAMS(
		{ "ExecFreq", "Clock frequency of RTL design in GHz", "1GHz" },
		{ "maxCycles", "Number of Clock ticks the simulation must atleast execute before halting", "1000" },
        {"memoryinterface", "Interface to memory", "memHierarchy.memInterface"}
	)

    //Default will be single port for communicating with Ariel CPU. Need to see the requirement/use-case of multi-port design and how to incorporate it in our parser tool.  
    SST_ELI_DOCUMENT_PORTS(
        {"CPURtllink", "Link to the Rtlmodel", { "Rtlmodel.RTLEvent", "" } },
        {"RtlCacheLink", "Link to Cache", {"memHierarchy.memInterface" , ""} }
    )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"memmgr", "Memory manager to translate virtual addresses to physical, handle malloc/free, etc.", "SST::RtlComponent::RtlMemoryManager"},
            {"memory", "Interface to the memoryHierarchy (e.g., caches)", "SST::Interfaces::SimpleMem" }
    )

//Placeholder for Boiler Plate code

void init() {
    //Any initialization code or binary/hex file to be loaded to memHierarchy
}

void setup() {
    dut->reset = UInt<1>(1);
    axiport->reset = UInt<1>(1);
	output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

void finish() {
	output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
    free(getBaseDataAddress());
}

void ClockTick() {
    //Code or piece of logic to be executed other than eval() call at every SST Clock 
}

void handleRTLEvent() {
    //Piece of logic other than updating input signals at stimulus based on input_port spec file
}


//Example statstics to be recorded for various events in DUT/SST Element
Statistic<uint64_t>* statReadRequests ClockTick;
Statistic<uint64_t>* statWriteRequests RTLSSTmemEvent; 
Statistic<uint64_t>* statFlushRequests RTLSSTmemEvent;
Statistic<uint64_t>* statFenceRequests AXISSTEvent;
Statistic<uint64_t>* statReadRequestSizes SSTAXIEvent;
Statistic<uint64_t>* statWriteRequestSizes RTLSSTmemEvent;
Statistic<uint64_t>* statSplitReadRequests RTLSSTmemEvent;
Statistic<uint64_t>* statSplitWriteRequests SSTRTLmemEvent;

//Update Inputs based on exteral stimulus such as an event

void RTLEvent::UpdateRtlSignals(void *update_data, Rtlheader* cmodel, uint64_t& cycles) {
    bool* update_rtl_params = (bool*)update_data; 
    update_inp = update_rtl_params[0];
    update_ctrl = update_rtl_params[1];
    update_eval_args = update_rtl_params[2];
    update_registers = update_rtl_params[3];
    verbose = update_rtl_params[4];
    done_reset = update_rtl_params[5];
    sim_done = update_rtl_params[6];
    uint64_t* cycles_ptr = (uint64_t*)(&update_rtl_params[7]);
    sim_cycles = *cycles_ptr;
    cycles = sim_cycles;
    cycles_ptr++;

    fprintf(stderr, "sim_cycles: %" PRIu64, sim_cycles);
    fprintf(stderr, "update_inp: %d", update_inp);
    fprintf(stderr, "update_ctrl: %d", update_ctrl);
    if(update_inp) {
        inp_ptr =  (void*)cycles_ptr; 
        input_sigs(cmodel);
    }

    if(update_ctrl) {
        UInt<4>* rtl_inp_ptr = (UInt<4>*)inp_ptr;
        ctrl_ptr = (void*)(&rtl_inp_ptr[5]);
        control_sigs(cmodel);
    }
}

input_sigs() {
    //Update necessary input signals, at stimulus
}

ctrl_sigs() {
    //Update necessary ctrl signals, at stimulus
}
