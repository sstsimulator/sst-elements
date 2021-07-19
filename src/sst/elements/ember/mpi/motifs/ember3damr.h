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


#ifndef _H_EMBER_3D_AMR
#define _H_EMBER_3D_AMR

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>

#include "mpi/embermpigen.h"
#include "ember3damrblock.h"

using namespace SST;

namespace SST {
namespace Ember {

class Ember3DAMRGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        Ember3DAMRGenerator,
        "ember",
        "3DAMRMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Models an adaptive refinement step from MiniAMR",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.verbose",          "Sets the verbosity of the output", "0" },
        {   "arg.nx",               "Sets the size of a block in X", "8" },
        {   "arg.ny",               "Sets the size of a block in Y", "8" },
        {   "arg.nz",               "Sets the size of a block in Z", "8" },
        {   "arg.fieldspercell",    "Sets the number of fields per mesh cell", "8" },
        {   "arg.blockfile",        "File containing the 3D AMR blocks (from MiniAMR)",     "blocks.amr"},
        {   "arg.filetype",         "Mesh file type, set to \'binary\' or \'text\'", "text" },
        {   "arg.printmap",         "Prints a map of blocks to ranks (=\"no\" or \"yes\")", "no" },
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )


public:
	Ember3DAMRGenerator(SST::ComponentId_t, Params& params);
	~Ember3DAMRGenerator();
	void configure();
        bool generate( std::queue<EmberEvent*>& evQ );
	int32_t power3(const uint32_t expon);

	uint32_t power2(uint32_t exponent) const;
        void loadBlocks();

	uint32_t calcBlockID(const uint32_t posX, const uint32_t posY, const uint32_t posZ, const uint32_t level);
        void calcBlockLocation(const uint32_t blockID, const uint32_t blockLevel, uint32_t* posX, uint32_t* posY, uint32_t* posZ);
        bool isBlockLocal(const uint32_t bID) const;
	void postBlockCommunication(std::queue<EmberEvent*>& evQ, int32_t* blockComm, uint32_t* nextReq, const uint32_t faceSize, const uint32_t msgTag,
		const Ember3DAMRBlock* theBlock);
	void aggregateBlockCommunication(const std::vector<Ember3DAMRBlock*>& blocks, std::map<int32_t, uint32_t>& blockToMessageSize);
	void aggregateCommBytes(Ember3DAMRBlock* curBlock, std::map<int32_t, uint32_t>& blockToMessageSize);

private:
	void printBlockMap();

        std::vector<Ember3DAMRBlock*> localBlocks;
	MessageRequest*   requests;
	MessageResponse** responses;

	void* blockMessageBuffer;

//        static std::map<uint32_t, int32_t>  blockToNodeMap;
        char* blockFilePath;

	Output* out;

	uint32_t iteration;
	uint32_t maxIterations;
	uint32_t nextBlockToBeProcessed;
	uint32_t nextRequestID;
	uint32_t maxRequestCount;

        uint32_t blockCount;
        uint32_t maxLevel;
        uint32_t blocksX;
        uint32_t blocksY;
        uint32_t blocksZ;

        // Share these over all instances of the motif
        uint32_t peX;
        uint32_t peY;
        uint32_t peZ;

        uint32_t nsCompute;
        uint32_t nsCopyTime;

        uint32_t blockNx;
        uint32_t blockNy;
        uint32_t blockNz;

        uint32_t items_per_cell;
        uint32_t sizeof_cell;

        int32_t  x_down;
        int32_t  x_up;
        int32_t  y_down;
        int32_t  y_up;
        int32_t  z_down;
        int32_t  z_up;
	bool printMaps;

    int8_t meshType;

};

}
}

#endif
