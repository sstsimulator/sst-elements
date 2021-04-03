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

#ifndef _BASICSTATISTICS_H
#define _BASICSTATISTICS_H

/*
 * This example demonstrates use of the SST Statistics infrastructure
 * The component is isolated (no links).
 * The component has a clock and two random number generators. It uses the
 * generators in its clock handler function to create values to add to the
 * statistics.
 *
 * Concepts covered:
 *  - Statistics types
 *  - Statistics with SubIDs
 *  - Obtaining multiple pointers to the same statistic
 *
 */

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/rng/mersenne.h>

namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class basicStatistics : public SST::Component
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
        basicStatistics,                    // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "basicStatistics",                  // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Basic usage of SST Statistics",    // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    // Document the parameters that this component accepts
    // { "parameter_name", "description", "default value or NULL if required" }
    SST_ELI_DOCUMENT_PARAMS(
        { "marsagliaZ", "Seed for the Marsaglia RNG", "42"},
        { "marsagliaW", "Seed for the Marsaglia RNG", "3498"},
        { "mersenne",   "Seed for the Mersenne RNG", "194785"},
        { "run_cycles", "Number of clock cycles to run for", "10000"},
        { "subids", "Number of unique SUBID_statistic instances to create", "5"}
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS( )
    
    // Document the statistic that this component provides
    // { "statistic_name", "description", "units", enable_level }
    SST_ELI_DOCUMENT_STATISTICS( 
        {"UINT32_statistic", "Statistic that records unsigned 32-bit values", "unitless", 1},
        {"UINT32_statistic_duplicate", "Statistic that records unsigned 32-bit values. Multiple stats record values to this statistic.", "unitless", 1},
        {"UINT64_statistic", "Statistic that records unsigned 64-bit values", "unitless", 2},
        {"INT32_statistic",  "Statistic that records signed 32-bit values", "unitless", 3},
        {"INT64_statistic",  "Statistic that records signed 64-bit values", "unitless", 3},
        {"SUBID_statistic", "Statistic to demonstrate SubIDs to obtain multiple instances of the same statistic name. Type is double.", "unitless", 4},
    )

    // Optional since there is nothing to document - see SubComponent examples for more info
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    basicStatistics(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    ~basicStatistics();

private:
    // Clock handler, called on each clock cycle
    virtual bool clockTic(SST::Cycle_t);

    // Parameter
    uint64_t runcycles;

    // Statistics

    // These statistics illustrate different types
    Statistic<uint64_t>* stat_U64;
    Statistic<int32_t>* stat_I32;
    Statistic<int64_t>* stat_I64;
    
    // These statistics illustrate multiple references to the same statistic
    Statistic<uint32_t>* stat_U32;              // Reference to UINT32_statistic_duplicate
    Statistic<uint32_t>* stat_U32_duplicate;    // Another reference to UINT32_statistic_duplicate, but counts different values
    Statistic<uint32_t>* stat_U32_single;       // Reference to UINT32_statistic and counts the same values that stat_U32 does

    // This statistic illustrates the use of SubIDs to dynamically obtain multiple instances of the same statistic name
    std::vector<Statistic<double>*> stat_subid;

    // Random number generator
    SST::RNG::MarsagliaRNG* rng0;
    SST::RNG::MersenneRNG*  rng1;

};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASICSTATISTICS_H */
