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

#include "llyrMapper.h"

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
                  LSQueue* lsqueue, SimpleMem* mem_interface);

private:


};

void GEMMMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LSQueue* lsqueue, SimpleMem* mem_interface)
{
    uint32_t queueDepth = 256;
    //Dummy node to make BFS easier
    ProcessingElement* tempPE = new DummyProcessingElement(DUMMY, 0, 0, lsqueue, mem_interface);
    graphOut.addVertex( 0, tempPE );

    tempPE = new LoadProcessingElement( LD, 1, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 1, tempPE );

    tempPE = new LoadProcessingElement( LD, 2, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 2, tempPE );

    tempPE = new LoadProcessingElement( LD, 3, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 3, tempPE );

    tempPE = new IntProcessingElement( MUL, 4, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 4, tempPE );

    tempPE = new IntProcessingElement( MUL, 5, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 5, tempPE );

    tempPE = new StoreProcessingElement( ST, 6, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 6, tempPE );

    tempPE = new StoreProcessingElement( ST, 7, queueDepth, lsqueue, mem_interface );
    graphOut.addVertex( 7, tempPE );

    graphOut.addEdge( 0, 1 );
    graphOut.addEdge( 0, 2 );
    graphOut.addEdge( 0, 3 );
    graphOut.addEdge( 1, 4 );
    graphOut.addEdge( 3, 4 );
    graphOut.addEdge( 2, 5 );
    graphOut.addEdge( 3, 5 );
    graphOut.addEdge( 4, 6 );
    graphOut.addEdge( 5, 7 );

    std::queue< uint32_t > nodeQueue;

    graphOut.printGraph();

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

#endif // _GEMM_MAPPER_H

