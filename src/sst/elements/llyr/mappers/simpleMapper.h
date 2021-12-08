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

#include <stack>
#include <sstream>
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
        "App to HW",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                  LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

private:


};

void SimpleMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][simpleMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    std::queue< uint32_t > nodeQueue;

    //Mark all nodes in the PE graph un-visited
    std::map< uint32_t, Vertex< AppNode > >* app_vertex_map_ = appGraph.getVertexMap();
    typename std::map< uint32_t, Vertex< AppNode > >::iterator appIterator;
    for(appIterator = app_vertex_map_->begin(); appIterator != app_vertex_map_->end(); ++appIterator) {
        appIterator->second.setVisited(0);

        output_->verbose(CALL_INFO, 32, 0, "--Vertex %u -- In Degree %u\n", appIterator->first, appIterator->second.getInDegree());

        if( appIterator->second.getInDegree() == 0 ) {
            nodeQueue.push(appIterator->first);
        }
    }

    //assign new ID to mapped nodes in the graph and track mapping between app and graph
    uint32_t newNodeNum = 1;
    std::map< uint32_t, uint32_t > mapping;
    while( nodeQueue.empty() == 0 ) {
        std::stringstream dataOut;
        uint32_t currentAppNode = nodeQueue.front();
        nodeQueue.pop();

        app_vertex_map_->at(currentAppNode).setVisited(1);
        opType tempOp = app_vertex_map_->at(currentAppNode).getValue().optype_;
        if( tempOp == ADDCONST || tempOp == SUBCONST || tempOp == MULCONST || tempOp == DIVCONST || tempOp == REMCONST) {
            int64_t intConst = std::stoll(app_vertex_map_->at(currentAppNode).getValue().constant_val_);
            addNode( tempOp, intConst, newNodeNum, graphOut, llyr_config );
        } else if( tempOp == LDADDR || tempOp == STADDR ) {
            int64_t intConst = std::stoll(app_vertex_map_->at(currentAppNode).getValue().constant_val_);
            addNode( tempOp, intConst, newNodeNum, graphOut, llyr_config );
        } else {
            addNode( tempOp, newNodeNum, graphOut, llyr_config );
        }

        // create a record of the mapping (new, old)
        [[maybe_unused]] auto retVal = mapping.emplace( currentAppNode, newNodeNum );
        output_->verbose(CALL_INFO, 32, 0, "-- Current %" PRIu32 " New %" PRIu32 "\n", currentAppNode, newNodeNum);
        output_->verbose(CALL_INFO, 32, 0, "Adjacency list of vertex: %" PRIu32 "\n", currentAppNode);

        // add the destination vertices from this node to the node queue
        dataOut << " head";
        std::vector< Edge* >* adjacencyList = app_vertex_map_->at(currentAppNode).getAdjacencyList();
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertex = (*it)->getDestination();

            if( app_vertex_map_->at(destinationVertex).getVisited() == 0 ) {
                dataOut << " -> " << destinationVertex;
                app_vertex_map_->at(destinationVertex).setVisited(1);
                nodeQueue.push(destinationVertex);
            }
            else {
                dataOut << " +> " << destinationVertex;
            }
        }

        newNodeNum = newNodeNum + 1;
        output_->verbose(CALL_INFO, 32, 0, "%s\n", dataOut.str().c_str());
    }

    // insert dummy as node 0 to make BFS easier
    addNode( DUMMY, 0, graphOut, llyr_config );

    // now add the edges
    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map_ = graphOut.getVertexMap();
    typename std::map< uint32_t, Vertex< ProcessingElement* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {

        // lookup the matched app PE
        bool found  = 0;
        uint32_t appPE;
        for( auto it = mapping.begin(); it != mapping.end(); ++it) {

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
        output_->verbose(CALL_INFO, 32, 0, "Vertex %" PRIu32 " -- In Degree %" PRIu32 "\n",
                         vertexIterator->first, vertexIterator->second.getInDegree());

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
        std::stringstream dataOut;

        vertex_map_->at(currentNode).setVisited(1);

        output_->verbose(CALL_INFO, 32, 0, "Adjacency list of vertex: %" PRIu32 "\n", currentNode);
        std::vector< Edge* >* adjacencyList = vertex_map_->at(currentNode).getAdjacencyList();
        ProcessingElement* srcNode;
        ProcessingElement* dstNode;

        //add the destination vertices from this node to the node queue
        dataOut << " head";
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); it++ ) {
            uint32_t destinationVertex = (*it)->getDestination();

            srcNode = vertex_map_->at(currentNode).getValue();
            dstNode = vertex_map_->at(destinationVertex).getValue();

            dataOut << "\n";
            dataOut << "\tsrcNode " << srcNode->getProcessorId() << "(" << srcNode->getOpBinding() << ")\n";
            dataOut << "\tdstNode " << dstNode->getProcessorId() << "(" << dstNode->getOpBinding() << ")\n";
            output_->verbose(CALL_INFO, 32, 0, "%s\n", dataOut.str().c_str());

            srcNode->bindOutputQueue(dstNode);
            dstNode->bindInputQueue(srcNode);

            if( vertex_map_->at(destinationVertex).getVisited() == 0 ) {
                vertex_map_->at(destinationVertex).setVisited(1);
                nodeQueue.push(destinationVertex);
            }
        }

        //FIXME Need to use a fake init on ST for now
        opType tempOp = vertex_map_->at(currentNode).getValue()->getOpBinding();
        if( tempOp == ST ) {
            vertex_map_->at(currentNode).getValue()->queueInit();
        } else if( tempOp == LDADDR || tempOp == STADDR ) {
            vertex_map_->at(currentNode).getValue()->queueInit();
        }
    }

    //FIXME Fake init for now, need to read values from stack
    //Initialize any L/S PEs at the top of the graph
    std::vector< Edge* >* rootAdjacencyList = vertex_map_->at(0).getAdjacencyList();
    for( auto it = rootAdjacencyList->begin(); it != rootAdjacencyList->end(); it++ ) {
        uint32_t destinationVertex = (*it)->getDestination();
        vertex_map_->at(destinationVertex).getValue()->queueInit();
    }

}// mapGraph

}// namespace Llyr
}// namespace SST

#endif // _SIMPLE_MAPPER_H

