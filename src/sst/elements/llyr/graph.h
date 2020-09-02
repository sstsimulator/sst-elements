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


#ifndef _LLYR_GRAPH_H
#define _LLYR_GRAPH_H

#include <sst/core/output.h>

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

namespace SST {
namespace Llyr {

class Edge
{
private:
    float weight;
    uint32_t destinationVertex;

protected:

public:
    Edge();
    Edge( uint32_t vertexIn )
    {
        weight = 0.00;
        destinationVertex = vertexIn;
    }
    Edge( float weightIn, uint32_t vertexIn )
    {
        weight = weightIn;
        destinationVertex = vertexIn;
    }

    bool setWeight( float weightIn )
    {
        weight = weightIn;
    }

    float getWeight( void ) const
    {
        return weight;
    }

    uint32_t getDestination( void ) const
    {
        return destinationVertex;
    }

};


template<class T>
class Vertex
{
private:
    T type;
    bool visited;

    std::vector< Edge* >* adjacencyList;

protected:

public:
    Vertex()
    {
        adjacencyList = new std::vector< Edge* >;
        visited = 0;
    }

    Vertex( T typeIn )
    {
        adjacencyList = new std::vector< Edge* >;

        type = typeIn;
        visited = 0;
    }

    bool setType( T typeIn )
    {
        type = typeIn;
        return true;
    }

    T getType( void ) const
    {
        return type;
    }

    std::vector< Edge* >* getAdjacencyList( void ) const
    {
        return adjacencyList;
    }

    void addEdge( Edge* edgeIn )
    {
        adjacencyList->push_back(edgeIn);
    }
};

template<class T>
class LlyrGraph
{

private:
    uint32_t vertices;
    std::map< uint32_t, Vertex<T> >* vertexMap;

    SST::Output* output;

protected:

public:
    LlyrGraph();
    ~LlyrGraph();

    void printGraph( void );
    void printDot( std::string fileName );

    uint32_t numVertices( void );
    void addEdge( uint32_t beginVertex, uint32_t endVertex );
    void addEdge( uint32_t beginVertex, uint32_t endVertex, float weightIn );
    void addVertex( uint32_t beginVertex, T type );

};

template<class T>
LlyrGraph<T>::LlyrGraph()
{
    vertexMap = new std::map< uint32_t, Vertex<T> >;
    vertices = 0;
}

template<class T>
LlyrGraph<T>::~LlyrGraph()
{}

template<class T>
void LlyrGraph<T>::printGraph()
{
    typename std::map< uint32_t, Vertex<T> >::iterator vertexIterator;
    for(vertexIterator = vertexMap->begin(); vertexIterator != vertexMap->end(); ++vertexIterator)
    {
        std::cout << "\n Adjacency list of vertex " << vertexIterator->first << "\n head ";

        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        std::vector< Edge* >::iterator it;
        for( it = adjacencyList->begin(); it != adjacencyList->end(); it++ )
        {
            std::cout << "-> " << (*it)->getDestination();
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
    for(vertexIterator = vertexMap->begin(); vertexIterator != vertexMap->end(); ++vertexIterator)
    {
        outputFile << vertexIterator->first << "[label=\"";
        outputFile << vertexIterator->second.getType();
        outputFile << "\"];\n";
    }

    for(vertexIterator = vertexMap->begin(); vertexIterator != vertexMap->end(); ++vertexIterator)
    {
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        std::vector< Edge* >::iterator it;
        for( it = adjacencyList->begin(); it != adjacencyList->end(); it++ )
        {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << (*it)->getDestination();
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
    Edge* edge = new Edge( endVertex );

    vertexMap->at(beginVertex).addEdge(edge);
}

template<class T>
void LlyrGraph<T>::addEdge( uint32_t beginVertex, uint32_t endVertex, float weightIn )
{
    std::cout << "add edge:  " << beginVertex << " --> " << endVertex << std::endl;
    Edge* edge = new Edge( weightIn, endVertex );

    vertexMap->at(beginVertex).addEdge(edge);
}

template<class T>
void LlyrGraph<T>::addVertex(uint32_t vertexIn, T type)
{
    std::cout << "add vertex:  " << vertexIn << std::endl;

    Vertex<T> vertex;
    vertex.setType(type);

    std::pair< typename std::map< uint32_t, Vertex<T> >::iterator,bool > retVal;
    retVal = vertexMap->insert( std::pair< uint32_t, Vertex<T> >( vertexIn, vertex) );
    if( retVal.second == false )
    {
        ///TODO
    }

    vertices = vertices + 1;
}


} // namespace LLyr
} // namespace SST

#endif



