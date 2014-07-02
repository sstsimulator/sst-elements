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


#define XML 0

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/timeLord.h>
#include <sst/core/configGraph.h>
#if XML
#include <sst/core/model/sdlmodel.h>
#else
#include <sst/core/model/pymodel.h>
#endif

#include <dll/gem5dll.hh>

#include <util.h>
#include <debug.h>
#include <factory.h>
#include <portLink.h>

namespace SST {
namespace M5 {

struct LinkInfo {
    std::string compName;
    std::string portName;
    int portNum;
};

typedef std::pair< LinkInfo, LinkInfo>      link_t;
typedef std::map< std::string, link_t >     linkMap_t;

//static void printLinkMap( linkMap_t& );
static void connectAll( objectMap_t&, linkMap_t& );
static void createLinks( M5&, Gem5Object_t&, SST::Params& );

objectMap_t buildConfig( SST::M5::M5* comp, std::string name, std::string configFile, SST::Params& params )
{
    objectMap_t     objectMap;
    linkMap_t       linkMap;

    DBGC( 2, "name=`%s` file=`%s`\n", name.c_str(), configFile.c_str() );

#if XML
    SST::SSTSDLModelDefinition sdl = SST::SSTSDLModelDefinition( configFile );
#else
    Config cfg(0,1);
    cfg.model_options = configFile;
    SST::SSTPythonModelDefinition sdl = SST::SSTPythonModelDefinition(SST_INSTALL_PREFIX "/libexec/xmlToPython.py", 0, &cfg);
#endif

    SST::ConfigGraph& graph = *sdl.createConfigGraph();

    Factory factory( comp );

    SST::ConfigLinkMap_t::iterator lmIter;

    for ( lmIter = graph.getLinkMap().begin(); 
                lmIter != graph.getLinkMap().end(); ++lmIter ) {
        SST::ConfigLink& tmp = (*lmIter);
        DBGC(2,"key=%s name=%s\n",tmp.key().c_str(), tmp.name.c_str());

        LinkInfo l0,l1;
        l0.compName = graph.getComponentMap()[ tmp.component[0] ].name.c_str();
        l1.compName = graph.getComponentMap()[ tmp.component[1] ].name.c_str();

        l0.portName = tmp.port[0];
        l1.portName = tmp.port[1];

        l0.portNum = 0;
        l1.portNum = 0;

        linkMap[ tmp.name  ] =  std::make_pair( l0, l1 ); 
    } 
    //printLinkMap( linkMap );

    SST::ConfigComponentMap_t::iterator iter; 

    for ( iter = graph.getComponentMap().begin(); 
            iter != graph.getComponentMap().end(); ++iter ) {
        SST::ConfigComponent& tmp = (*iter);
        DBGC(2,"id=%d %s %s\n",tmp.id, tmp.name.c_str(), 
                                                    tmp.type.c_str());

        SST::Params tmpParams = params.find_prefix_params( tmp.name + "." );
        tmp.params.insert( tmpParams.begin(), tmpParams.end() );
        tmp.params.enableVerify(false);

        Gem5Object_t* simObject = factory.createObject( 
                        name + "." + tmp.name, tmp.type, tmp.params );
        objectMap[ tmp.name.c_str() ] = simObject;  
        
        createLinks( *comp, *(Gem5Object_t*) simObject, tmp.params );
    }

    connectAll( objectMap, linkMap );
    return objectMap;
}

static void createLinks( M5& comp,
                        Gem5Object_t& obj, SST::Params& params ) 
{
    const SST::Params& links = params.find_prefix_params("link.");
        
    int num = 0;
    while ( 1 ) {
        std::stringstream numSS;
        std::string tmp;

        numSS << num;

        tmp = numSS.str() + ".";
            
        SST::Params link = links.find_prefix_params( tmp );

        if ( link.empty() ) {
                return;
        }

        obj.links.push_back(  new PortLink( comp, obj, link ) );
        ++num;
    }
}


//static void printLinkMap( linkMap_t& map  )
//{
//    for ( linkMap_t::iterator iter=map.begin(); iter != map.end(); ++iter ) {
//        printf("link=%s %s<->%s\n",iter->first.c_str(),
//            iter->second.first.compName.c_str(), 
//            iter->second.second.compName.c_str());
//    }
//}

static void connectAll( objectMap_t& objMap, linkMap_t& linkMap  )
{
    linkMap_t::iterator iter; 
    for ( iter=linkMap.begin(); iter != linkMap.end(); ++iter ) {
        DBGC( 2,"connecting %s [%s %s %d]<->[%s %s %d]\n",iter->first.c_str(),
            iter->second.first.compName.c_str(), 
            iter->second.first.portName.c_str(), 
            iter->second.first.portNum, 
            iter->second.second.compName.c_str(),
            iter->second.second.portName.c_str(),
            iter->second.second.portNum);

        std::string portName1 = iter->second.first.portName;
        std::string portName2 = iter->second.second.portName;
        Gem5Object_t* obj1 = objMap[iter->second.first.compName];
        Gem5Object_t* obj2 = objMap[iter->second.second.compName];


        // this is a hack to allow M5 full system mode to work
        if ( ( ! portName1.compare(0,4,"pic." ) ) )
        {
            portName1 = portName1.substr(4); 
        }

        if ( ( ! portName2.compare(0,4,"pic." ) ) ) 
        {
            portName2 = portName2.substr(4); 
        }
        
        if ( ! libgem5::ConnectPorts( (Gem5Object_t*) obj1,
                        portName1,
                        iter->second.first.portNum,
                        (Gem5Object_t*) obj2,
                        portName2,
                        iter->second.second.portNum) ) 
        {
            printf("connectPorts failed\n");
            exit(1);
        }
    } 
}

unsigned freq_to_ticks( std::string val )
{
    unsigned cycles = SST::Simulation::getSimulation()->
                    getTimeLord()->getSimCycles(val,__func__);
    DBGC(2,"%s() %s ticks=%lu\n",__func__,val.c_str(),cycles);
    return cycles;
}

unsigned latency_to_ticks( std::string val )
{

    unsigned cycles = SST::Simulation::getSimulation()->
                    getTimeLord()->getSimCycles(val,__func__);
    DBGC(2,"%s() %s ticks=%lu\n",__func__,val.c_str(),cycles);
    return cycles;
}


}
}

