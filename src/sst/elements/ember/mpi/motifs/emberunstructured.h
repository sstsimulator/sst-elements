// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_UNSTRUCTURED
#define _H_EMBER_UNSTRUCTURED

#include <sst/core/component.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberUnstructuredGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberUnstructuredGenerator,
        "ember",
        "UnstructuredMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "NetworkSim: Performs an Unstructured Communication Motif based on an input graph",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.graphfile",        "Name of the file the includes the communication graph",        "-1"},
        {   "arg.p_size",           "Sets the problem size",            "10000"},
        {   "arg.fields_per_cell",  "Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
        {   "arg.datatype_width",   "Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure byte count is correct.", "8"},
        {   "arg.iterations",       "Sets the number of unstructured motif operations to perform",  "1"},
        {   "arg.computetime",      "Sets the number of nanoseconds to compute for",    "0"},
    )

	EmberUnstructuredGenerator( SST::ComponentId_t, Params& params );
	~EmberUnstructuredGenerator() {}
	void configure() override;
	bool generate( std::queue<EmberEvent*>& evQ ) override;

private:
	std::string graphFile;
	uint32_t m_loopIndex;
	uint32_t iterations;
	uint64_t m_startTime;
    uint64_t m_stopTime;


	// Share these over all instances of the motif

	uint32_t p_size; //problem size
	uint32_t items_per_cell;
	uint32_t sizeof_cell;
	uint32_t nsCompute;


	std::vector<std::map<int,int> >* rawCommMap; //raw communication map taken from input graph file
	std::vector<std::map<int,int> >* CommMap; //updated communication map based on the task mapping

	int jobId; //NetworkSim

	std::vector<std::map<int,int> >* readCommFile(std::string fileName, int procsNeeded);
};

//helper class
template <class T>
class MatrixMarketReader2D {
    public:
        MatrixMarketReader2D() { };
        ~MatrixMarketReader2D() { };
        //@ignoreValues: Ignores matrix cell values for coordinate input
        std::vector<T*>* readMatrix(const char* fileName, bool ignoreValues = false);
        int xdim, ydim;
};


}
}

#endif
