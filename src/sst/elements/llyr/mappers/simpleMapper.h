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

    std::queue< uint32_t > nodeQueue;

    //-------------- DFS ---------------------------------
    //Mark all nodes in the PE graph un-visited
    std::map< uint32_t, Vertex< opType > >* app_vertex_map_ = appGraph.getVertexMap();
    typename std::map< uint32_t, Vertex< opType > >::iterator appIterator;
    for(appIterator = app_vertex_map_->begin(); appIterator != app_vertex_map_->end(); ++appIterator) {
        appIterator->second.setVisited(0);

//         std::cout << "Vertex " << appIterator->first << " -- In Degree " << appIterator->second.getInDegree();
//         std::cout << std::endl;

        if( appIterator->second.getInDegree() == 0 ) {
            nodeQueue.push(appIterator->first);
        }
    }

    //assign new ID to mapped nodes in the graph and track mapping between app and graph
    uint32_t newNodeNum = 1;
    std::map< uint32_t, uint32_t > mapping;
    while( nodeQueue.empty() == 0 ) {
        uint32_t currentAppNode = nodeQueue.front();
        nodeQueue.pop();

        app_vertex_map_->at(currentAppNode).setVisited(1);
        addNode( app_vertex_map_->at(currentAppNode).getType(), newNodeNum, graphOut, llyr_config );

        // create a record of the mapping (new, old)
        auto retVal = mapping.emplace( currentAppNode, newNodeNum );
//         std::cout << " Current " << currentAppNode << " New " << newNodeNum << std::endl;
//
//         std::cout << "\n Adjacency list of vertex " << currentAppNode << "\n head ";
        std::vector< Edge* >* adjacencyList = app_vertex_map_->at(currentAppNode).getAdjacencyList();

        //add the destination vertices from this node to the node queue
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertex = (*it)->getDestination();

            if( app_vertex_map_->at(destinationVertex).getVisited() == 0 ) {
//                 std::cout << " -> " << destinationVertex;
                app_vertex_map_->at(destinationVertex).setVisited(1);
                nodeQueue.push(destinationVertex);
            }
            else {
//                 std::cout << " +> " << destinationVertex;
            }
        }

        newNodeNum = newNodeNum + 1;
//         std::cout << std::endl;
    }

    // insert dummy as node 0 to make BFS easier
    addNode( DUMMY, 0, graphOut, llyr_config );

    // now add the edges
    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map_ = graphOut.getVertexMap();
    typename std::map< uint32_t, Vertex< ProcessingElement* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {

        // lookup the matched app PE, skipping
        bool found  = 0;
        uint32_t appPE;
        for( auto it = mapping.begin(); it != mapping.end(); ++it) {
//             std::cout << " Looking " << vertexIterator->first << std::endl;
            if( it->second == vertexIterator->first ) {
                found = 1;
                appPE = it->first;
                break;
            }
        }

        if( found == 0 ) {
            continue;
        }

        // iterate through the adjeceny list of the app graph node and find corresponding mapped-graph node
        std::vector< Edge* >* adjacencyList = app_vertex_map_->at(appPE).getAdjacencyList();
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertex = mapping.at((*it)->getDestination());
            graphOut.addEdge( vertexIterator->first, destinationVertex );
        }

        // add edges from the dummy root
//         std::cout << "Vertex " << vertexIterator->first << " -- In Degree " << vertexIterator->second.getInDegree();
//         std::cout << std::endl;
        if( vertexIterator->second.getInDegree() == 0 ) {
            graphOut.addEdge( 0, vertexIterator->first );
        }
    }

    //-------------- BFS ---------------------------------
    //Mark all nodes in the PE graph un-visited
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
            uint32_t destinationVertex = (*it)->getDestination();

            srcNode = vertex_map_->at(currentNode).getType();
            dstNode = vertex_map_->at(destinationVertex).getType();

            srcNode->bindOutputQueue(dstNode);
            dstNode->bindInputQueue(srcNode);

            if( vertex_map_->at(destinationVertex).getVisited() == 0 ) {
                std::cout << " -> " << destinationVertex;
                vertex_map_->at(destinationVertex).setVisited(1);
                nodeQueue.push(destinationVertex);
            }
        }

        vertex_map_->at(currentNode).getType()->fakeInit();
        std::cout << std::endl;
    }

}// mapGraph

}// namespace Llyr
}// namespace SST

#endif // _SIMPLE_MAPPER_H

