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

#ifndef _BASICCLOCKS_H
#define _BASICCLOCKS_H

/*
 * Summary: This example demonstrates registering clocks and handlers.
 *
 * Concepts covered:
 *  - Clocks and clock handlers
 *  - TimeConverters
 *  - UnitAlgebra for error checking
 *
 * Description:
 *  The component registers three clocks with parameterizable frequencies. 
 *  The constructor uses UnitAlgebra to check that each frequency parameter includes units.
 *  Clock0 uses mainTick as its handler. Clock1 and Clock2 use otherTick as their handler 
 *  and pass an id (1 and 2, respectively) that identifies which clock is calling the shared handler.
 *
 * Simulation:
 *  The simulation runs until Clock0 has executed 'clockTicks' cycles. Clock0 prints a message to 
 *  STDOUT ten times during the simulation (see 'printInterval' variable). This notification prints
 *  Clock0's current cycle, the simulation time, and uses Clock1 and Clock2's TimeConverters to print
 *  the current time in terms of Clock1 and Clock2 cycles.
 *  Clock1 and Clock2 print a notification each time their handler is called. After ten ticks, Clock1 and
 *  Clock2 suspend themselves.
 */

#include <sst/core/component.h>

namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class basicClocks : public SST::Component
{
public:

/*
 *  SST Registration macros register Components with the SST Core and 
 *  document their parameters, ports, etc.
 *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
 *  are only required if relevant
 */
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        basicClocks,                        // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "basicClocks",                      // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Basic: managing clocks example",   // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    // Document the parameters that this component accepts
    // { "parameter_name", "description", "default value or NULL if required" }
    SST_ELI_DOCUMENT_PARAMS(
        { "clock0",     "Frequency or period (with units) of clock0",     "1GHz" },
        { "clock1",     "Frequency or period (with units) of clock1",     "5ns" },
        { "clock2",     "Frequency or period (with units) of clock2",     "15ns" },
        { "clockTicks", "Number of clock0 ticks to execute",    "500" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS()
    
    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    // Optional since there is nothing to document - see SubComponent examples for more info
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    basicClocks(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    ~basicClocks();

private:
   
    // Clock handler for clock0
    bool mainTick(SST::Cycle_t cycle);

    // Clock handler for clock1 and clock2
    bool otherTick(SST::Cycle_t cycle, uint32_t id);
    
    // TimeConverters - see timeConverter.h/.cc in sst-core
    // These store a clock interval and can be used to convert between time
    TimeConverter* clock1converter;     // TimeConverter for clock1
    TimeConverter* clock2converter;     // TimeConverter for clock2
    Clock::HandlerBase* clock2Handler; // Clock2 handler (clock2Tick)


    // Params
    Cycle_t cycleCount;
    std::string clock0Freq;
    std::string clock1Freq;
    std::string clock2Freq;

    // SST Output object, for printing, error messages, etc.
    SST::Output* out;
    
    // Number of cycles between print statements in mainTick
    Cycle_t printInterval;
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASICCLOCKS_H */
