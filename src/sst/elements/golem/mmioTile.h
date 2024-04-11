// Copyright 2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MMIOTILE_H
#define _MMIOTILE_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/interfaces/stdMem.h>

#include "sst/elements/golem/vroccanalog.h"

#ifdef MANUAL_COMPUTE_ARRAY
	#include "manualMVMComputeArray.h"
#else
	#include "crossSimComputeArray.h"
#endif

namespace SST {
namespace Golem {

class MMIOTile : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(MMIOTile, "golem", "MMIOTile",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Tile wrapper for compute array that exposes an MMIO interface for the array",   // Description
        COMPONENT_CATEGORY_PROCESSOR    // Category
    )

    SST_ELI_DOCUMENT_PARAMS(
    	{"clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"},
        {"verbose",            "Verbosity of outputs", "0"},
        {"mmioAddr" ,          "Address MMIO interface"},
        {"numArrays",          "Number of distinct arrays in the the tile.", "1"},
        {"arrayInputSize",     "Length of input vector. Implies array rows."},
        {"arrayOutputSize",    "Length of output vector. Implies array columns."},
        {"inputOperandSize",   "Size of input operand in bytes.", "4"},
        {"outputOperandSize",  "Size of output operand in bytes.", "4"}
    )

    //Subcomponent for access to the memory heirachy (memNIC) and compute arrays
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        { "memory", "Interface to memory hierarchy", "SST::Interfaces::StandardMem" },
        { "array", "Analog array model", "SST::Golem::ComputeArray"}
    )


    MMIOTile(ComponentId_t id, Params& params);
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void emergencyShutdown() override;

    class StandardMemHandlers : public Interfaces::StandardMem::RequestHandler {
    public:
        friend class MMIOTile;

        StandardMemHandlers(MMIOTile* tile, SST::Output *out) : Interfaces::StandardMem::RequestHandler(out), tile(tile) {}
        virtual ~StandardMemHandlers() {}
        virtual void handle(Interfaces::StandardMem::Read*) override;
        virtual void handle(Interfaces::StandardMem::Write*) override;

        MMIOTile* tile;
    };

private:
    void handleMemEvent( Interfaces::StandardMem::Request *ev );
    void handleArrayEvent( Event *ev);

    SST::Output out;

    // Tile Parameters
    int numArrays;
    int arrayInputSize;
    int arrayOutputSize;
    int inputOperandSize;
    int outputOperandSize;

    // MMIO range delimiters
    uint64_t mmioStartAddr;
    uint64_t inputStartAddr;
    uint64_t outputStartAddr;
    uint64_t inputDataSize;
    uint64_t outputDataSize;
    uint64_t inputTotalSize;
    uint64_t outputTotalSize;

    Interfaces::StandardMem* memory;
    ComputeArray* array;
    StandardMemHandlers* memHandlers;

    //Arrays are either in an idle (0) or computing (1) state
    //Not going to use an enum for now
    std::vector<char> arrayStates;

    //Data structures corrosponding to the physical input and output registers
    std::vector<std::vector<float>> arrayIns;
    std::vector<std::vector<float>> arrayOuts;
    std::vector<std::vector<float>> matrices;

    // Even though MMIOTile does not need a clock the array objects uses the parent TimeConverter
    TimeConverter *clockTC;                 // Clock object

};


} // namespace Golem
} // namespace SST


#endif /* _MMIOTILE_H */
