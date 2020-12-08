// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLE_MAPPER_H
#define _SIMPLE_MAPPER_H

#include <iostream>

#include "mappers/llyrMapper.h"

namespace SST {
namespace Llyr {

class SimpleMapper : public LlyrMapper
{

public:
    explicit SimpleMapper(Params& params) :
        LlyrMapper() {}
    ~SimpleMapper() { }

    SST_ELI_REGISTER_MODULE_DERIVED(
        SimpleMapper,
        "llyr",
        "mapper.simple",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Greedy subgraph mapper.",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                  LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

private:


};

void SimpleMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][simpleMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    //Dummy node to make BFS easier
    ProcessingElement* tempPE = new DummyProcessingElement(DUMMY, 0, llyr_config);
    graphOut.addVertex( 0, tempPE );

    tempPE = new LoadProcessingElement( LD, 1, llyr_config );
    graphOut.addVertex( 1, tempPE );

    tempPE = new LoadProcessingElement( LD, 2, llyr_config );
    graphOut.addVertex( 2, tempPE );

    tempPE = new IntProcessingElement( ADD, 3, llyr_config );
    graphOut.addVertex( 3, tempPE );

    tempPE = new StoreProcessingElement( ST, 4, llyr_config );
    graphOut.addVertex( 4, tempPE );

    graphOut.addEdge( 0, 1 );
    graphOut.addEdge( 0, 2 );
    graphOut.addEdge( 1, 3 );
    graphOut.addEdge( 2, 3 );
    graphOut.addEdge( 3, 4 );

    std::queue< uint32_t > nodeQueue;

    if( output_->getVerboseLevel() >= 10 ) {
        graphOut.printGraph();
    }

    //Mark all nodes in the PE graph un-visited
    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map_ = graphOut.getVertexMap();
    typename std::map< uint32_t, Vertex< ProcessingElement* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        vertexIterator->second.setVisited(0);
    }

    //Node 0 is a dummy node and is always the entry point
    nodeQueue.push(0);

    //BFS and add input/output edges
    while( nodeQueue.empty() == 0 ) {
        uint32_t currentNode = nodeQueue.front();
        nodeQueue.pop();

        vertex_map_->at(currentNode).setVisited(1);

        std::cout << "\n Adjacency list of vertex " << currentNode << "\n head ";
        std::vector< Edge* >* adjacencyList = vertex_map_->at(currentNode).getAdjacencyList();
        ProcessingElement* srcNode;
        ProcessingElement* dstNode;

        //add the destination vertices from this node to the node queue
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertx = (*it)->getDestination();

            srcNode = vertex_map_->at(currentNode).getType();
            dstNode = vertex_map_->at(destinationVertx).getType();

            srcNode->bindOutputQueue(dstNode);
            dstNode->bindInputQueue(srcNode);

            if( vertex_map_->at(destinationVertx).getVisited() == 0 ) {
                std::cout << " -> " << destinationVertx;
                vertex_map_->at(destinationVertx).setVisited(1);
                nodeQueue.push(destinationVertx);
            }
        }

        vertex_map_->at(currentNode).getType()->fakeInit();
        std::cout << std::endl;
    }

}

}
} // namespace SST

#endif // _SIMPLE_MAPPER_H

