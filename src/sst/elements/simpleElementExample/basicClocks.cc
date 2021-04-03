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


// This include is ***REQUIRED*** 
// for ALL SST implementation files
#include "sst_config.h"

#include "basicClocks.h"


using namespace SST;
using namespace SST::simpleElementExample;

/* 
 * During construction the basicClocks component should prepare for simulation
 * - Read parameters
 * - Register clocks
 */
basicClocks::basicClocks(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    out = new Output("", 1, 0, Output::STDOUT);

    // Get parameters from the Python input
    // Clocks can be specified by frequency or period
    clock0Freq = params.find<std::string>("clock0", "1GHz");
    clock1Freq = params.find<std::string>("clock1", "5ns");
    clock2Freq = params.find<std::string>("clock2", "15ns");
    cycleCount = params.find<Cycle_t>("clockTicks", "500");

    // Use UnitAlgebra to error check the clock parameters
    // Check that all frequencies have time units associated
    UnitAlgebra clock0Freq_ua(clock0Freq);
    UnitAlgebra clock1Freq_ua(clock1Freq);
    UnitAlgebra clock2Freq_ua(clock2Freq);
    
    if (! (clock0Freq_ua.hasUnits("Hz") || clock0Freq_ua.hasUnits("s") ) )
        out->fatal(CALL_INFO, -1, "Error in %s: the 'clock0' parameter needs to have units of Hz or s\n", getName().c_str());
    
    if (! (clock1Freq_ua.hasUnits("Hz") || clock1Freq_ua.hasUnits("s") ) )
        out->fatal(CALL_INFO, -1, "Error in %s: the 'clock1' parameter needs to have units of Hz or s\n", getName().c_str());
    
    if (! (clock2Freq_ua.hasUnits("Hz") || clock2Freq_ua.hasUnits("s") ) )
        out->fatal(CALL_INFO, -1, "Error in %s: the 'clock2' parameter needs to have units of Hz or s\n", getName().c_str());

    
    // Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Register our clocks

    // Main clock (clock 0)
    // Clock can be registered with a string or UnitAlgebra, here we use the string
    registerClock(clock0Freq, new Clock::Handler<basicClocks>(this, &basicClocks::mainTick));
    
    out->output("Registering clock0 at %s\n", clock0Freq.c_str());

    // Second clock, here we'll use the UnitAlgebra to register
    // Clock handler can add a template parameter. In this example clock1 and clock2 share a handler but
    // pass unique IDs in to it to differentiate
    // We also save the registerClock return value (a TimeConverter) so that we can use it later (see mainTick)
    clock1converter = registerClock(clock1Freq_ua, 
            new Clock::Handler<basicClocks, uint32_t>(this, &basicClocks::otherTick, 1));
    
    out->output("Registering clock1 at %s (that's %s or %s if we convert the UnitAlgebra to string)\n",
            clock1Freq.c_str(), clock1Freq_ua.toString().c_str(), clock1Freq_ua.toStringBestSI().c_str());

    // Last clock, as with clock1, the handler has an extra parameter and we save the registerClock return parameter
    Clock::HandlerBase* handler = new Clock::Handler<basicClocks, uint32_t>(this, &basicClocks::otherTick, 2);
    clock2converter = registerClock(clock2Freq, handler);
    
    out->output("Registering clock2 at %s\n", clock2Freq.c_str());

    // This component prints the clock cycles & time every so often so calculate a print interval
    // based on simulation time
    printInterval = cycleCount / 10;
    if (printInterval < 1)
        printInterval = 1;
}


/*
 * Destructor, clean up our output
 */
basicClocks::~basicClocks()
{
    delete out;
}


/* 
 * Main clock (clock0) handler
 * Every 'printInterval' cycles, this handler prints the time & cycle count for all clocks
 * When cycleCount cycles have elapsed, this clock triggers the end of simulation
 */
bool basicClocks::mainTick( Cycle_t cycles)
{
    // Print time in terms of clock cycles every so often
    // Clock0 cycles: Number of elapsed cycles in terms of this clock
    // Clock1 cycles: Number of elapsed cycles in terms of clock1 cycles, use the clock1converter to calculate this
    // Clock2 cycles: Number of elapsed cycles in terms of clock2 cycles, use the clock2converter to calculate this
    // Simulation cycles: Number of elapsed cycles in terms of SST Core cycles, get this from the simulator
    // Simulation ns: Elapsed time in terms of nanoseconds, get this from the simulator
    if (cycles % printInterval == 0) {
        out->output("Clock0 cycles: %" PRIu64 ", Clock1 cycles: %" PRIu64 ", Clock2 cycles: %" PRIu64 ", SimulationCycles: %" PRIu64 ", Simulation ns: %" PRIu64 "\n",
                cycles, getCurrentSimTime(clock1converter), getCurrentSimTime(clock2converter),
                getCurrentSimCycle(), getCurrentSimTimeNano());

    }

    cycleCount--;

    // Check if exit condition is met
    // If so, tell the simulation it can end
    if (cycleCount == 0) {
        primaryComponentOKToEndSim();
        return true;
    } else {
        return false;
    }
}

/*
 * Other clock (clock1 & clock2) handler
 * Let both clocks run for 10 cycles
 */
bool basicClocks::otherTick( Cycle_t cycles, uint32_t id )
{
    out->output("Clock #%d - TICK num: %" PRIu64 "\n", id, cycles);

    if (cycles == 10) {
        return true; // Stop calling this handler after 10 cycles
    } else {
        return false; // Keep calling this handler if it hasn't been 10 cycles yet
    }
}
