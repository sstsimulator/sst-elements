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

#include "graph.h"

#include <string>
#include <fstream>

namespace SST {
namespace Llyr {

LlyrGraph::LlyrGraph()
{
    vertices = 0;
}

LlyrGraph::~LlyrGraph()
{}

void LlyrGraph::printGraph()
{
    std::map< uint32_t, Vertex >::iterator vertexIterator;
    for(vertexIterator = vertexMap.begin(); vertexIterator != vertexMap.end(); ++vertexIterator)
    {
        std::cout << "\n Adjacency list of vertex "
            << vertexIterator->first << "\n head ";

        std::vector< uint32_t >::iterator it;
        for( it = vertexIterator->second.adjacencyList->begin(); it != vertexIterator->second.adjacencyList->end(); it++)
        {
            std::cout << "-> " << *it;
        }
        std::cout << std::endl;
    }
}

void LlyrGraph::printDot( std::string fileName )
{
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);         //open a file for writing (truncate the current contents)
    if ( !outputFile )                                                   //check to be sure file is open
        std::cerr << "Error opening file.";

    outputFile << "digraph G {" << "\n";

    std::map< uint32_t, Vertex >::iterator vertexIterator;
    for(vertexIterator = vertexMap.begin(); vertexIterator != vertexMap.end(); ++vertexIterator)
    {
        outputFile << vertexIterator->first << "[label=\"";
        outputFile << vertexIterator->second.type;
        outputFile << "\"];\n";
    }

    for(vertexIterator = vertexMap.begin(); vertexIterator != vertexMap.end(); ++vertexIterator)
    {
        std::vector< uint32_t >::iterator it;
        for( it = vertexIterator->second.adjacencyList->begin(); it != vertexIterator->second.adjacencyList->end(); it++)
        {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << *it;
            outputFile << "\n";
        }
    }

    outputFile << "}";
    outputFile.close();
}

uint32_t LlyrGraph::numVertices()
{
    return vertices;
}

void LlyrGraph::addEdge( uint32_t beginVertex, uint32_t endVertex )
{
    std::cout << "add edge:  " << beginVertex << " --> " << endVertex << std::endl;

    vertexMap[beginVertex].adjacencyList->push_back(endVertex);
}

void LlyrGraph::addVertex(uint32_t vertex, std::string type)
{
    std::cout << "add vertex:  " << vertex << std::endl;

    Vertex thisVertex;
    thisVertex.type = type;
    thisVertex.adjacencyList = new std::vector< uint32_t >;

    std::pair< std::map< uint32_t, Vertex >::iterator,bool > retVal;
    retVal = vertexMap.insert( std::pair< uint32_t, Vertex >( vertex, thisVertex) );
    if( retVal.second == false )
    {
        ///TODO
    }

    vertices = vertices + 1;
}



} // namespace LLyr
} // namespace SST
