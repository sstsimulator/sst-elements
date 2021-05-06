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

#include <sst/core/sst_config.h>
#include <sst/core/output.h>

#include "edge.h"
#include "vertex.h"

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

namespace SST {
namespace Llyr {


template<class T>
class LlyrGraph
{

private:
    uint32_t vertices_;
    std::map< uint32_t, Vertex<T> >* vertex_map_;

protected:

public:
    LlyrGraph();
    ~LlyrGraph();

    uint32_t operator []( const Vertex<T>& value ) const;

    static void copyGraph( const LlyrGraph<T> &graphIn, LlyrGraph<T> &graphOut );

    void printGraph();
    void printDot( std::string fileName );

    uint32_t outEdges( uint32_t vertexId );
    uint32_t numVertices() const;

    void addEdge( uint32_t beginVertex, uint32_t endVertex );
    void addEdge( uint32_t beginVertex, uint32_t endVertex, EdgeProperties* properties );

    uint32_t addVertex( T type );
    uint32_t addVertex( uint32_t vertexNum, T type );

    Vertex<T>* getVertex( uint32_t vertexNum ) const;
    void setVertex( uint32_t vertexNum, const Vertex<T> &vertex );

    std::map< uint32_t, Vertex<T> >* getVertexMap( void ) const;

};

template<class T>
LlyrGraph<T>::LlyrGraph()
{
    vertex_map_ = new std::map< uint32_t, Vertex<T> >;
    vertices_ = 0;
}

template<class T>
LlyrGraph<T>::~LlyrGraph()
{}

template<class T>
uint32_t LlyrGraph<T>::operator []( const Vertex<T>& value ) const
{
    for( auto it = vertex_map_->begin(); it != vertex_map_ ->end(); ++it ) {
        if( it->second == value ) {
            return it->first;
        }
    }

    return 0;
}

template<class T>
void LlyrGraph<T>::copyGraph( const LlyrGraph<T> &graphIn, LlyrGraph<T> &graphOut )
{
    //container of input -> output vertex mappings
    std::map< uint32_t, uint32_t > vertexMappings;

    //add all of the vertices in the input graph to the output graph
    auto vertexMap = graphIn.getVertexMap();
    for( auto vertexIterator = vertexMap->begin(); vertexIterator != vertexMap ->end(); ++vertexIterator ) {
        auto vertexIn = vertexIterator->second;
        uint32_t newVertex = graphOut.addVertex(vertexIn.getValue());

        auto retVal = vertexMappings.emplace( vertexIterator->first, newVertex );
        if( retVal.second == false ) {
            ///TODO
        }

        std::cout << "Old: " << vertexIterator->first << "  New: " << newVertex << std::endl;

    }

    //go back and add all of the outEdges
    for( auto vertexIterator = vertexMap->begin(); vertexIterator != vertexMap ->end(); ++vertexIterator ) {
        uint32_t sourceVertex = vertexMappings[vertexIterator->first];
//         std::cout << "\n Adjacency list of vertex " << vertexIterator->first << "\n head ";
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
//             std::cout << "-> " << (*it)->getDestination();

            EdgeProperties* tempProperties = (*it)->getProperties();
            uint32_t destinationVertex = vertexMappings[(*it)->getDestination()];
//             std::cout << "SRC: " << vertexIterator->first << "  DST: " << (*it)->getDestination() << std::endl;
//             std::cout << "SRC: " << sourceVertex << "  DST: " << destinationVertex << std::endl;
            graphOut.addEdge(sourceVertex, destinationVertex, tempProperties);

        }
        std::cout << std::endl;
    }

}

template<class T>
void LlyrGraph<T>::printGraph(void)
{
    typename std::map< uint32_t, Vertex<T> >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        std::cout << "\n Adjacency list of vertex " << vertexIterator->first << "\n head ";

        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
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
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        outputFile << vertexIterator->first << "[label=\"";
        outputFile << vertexIterator->second.getValue();
        outputFile << "\"];\n";
    }

    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
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

template<class T>
uint32_t LlyrGraph<T>::outEdges(uint32_t vertexId)
{
    return vertex_map_->at(vertexId).adjacencyList_->size();
}

template<class T>
uint32_t LlyrGraph<T>::numVertices(void) const
{
    return vertices_;
}

template<class T>
void LlyrGraph<T>::addEdge( uint32_t beginVertex, uint32_t endVertex )
{
    std::cout << "add edge:  " << beginVertex << " --> " << endVertex << "\n" << std::endl;
    Edge* edge = new Edge( endVertex );

    vertex_map_->at(beginVertex).addEdge(edge);
    vertex_map_->at(beginVertex).addOutDegree();
    vertex_map_->at(endVertex).addInDegree();
}

template<class T>
void LlyrGraph<T>::addEdge( uint32_t beginVertex, uint32_t endVertex, EdgeProperties* properties )
{
    std::cout << "add edge:  " << beginVertex << " --> " << endVertex << "\n" << std::endl;

    Edge* edge = new Edge( properties, endVertex );

    vertex_map_->at(beginVertex).addEdge(edge);
    vertex_map_->at(beginVertex).addOutDegree();
    vertex_map_->at(endVertex).addInDegree();
}

template<class T>
uint32_t LlyrGraph<T>::addVertex(T type)
{
    Vertex<T> vertex;
    vertex.setValue(type);

    uint32_t vertexNum = vertices_;
    std::cout << "add vertex:  " << vertexNum << "\n" << std::endl;
    auto retVal = vertex_map_->emplace( vertexNum, vertex );
    if( retVal.second == false ) {
        ///TODO
    }

    vertices_ = vertices_ + 1;
    return vertexNum;
}

template<class T>
uint32_t LlyrGraph<T>::addVertex(uint32_t vertexNum, T type)
{
    Vertex<T> vertex;
    vertex.setValue(type);

    std::cout << "add vertex:  " << vertexNum << "\n" << std::endl;
    auto retVal = vertex_map_->emplace( vertexNum, vertex );
    if( retVal.second == false ) {
        ///TODO
    }

    vertices_ = vertices_ + 1;
    return vertexNum;
}

template<class T>
Vertex<T>* LlyrGraph<T>::getVertex( uint32_t vertexNum ) const
{
    return &vertex_map_->at(vertexNum);
}

template<class T>
void LlyrGraph<T>::setVertex( uint32_t vertexNum, const Vertex<T> &vertex )
{
    vertex_map_->at(vertexNum) = vertex;
}

template<class T>
std::map< uint32_t, Vertex<T> >* LlyrGraph<T>::getVertexMap( void ) const
{
    return vertex_map_;
}

} // namespace LLyr
} // namespace SST

#endif



