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

#ifndef _GEMM_MAPPER_H
#define _GEMM_MAPPER_H

#include <iostream>

#include "mappers/llyrMapper.h"

namespace SST {
namespace Llyr {

class GEMMMapper : public LlyrMapper
{

public:
    explicit GEMMMapper(Params& params) :
        LlyrMapper() {}
    ~GEMMMapper() { }

    SST_ELI_REGISTER_MODULE_DERIVED(
        GEMMMapper,
        "llyr",
        "mapper.gemm",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "GEMM optimized mapper",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                  LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

private:


};

void GEMMMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][gemmMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    //Dummy node to make BFS easier
ProcessingElement* tempPE = new DummyProcessingElement(DUMMY, 0, llyr_config);
graphOut.addVertex( 0, tempPE );

tempPE = new LoadProcessingElement( LD, 1, llyr_config );
graphOut.addVertex( 1, tempPE );

tempPE = new LoadProcessingElement( LD, 2, llyr_config );
graphOut.addVertex( 2, tempPE );

tempPE = new LoadProcessingElement( LD, 3, llyr_config );
graphOut.addVertex( 3, tempPE );

tempPE = new LoadProcessingElement( LD, 4, llyr_config );
graphOut.addVertex( 4, tempPE );

tempPE = new LoadProcessingElement( LD, 5, llyr_config );
graphOut.addVertex( 5, tempPE );

tempPE = new LoadProcessingElement( LD, 6, llyr_config );
graphOut.addVertex( 6, tempPE );

tempPE = new LoadProcessingElement( LD, 7, llyr_config );
graphOut.addVertex( 7, tempPE );

tempPE = new LoadProcessingElement( LD, 8, llyr_config );
graphOut.addVertex( 8, tempPE );

tempPE = new LoadProcessingElement( LD, 9, llyr_config );
graphOut.addVertex( 9, tempPE );

tempPE = new LoadProcessingElement( LD, 10, llyr_config );
graphOut.addVertex( 10, tempPE );

tempPE = new LoadProcessingElement( LD, 11, llyr_config );
graphOut.addVertex( 11, tempPE );

tempPE = new LoadProcessingElement( LD, 12, llyr_config );
graphOut.addVertex( 12, tempPE );

tempPE = new IntProcessingElement( MUL, 13, llyr_config );
graphOut.addVertex( 13, tempPE );

tempPE = new IntProcessingElement( MUL, 14, llyr_config );
graphOut.addVertex( 14, tempPE );

tempPE = new IntProcessingElement( MUL, 15, llyr_config );
graphOut.addVertex( 15, tempPE );

tempPE = new IntProcessingElement( MUL, 16, llyr_config );
graphOut.addVertex( 16, tempPE );

tempPE = new IntProcessingElement( MUL, 17, llyr_config );
graphOut.addVertex( 17, tempPE );

tempPE = new IntProcessingElement( MUL, 18, llyr_config );
graphOut.addVertex( 18, tempPE );

tempPE = new IntProcessingElement( MUL, 19, llyr_config );
graphOut.addVertex( 19, tempPE );

tempPE = new IntProcessingElement( MUL, 20, llyr_config );
graphOut.addVertex( 20, tempPE );

tempPE = new IntProcessingElement( MUL, 21, llyr_config );
graphOut.addVertex( 21, tempPE );

tempPE = new IntProcessingElement( MUL, 22, llyr_config );
graphOut.addVertex( 22, tempPE );

tempPE = new IntProcessingElement( MUL, 23, llyr_config );
graphOut.addVertex( 23, tempPE );

tempPE = new IntProcessingElement( MUL, 24, llyr_config );
graphOut.addVertex( 24, tempPE );

tempPE = new IntProcessingElement( ADD, 25, llyr_config );
graphOut.addVertex( 25, tempPE );

tempPE = new IntProcessingElement( ADD, 26, llyr_config );
graphOut.addVertex( 26, tempPE );

tempPE = new IntProcessingElement( ADD, 27, llyr_config );
graphOut.addVertex( 27, tempPE );

tempPE = new IntProcessingElement( ADD, 28, llyr_config );
graphOut.addVertex( 28, tempPE );

tempPE = new IntProcessingElement( ADD, 29, llyr_config );
graphOut.addVertex( 29, tempPE );

tempPE = new IntProcessingElement( ADD, 30, llyr_config );
graphOut.addVertex( 30, tempPE );

tempPE = new IntProcessingElement( ADD, 31, llyr_config );
graphOut.addVertex( 31, tempPE );

tempPE = new IntProcessingElement( ADD, 32, llyr_config );
graphOut.addVertex( 32, tempPE );

tempPE = new StoreProcessingElement( ST, 33, llyr_config );
graphOut.addVertex( 33, tempPE );

tempPE = new StoreProcessingElement( ST, 34, llyr_config );
graphOut.addVertex( 34, tempPE );

tempPE = new StoreProcessingElement( ST, 35, llyr_config );
graphOut.addVertex( 35, tempPE );

tempPE = new StoreProcessingElement( ST, 36, llyr_config );
graphOut.addVertex( 36, tempPE );

graphOut.addEdge( 0, 1 );
graphOut.addEdge( 0, 2 );
graphOut.addEdge( 0, 3 );
graphOut.addEdge( 0, 4 );
graphOut.addEdge( 0, 5 );
graphOut.addEdge( 0, 6 );
graphOut.addEdge( 0, 7 );
graphOut.addEdge( 0, 8 );
graphOut.addEdge( 0, 9 );
graphOut.addEdge( 0, 10 );
graphOut.addEdge( 0, 11 );
graphOut.addEdge( 0, 12 );

graphOut.addEdge( 1, 13 );
graphOut.addEdge( 1, 16 );

graphOut.addEdge( 7, 13 );
graphOut.addEdge( 7, 19 );

graphOut.addEdge( 2, 14 );
graphOut.addEdge( 2, 17 );

graphOut.addEdge( 9, 14 );
graphOut.addEdge( 9, 20 );

graphOut.addEdge( 3, 15 );
graphOut.addEdge( 3, 18 );

graphOut.addEdge( 11, 15 );
graphOut.addEdge( 11, 21 );

graphOut.addEdge( 8, 16 );
graphOut.addEdge( 8, 22 );

graphOut.addEdge( 10, 17 );
graphOut.addEdge( 10, 23 );

graphOut.addEdge( 12, 18 );
graphOut.addEdge( 12, 24 );

graphOut.addEdge( 4, 19 );
graphOut.addEdge( 4, 22 );

graphOut.addEdge( 5, 20 );
graphOut.addEdge( 5, 23 );

graphOut.addEdge( 6, 21 );
graphOut.addEdge( 6, 24 );

graphOut.addEdge( 13, 25 );
graphOut.addEdge( 14, 25 );
graphOut.addEdge( 25, 26 );
graphOut.addEdge( 15, 26 );

graphOut.addEdge( 16, 27 );
graphOut.addEdge( 17, 27 );
graphOut.addEdge( 27, 28 );
graphOut.addEdge( 18, 28 );

graphOut.addEdge( 19, 29 );
graphOut.addEdge( 20, 29 );
graphOut.addEdge( 29, 30 );
graphOut.addEdge( 21, 30 );

graphOut.addEdge( 22, 31 );
graphOut.addEdge( 23, 31 );
graphOut.addEdge( 31, 32 );
graphOut.addEdge( 24, 32 );

graphOut.addEdge( 26, 33 );
graphOut.addEdge( 28, 34 );
graphOut.addEdge( 30, 35 );
graphOut.addEdge( 32, 36 );





    std::queue< uint32_t > nodeQueue;

    //debugging
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

        output_->verbose(CALL_INFO, 10, 0, "Adjacency list of vertex:  %" PRIu32 "\n head ", currentNode);
//         std::cout << "\n Adjacency list of vertex " << currentNode << "\n head ";
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
                output_->verbose(CALL_INFO, 10, 0, " -> %" PRIu32, destinationVertx);
//                 std::cout << " -> " << destinationVertx;
                vertex_map_->at(destinationVertx).setVisited(1);
                nodeQueue.push(destinationVertx);
            }
        }

        vertex_map_->at(currentNode).getType()->fakeInit();
        std::cout << std::endl;
    }

}// mapGraph

}// namespace Llyr
}// namespace SST

#endif // _GEMM_MAPPER_H

