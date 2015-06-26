// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/component.h>

#include "embermpigen.h"

using namespace SST;
using namespace SST::Ember;

const char* EmberMessagePassingGenerator::m_eventName[] = {
    FOREACH_ENUM(GENERATE_STRING)
};

EmberMessagePassingGenerator::EmberMessagePassingGenerator( 
            Component* owner, Params& params) :
    EmberGenerator(owner, params), 
    m_spyplotMode( EMBER_SPYPLOT_NONE ),
	m_data( NULL ),
    m_spyinfo( NULL )
{
    m_Stats.resize( NUM_EVENTS );
    
    char* nameBuffer = (char*) malloc(sizeof(char) * 256);
    
    for(int i = 0; i < NUM_EVENTS; i++) {
        std::string baseEventName( m_eventName[i] );
        
        sprintf(nameBuffer, "time-%s", baseEventName.c_str());
        m_Stats[i] = registerStatistic<uint32_t>(nameBuffer, "0");
    }
    
    free(nameBuffer);
    
	// we should have a list that describes the adhoc histograms and 
    // iterator over it but since we currently only have 2 ...
	//m_histoM[ "SendSize" ] = new Histogram<uint32_t, uint32_t>(
    //                                "Send Sizes in Bytes", userBinWidth );
	//m_histoM[ "RecvSize" ] = new Histogram<uint32_t, uint32_t>(
    //                                "Recv Sizes in Bytes", userBinWidth );

    Params distribParams = params.find_prefix_params("distribParams.");
    string distribModule = params.find_string("distribModule",
                                                "ember.ConstDistrib");

    m_computeDistrib = dynamic_cast<EmberComputeDistribution*>(
        owner->loadModuleWithComponent(distribModule, owner, distribParams));

    if(NULL == m_computeDistrib) {
        std::cerr << "Error: Unable to load compute distribution: \'" 
                                    << distribModule << "\'" << std::endl;
        exit(-1);
    }

    m_spyplotMode = (uint32_t) params.find_integer("spyplotmode", 0);

    if(m_spyplotMode != EMBER_SPYPLOT_NONE) {
        m_spyinfo = new std::map<int32_t, EmberSpyInfo*>();
    }

    Params mapParams = params.find_prefix_params("rankmap.");
    string rankMapModule = params.find_string("rankmapper", "ember.LinearMap");
    m_rankMap = dynamic_cast<EmberRankMap*>(
			owner->loadModuleWithComponent(rankMapModule, owner, mapParams));
		
    if(NULL == m_rankMap) {
        std::cerr << "Error: Unable to load rank map scheme: \'"
								 << rankMapModule << "\'" << std::endl;
        exit(-1);
    }

}

EmberMessagePassingGenerator::~EmberMessagePassingGenerator()
{
    GEN_DBG( 2, "\n");
//    for ( int i = 0; i < NUM_EVENTS; i++ ) {
//        delete m_Stats[i];
//    }

	if ( m_spyinfo )
		delete m_spyinfo;

	//delete m_histoM[ "SendSize" ];
	//delete m_histoM[ "RecvSize" ];
	delete m_rankMap;
	delete m_computeDistrib;
}

void EmberMessagePassingGenerator::getPosition( int32_t rank, int32_t px, 
	int32_t py, int32_t pz, int32_t* myX, int32_t* myY, int32_t* myZ) 
{
    m_rankMap->getPosition(rank, px, py, pz, myX, myY, myZ);
}

void EmberMessagePassingGenerator::getPosition( int32_t rank, int32_t px,
	int32_t py, int32_t* myX, int32_t* myY) 
{
    m_rankMap->getPosition(rank, px, py, myX, myY);
}

int32_t EmberMessagePassingGenerator::convertPositionToRank( 
	int32_t peX, int32_t peY, int32_t peZ, 
	int32_t posX, int32_t posY, int32_t posZ) 
{
    return m_rankMap->convertPositionToRank(peX, peY, peZ, posX, posY, posZ);
}

void EmberMessagePassingGenerator::completed(const SST::Output* output, 
        uint64_t time ) 
{
    GEN_DBG( 1, "%s\n",m_name.c_str());

    if( EMBER_SPYPLOT_NONE != m_spyplotMode) {
	char* filenameBuffer = (char*) malloc(sizeof(char) * PATH_MAX);
	sprintf(filenameBuffer, "ember-%" PRIu32 "-%s-%" PRIu32 ".spy",
		(uint32_t) m_data->motifNum,
		m_name.c_str(),
		(uint32_t) m_data->rank);

        generateSpyplotRank( filenameBuffer );
	free(filenameBuffer);
    }
}

void EmberMessagePassingGenerator::updateSpyplot(
                            RankID remoteRank, size_t bytesSent) 
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

void EmberMessagePassingGenerator::generateSpyplotRank(const char* filename)
{
    GEN_DBG( 2, "Generating Communications Spyplots (Rank %" PRId32 "\n", 
                                                        m_data->rank);

    FILE* spyplotFile = fopen(filename, "wt");
    assert(NULL != spyplotFile);

    std::map<int32_t, EmberSpyInfo*>::iterator spy_itr;

    for(spy_itr = m_spyinfo->begin(); spy_itr != m_spyinfo->end(); spy_itr++) {
        EmberSpyInfo* info = spy_itr->second;

        fprintf(spyplotFile, "%" PRId32 " %" PRIu32 " %" PRIu64 "\n",
            info->getRemoteRank(), info->getSendCount(), info->getBytesSent());
    }

    fclose(spyplotFile);

    ///////////////////////////////////////////////////////////

    GEN_DBG( 2, "Generating Communications Spyplots completed"
                                " (Rank %" PRId32 "\n", m_data->rank);
}
