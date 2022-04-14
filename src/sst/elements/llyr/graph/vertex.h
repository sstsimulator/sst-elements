// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _LLYR_G_VERTEX_H
#define _LLYR_G_VERTEX_H

#include <sst/core/sst_config.h>
#include <sst/core/output.h>

#include <vector>
#include <cstdint>

#include "edge.h"

namespace SST {
namespace Llyr {

struct VertexProperties
{

};

template<class T>
class Vertex
{
private:
    T    value_;
    bool visited_;
    uint32_t numInEdges_;
    uint32_t numOutEdges_;

    std::vector< Edge* >* adjacencyList_;

protected:

public:
    Vertex()
    {
        adjacencyList_ = new std::vector< Edge* >;
        visited_ = 0;
        numInEdges_ = 0;
        numOutEdges_ = 0;
    }

    Vertex( T typeIn ) : value_(typeIn)
    {
        adjacencyList_ = new std::vector< Edge* >;
        visited_ = 0;
        numInEdges_ = 0;
        numOutEdges_ = 0;
    }

    Vertex(const Vertex &valueIn)
    {
        value_ = valueIn.value_;
        visited_ = valueIn.visited_;
        numInEdges_ = valueIn.numInEdges_;
        numOutEdges_ = valueIn.numOutEdges_;
        adjacencyList_ = new std::vector< Edge* >(*(valueIn.adjacencyList_));
    }

    bool operator == (const Vertex &valueIn) const
    {
        return(this->value_ == valueIn.getValue());
    }

    void setValue( T typeIn )
    {
        value_ = typeIn;
    }

    T getValue( void ) const
    {
        return value_;
    }

    void setVisited( bool visitIn )
    {
        visited_ = visitIn;
    }

    bool getVisited( void ) const
    {
        return visited_;
    }

    std::vector< Edge* >* getAdjacencyList( void ) const
    {
        return adjacencyList_;
    }

    bool addEdge( Edge* edgeIn )
    {
        bool found = 0;
        std::vector< Edge* >* adjacencyList = getAdjacencyList();
        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
            if( edgeIn->getDestination() == (*it)->getDestination() ) {
                found = 1;
                break;
            }
        }

        if(found == 1) {
            return 0;
        } else {
            adjacencyList_->push_back(edgeIn);
            return 1;
        }
    }

    void addInDegree()
    {
        ++numInEdges_;
    }

    uint32_t getInDegree() const
    {
        return numInEdges_;
    }

    void addOutDegree()
    {
        ++numOutEdges_;
    }

    uint32_t getOutDegree() const
    {
        return numOutEdges_;
    }

}; //END Vertex


} // namespace LLyr
} // namespace SST

#endif



