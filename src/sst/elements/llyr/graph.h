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

namespace SST {
namespace Llyr {

struct Vertex
{
    std::string type;
    std::vector< uint32_t >* adjacencyList;
};

class LlyrGraph
{

private:
    uint32_t vertices;
    std::map< uint32_t, Vertex > vertexMap;

    SST::Output* output;

protected:

public:

    LlyrGraph();
    ~LlyrGraph();

    void printGraph( void );
    void printDot( std::string fileName );

    uint32_t numVertices( void );
    void addEdge( uint32_t vertex, uint32_t edge );
    void addVertex( uint32_t beginVertex, std::string type );

};

} // namespace LLyr
} // namespace SST

#endif



