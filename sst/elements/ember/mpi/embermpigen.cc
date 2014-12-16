// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
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
    m_printStats( 0 ),
    m_spyplotMode( EMBER_SPYPLOT_NONE ),
	m_data( NULL ),
    m_spyinfo( NULL )
{
    m_printStats = (uint32_t) (params.find_integer("printStats", 0));

    m_histoV.resize( NUM_EVENTS );
    int userBinWidth = 5;
    for ( int i = 0; i < NUM_EVENTS; i++ ) {

        std::string name( m_eventName[i] );
        name += "_bin_width";
        userBinWidth = (uint64_t) params.find_integer(name, userBinWidth);
        std::string histoName( m_eventName[i]);
        histoName += " Time"; 
        m_histoV[i] = new Histogram<uint32_t, uint32_t>(
                                    histoName, userBinWidth );
    }

	// we should have a list that describes the adhoc histograms and 
    // iterator over it but since we currently only have 2 ...
	m_histoM[ "SendSize" ] = new Histogram<uint32_t, uint32_t>(
                                    "Send Sizes in Bytes", userBinWidth ); 
	m_histoM[ "RecvSize" ] = new Histogram<uint32_t, uint32_t>(
                                    "Recv Sizes in Bytes", userBinWidth ); 

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
    for ( int i = 0; i < NUM_EVENTS; i++ ) {
        delete m_histoV[i];
    }
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

void EmberMessagePassingGenerator::finish(const SST::Output* output, 
        uint64_t time ) 
{
    GEN_DBG( 2, "\n");

    if( EMBER_SPYPLOT_NONE != m_spyplotMode) {
	char* filenameBuffer = (char*) malloc(sizeof(char) * PATH_MAX);
	sprintf(filenameBuffer, "ember-%" PRIu32 "-%s-%" PRIu32 ".spy",
		(uint32_t) m_data->motifNum,
		m_name.c_str(),
		(uint32_t) m_data->rank);

        generateSpyplotRank( filenameBuffer );
	free(filenameBuffer);
    }

	//
	// NOTE THAT WE RETURN HERE! BE CAREFUL WHAT YOU PUT AFTER THIS POINT
	// 	
    if ( ! (m_printStats & (uint32_t) 1) ) {
		return;
	}

    output->output("rank %" PRIu32 ": Motif `%s` completed at: %" PRIu64 
                    " ns\n", m_data->rank, m_name.c_str(), time ); 

    for ( int i = 0; i < NUM_EVENTS; i++ ) {

        Histo* histo = m_histoV[i];
        if ( histo->getItemCount() ) {
            output->output("- Histogram of %s:\n", histo->getName() );
            output->output("--> Total time:     %" PRIu32 "\n",
                                                histo->getValuesSummed());
            output->output("--> Item count:     %" PRIu32 "\n",
                                                histo->getItemCount());
            output->output("--> Average:        %" PRIu32 "\n",
        			histo->getItemCount() == 0 ? 0 :
                    (histo->getValuesSummed() / histo->getItemCount()));
            if ( m_printStats > 1 ) {
                printHistogram(output, histo);
            }
        }
    }
	
	std::map<std::string, Histo*>::iterator iter = m_histoM.begin();	

	for ( ; iter != m_histoM.end(); ++iter ) { 
		Histo* histo = iter->second;
        if ( histo->getItemCount() ) {
    	    output->output("- Histogram of %s:\n", histo->getName() );
    	    output->output("--> Total bytes:    %" PRIu32 "\n",
												histo->getValuesSummed());
    	    output->output("--> Item count:     %" PRIu32 "\n",
												histo->getItemCount());
    	    output->output("--> Average:        %" PRIu32 "\n",
            	histo->getItemCount() == 0 ? 0 :
            	(histo->getValuesSummed() / histo->getItemCount()));




#if 0
        //This code calls getCurrentSimTimeNano(). What does this time mean
        //to a motif when it's start time is not 0

        output->output("--> Avr B/W:        %.2f GB/s (Duration of Run)\n",
            (histoSendSizes->getValuesSummed() / (1024.0 * 1024.0 * 1024.0)) / (((double) getCurrentSimTimeNano()) / 1000000000.0));
        output->output("--> Avr B/W:        %.2f GB/s (During Critical Path Send Operations)\n",
            (histoSendSizes->getValuesSummed() / (1024.0 * 1024.0 * 1024.0)) / (((double)
                (histoSend->getValuesSummed() + histoISend->getValuesSummed()) / 1000000000.0)));

        output->output("--> Avr B/W:        %.2f GB/s (Duration of Run)\n",
            (histoRecvSizes->getValuesSummed() / (1024.0 * 1024.0 * 1024.0)) / (((double) getCurrentSimTimeNano()) / 1000000000.0));
        output->output("--> Avr B/W:        %.2f GB/s (During Critical Path Recv Operations)\n",
            (histoRecvSizes->getValuesSummed() / (1024.0 * 1024.0 * 1024.0)) / (((double)
                (histoRecv->getValuesSummed() + histoIRecv->getValuesSummed()) / 1000000000.0)));
#endif

    	    if ( m_printStats > 1 ) {
        	    printHistogram(output, histo);
    	    }
        } 
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
