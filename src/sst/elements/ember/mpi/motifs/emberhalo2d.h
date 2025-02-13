// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_HALO_2D
#define _H_EMBER_HALO_2D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo2DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberHalo2DGenerator,
        "ember",
         "Halo2DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a 2D halo exchange Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",       "Sets the number of halo2d operations to perform",  "10"},
        {   "arg.computenano",      "Sets the number of nanoseconds to compute for",    "10"},
        {   "arg.messagesizex",     "Sets the message size in X-dimension (in bytes)",  "128"},
        {   "arg.messagesizey",     "Sets the message size in Y-dimension (in bytes)",  "128"},
        {   "arg.computecopy",      "Sets the time spent copying data between messages",    "5"},
        {   "arg.sizex",        "Sets the processor decomposition in Y", "0"},
        {   "arg.sizey",        "Sets the processor decomposition in X", "0"},
    )

    EmberHalo2DGenerator( SST::ComponentId_t id, Params& params );
    void configure() override;
    bool generate( std::queue<EmberEvent*>& evQ ) override;
    void completed( const SST::Output* output, uint64_t ) override;

private:
	uint32_t m_loopIndex;

	uint32_t sizeX;
	uint32_t sizeY;
	uint32_t nsCompute;
	uint32_t nsCopyTime;
	uint32_t messageSizeX;
	uint32_t messageSizeY;
	uint32_t iterations;
	uint32_t messageCount;

	bool sendWest;
	bool sendEast;
	bool sendNorth;
	bool sendSouth;

	int32_t  procEast;
	int32_t  procWest;
	int32_t  procNorth;
	int32_t  procSouth;

};

}
}

#endif
