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

template<class T>
Vertex<T>::Vertex(T type)
{
    this->type = type;
    visited = 0;
}

template<class T>
T Vertex<T>::getType() const
{
    return type;
}

template <class T>
void Vertex<T>::addEdge( uint32_t endVertex )
{
    adjacencyList->push_back(endVertex);
}

// template<class T>
// LlyrGraph<T>::LlyrGraph()
// {
//     vertices = 0;
// }
//
// template<class T>
// LlyrGraph<T>::~LlyrGraph()
// {}

template<class T>
void LlyrGraph<T>::printGraph()
{
    typename std::map< uint32_t, Vertex<T> >::iterator vertexIterator;
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

template<class T>
void LlyrGraph<T>::printDot( std::string fileName )
{
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);         //open a file for writing (truncate the current contents)
    if ( !outputFile )                                                   //check to be sure file is open
        std::cerr << "Error opening file.";

    outputFile << "digraph G {" << "\n";

    typename std::map< uint32_t, Vertex<T> >::iterator vertexIterator;
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

template<class T>
uint32_t LlyrGraph<T>::numVertices()
{
    return vertices;
}

template<class T>
void LlyrGraph<T>::addEdge( uint32_t beginVertex, uint32_t endVertex )
{
    std::cout << "add edge:  " << beginVertex << " --> " << endVertex << std::endl;

    vertexMap[beginVertex].addEdge(endVertex);
}

template<class T>
void LlyrGraph<T>::addVertex(uint32_t vertex, T type)
{
    std::cout << "add vertex:  " << vertex << std::endl;

    Vertex<T> thisVertex;
    thisVertex.type = type;
    thisVertex.adjacencyList = new std::vector< uint32_t >;

    std::pair< typename std::map< uint32_t, Vertex<T> >::iterator,bool > retVal;
    retVal = vertexMap.insert( std::pair< uint32_t, Vertex<T> >( vertex, thisVertex) );
    if( retVal.second == false )
    {
        ///TODO
    }

    vertices = vertices + 1;
}



} // namespace LLyr
} // namespace SST
