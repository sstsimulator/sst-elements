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

#ifndef _PY_MAPPER_H
#define _PY_MAPPER_H

#include <map>
#include <vector>
#include <iostream>

#include "mappers/llyrMapper.h"

namespace SST {
namespace Llyr {

class PyMapper : public LlyrMapper
{

public:
    explicit PyMapper(Params& params) :
        LlyrMapper() {}
    ~PyMapper() { }

    SST_ELI_REGISTER_MODULE_DERIVED(
        PyMapper,
        "llyr",
        "mapper.py",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "App to HW",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                  LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

private:

    void findAdjacencyList( std::string opList, std::vector< uint32_t >* );
    void printDot( std::string fileName, LlyrGraph< ProcessingElement* >* ) const;

};

void PyMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                            LlyrGraph< ProcessingElement* > &graphOut,
                            LlyrConfig* llyr_config)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][pyMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    std::string fileName = "00_sst.in";
    output_->verbose(CALL_INFO, 1, 0, "Mapping Application Using: %s\n", fileName.c_str());

    std::ifstream inputStream(fileName, std::ios::in);
    if( inputStream.is_open() ) {
        std::string thisLine;

        bool routing = 0;
        std::map< uint32_t, std::vector< uint32_t >* > adjList;
        while( std::getline( inputStream, thisLine ) ) {
            output_->verbose(CALL_INFO, 16, 0, "Parsing:  %s\n", thisLine.c_str());

            // need to seperate the compute from the routing
            if( thisLine.find( "-1" ) != std::string::npos ) {
                routing = 1;
            }

            // first read mapped graph
            // with delimiter ':'
            //      0 -- node mapping
            //      1 -- input label
            //      2 -- input list
            //      3 -- output list
            //      4 -- operation

            std::regex delimiter( ":" );
            std::sregex_token_iterator iterA(thisLine.begin(), thisLine.end(), delimiter, -1);
            std::sregex_token_iterator iterB;
            std::vector<std::string> tokenizedSring( iterA, iterB );

            // clean up strings
            for( auto it = tokenizedSring.begin(); it != tokenizedSring.end(); it++ ) {
                std::cout << "+++ " << *it << std::endl;
                it->erase(remove(it->begin(),it->end(), '\''), it->end());
                it->erase(remove(it->begin(),it->end(), '{'), it->end());
                it->erase(remove(it->begin(),it->end(), '}'), it->end());
                it->erase(remove_if(it->begin(), it->end(), isspace), it->end());
                std::cout << "--- " << *it << std::endl;
            }

            // target node in fabric
            std::uint64_t posA = tokenizedSring[0].find_first_of( "(" );
            std::uint64_t posB = tokenizedSring[0].find_first_of( "," );
            std::uint64_t posC = tokenizedSring[0].find_first_of( ")" );
            uint32_t hardwareVertex = std::stoul( tokenizedSring[0].substr( posA + 1, posB ) );
            output_->verbose(CALL_INFO, 15, 0, "Target Graph Node:  %u\n", hardwareVertex);

            // source node from application
            uint32_t applicationVertex = std::stoul( tokenizedSring[0].substr( posB + 1, posC ) );
            output_->verbose(CALL_INFO, 15, 0, "App Graph Node:  %u\n", applicationVertex);

            // operation
            opType op = getOptype(tokenizedSring[4]);
            output_->verbose(CALL_INFO, 15, 0, "Operation:  %u -- %s\n", op, getOpString(op).c_str());

            // encode node in hardware graph
            if( op != RTR ) {
                std::map< uint32_t, Vertex< AppNode > >* app_vertex_map_ = appGraph.getVertexMap();
                if( op == ADDCONST || op == SUBCONST || op == MULCONST || op == DIVCONST || op == REMCONST) {
                    int64_t intConst = std::stoll(app_vertex_map_->at(applicationVertex).getValue().constant_val_);
                    addNode( op, intConst, hardwareVertex, graphOut, llyr_config );
                } else if( op == LDADDR || op == STADDR ) {
                    int64_t intConst = std::stoll(app_vertex_map_->at(applicationVertex).getValue().constant_val_);
                    addNode( op, intConst, hardwareVertex, graphOut, llyr_config );
                } else {
                    addNode( op, hardwareVertex, graphOut, llyr_config );
                }
            } else {
                routing = 1;
                addNode( op, hardwareVertex, graphOut, llyr_config );
            }

            std::vector< uint32_t >* tempVector = new std::vector< uint32_t >;
            findAdjacencyList( tokenizedSring[3], tempVector);
            std::cout << "Hardware Vertex " << hardwareVertex << std::endl;
            auto newValue = adjList.emplace(hardwareVertex, tempVector);

            // if the node has already been added then it must also be a routing node
            if( newValue.second == false ) {
                std::cout << "FAILED on " << hardwareVertex << std::endl;
            }

            std::cout << "vecIn(" << tempVector->size() << "): ";
            for( auto it = tempVector->begin(); it != tempVector->end(); it++ ) {
                std::cout << *it << ", ";
            }
            std::cout << std::endl;

        }


        std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map_ = graphOut.getVertexMap();
        for( auto it = adjList.begin(); it != adjList.end(); it++ ) {
            ProcessingElement* srcNode = vertex_map_->at(it->first).getValue();

            std::cout << "nodeNum: " << it->first;
            std::cout << " -- numNeighbors: " << it->second->size();
            std::cout << std::endl;
            for( auto innerIt = it->second->begin(); innerIt != it->second->end(); innerIt++ ) {
                ProcessingElement* dstNode = vertex_map_->at(*innerIt).getValue();

                graphOut.addEdge( it->first, *innerIt );

                srcNode->bindOutputQueue(dstNode);
                dstNode->bindInputQueue(srcNode);
            }
        }

        inputStream.close();
    }
    else {
        output_->fatal( CALL_INFO, -1, "Error: Unable to open file\n" );
        exit(0);
    }

    output_->verbose( CALL_INFO, 1, 0, "Mapping complete.\n" );
    hardwareGraph.printDot("llyr_hdwr.dot");
    graphOut.printDot("llyr_mapped.dot");
    printDot("llyr_mapped-py.dot", &graphOut);

}// mapGraph

void PyMapper::findAdjacencyList( std::string opList, std::vector< uint32_t >* vecIn )
{
    opList.erase(remove(opList.begin(),opList.end(), '('), opList.end());
    opList.erase(remove(opList.begin(),opList.end(), ')'), opList.end());
    opList.erase(remove(opList.begin(),opList.end(), '['), opList.end());
    opList.erase(remove(opList.begin(),opList.end(), ']'), opList.end());
    opList.erase(opList.find("operation"), std::string("operation").length());
    std::regex delimiter( "," );
    std::sregex_token_iterator iterA(opList.begin(), opList.end(), delimiter, -1);
    std::sregex_token_iterator iterB;
    std::vector<std::string> tokenizedSring( iterA, iterB );

    std::cout << "***Len of vector " << tokenizedSring.size() << std::endl;
    //
    if( tokenizedSring.size() > 1 ) {
        for( uint32_t i = 0; i < tokenizedSring.size(); i++ ) {
            std::cout << "+++ " << tokenizedSring[i] << std::endl;
            if( i % 2 == 1 ) {
                std::cout << "-- " << tokenizedSring[i] << std::endl;
                vecIn->push_back(std::stoul(tokenizedSring[i]));
            }
        }
    }
}

void PyMapper::printDot( std::string fileName, LlyrGraph< ProcessingElement* >* graphIn ) const
{
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);         //open a file for writing (truncate the current contents)
    if ( !outputFile )                                                   //check to be sure file is open
        std::cerr << "Error opening file.";

    outputFile << "digraph G {" << "\n";

    std::map< uint32_t, Vertex< ProcessingElement* > >* vertex_map = graphIn->getVertexMap();
    std::map< uint32_t, Vertex<ProcessingElement*> >::iterator vertexIterator;
    for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
        outputFile << vertexIterator->first << "[label=\"";
        opType moop = vertexIterator->second.getValue()->getOpBinding();
        outputFile << vertexIterator->first << " - " << getOpString(moop);
        outputFile << "\"];\n";
    }

    for(vertexIterator = vertex_map->begin(); vertexIterator != vertex_map->end(); ++vertexIterator) {
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << (*it)->getDestination();
            outputFile << "\n";
        }
    }

    outputFile << "}";
    outputFile.close();
}

}// namespace Llyr
}// namespace SST

#endif // _PY_MAPPER_H

