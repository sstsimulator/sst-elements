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

#include <sst_config.h>

#include "embermpigen.h"
#include "libs/mpi/emberMPIEvent.h"

using namespace SST;
using namespace SST::Ember;

EmberMessagePassingGenerator::EmberMessagePassingGenerator(
            ComponentId_t id, Params& params, std::string name ) :
    EmberGenerator(id, params, name )
{
    Params mapParams = params.get_scoped_params("rankmap");
    std::string rankMapModule = params.find<std::string>("rankmapper", "ember.LinearMap");

    //NetworkSim: each job has its own custom map, so pass jobId info
    if(!rankMapModule.compare("ember.CustomMap")) {
    	mapParams.insert("_mapjobId", params.find<std::string>("_jobId", "-1"), true);
    }
    //end->NetworkSim

    m_rankMap = loadModule<EmberRankMap>( rankMapModule, mapParams );

    if(NULL == m_rankMap) {
        std::cerr << "Error: Unable to load rank map scheme: \'"
								 << rankMapModule << "\'" << std::endl;
        exit(-1);
    }

    const bool perMotifStats = params.find<bool>("mpiStatsPerMotif", true);

    // Register statistics
    const std::string namePrefix( "time-" );
    for (int i = 0; i < NUM_MPI_EVENTS; i++) {
        std::string statName = namePrefix + std::string( MPIEventNames[i] );
        std::string subId("");
        if ( perMotifStats ) {
            subId = name + "Motif_" + std::to_string(getMotifNum());
        }
        EventTimeStat* stat = registerStatistic<TimeStatDataType>( statName, subId );
        m_mpiTimeStats.push_back( stat );
    }

	m_mpi = static_cast<EmberMpiLib*>( getLib("mpi") );
	assert(m_mpi);
	m_mpi->initOutput( &getOutput() );
	setVerbosePrefix( rank() );
}

EmberMessagePassingGenerator::~EmberMessagePassingGenerator()
{
    verbose( CALL_INFO, 2, 0, "\n" );
}
