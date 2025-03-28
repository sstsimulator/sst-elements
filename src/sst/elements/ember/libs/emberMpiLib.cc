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


#include <limits.h>
#include "sst_config.h"
#include "libs/emberMpiLib.h"

using namespace SST;
using namespace SST::Ember;
using namespace SST::Statistics;

namespace SST {
namespace Ember {

#define GENERATE_STRING(STRING) #STRING,

const char* MPIEventNames[] = {
    FOREACH_MPI_EVENT(GENERATE_STRING)
};

#undef GENERATE_STRING

}
}


EmberMpiLib::EmberMpiLib( Params& params ) : m_size(0), m_rank(-1), m_backed(false),
    m_spyplotMode( EMBER_SPYPLOT_NONE ), m_spyinfo( NULL )
{
    m_Stats.resize( NUM_MPI_EVENTS, nullptr );

    m_spyplotMode = (uint32_t) params.find("spyplotmode", 0);

    if(m_spyplotMode != EMBER_SPYPLOT_NONE) {
        m_spyinfo = new std::map<int32_t, EmberSpyInfo*>();
    }
}

void EmberMpiLib::completed( const SST::Output* output,
    uint64_t time, std::string motifName, int motifNum )
{
    if ( EMBER_SPYPLOT_NONE != m_spyplotMode ) {
        char* filenameBuffer = (char*) malloc( sizeof(char) * PATH_MAX );
        snprintf( filenameBuffer, sizeof(char) * PATH_MAX,
                  "ember-%" PRIu32 "-%s-%" PRIu32 ".spy",
                  motifNum, motifName.c_str(), (uint32_t)m_rank );
        generateSpyplotRank( filenameBuffer );
        free( filenameBuffer );
    }

    // A motif has just completed, thus clearing the statistics pointers
    // configured by that motif.
    for ( size_t i = 0; i < m_Stats.size(); i++ ) {
        m_Stats[i] = nullptr;
    }
}

// Lets the MPI motif to configure the MPI time statistics to be used
// until the motif completes.
void EmberMpiLib::setEventStatistics(std::vector<Statistic<uint32_t>*>& stats) {
    assert( NUM_MPI_EVENTS == stats.size() );
    m_Stats = stats;
}

void EmberMpiLib::updateSpyplot( RankID remoteRank, size_t bytesSent )
{
    EmberSpyInfo* info = NULL;
    std::map<int32_t, EmberSpyInfo*>::iterator spy_itr;

    spy_itr = m_spyinfo->find(remoteRank);

    if(spy_itr == m_spyinfo->end()) {
        info = new EmberSpyInfo(remoteRank);
        m_spyinfo->insert(std::pair<int32_t, EmberSpyInfo*>(remoteRank, info));
    } else {
        info = spy_itr->second;
    }

    // Add the info for this send into the object
    info->incrementSendCount();
    info->addSendBytes(bytesSent);
}

void EmberMpiLib::generateSpyplotRank(const char* filename)
{
    FILE* spyplotFile = fopen(filename, "wt");
    assert(NULL != spyplotFile);

    std::map<int32_t, EmberSpyInfo*>::iterator spy_itr;

    for(spy_itr = m_spyinfo->begin(); spy_itr != m_spyinfo->end(); spy_itr++) {
        EmberSpyInfo* info = spy_itr->second;

        fprintf(spyplotFile, "%" PRId32 " %" PRIu32 " %" PRIu64 "\n",
            info->getRemoteRank(), info->getSendCount(), info->getBytesSent());
    }

    fclose(spyplotFile);
}
