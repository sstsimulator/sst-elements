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

#include "llyrMapper.h"

namespace SST {
namespace Llyr {

class SimpleMapper : public LlyrMapper
{

public:
    SimpleMapper(Params& params) :
        LlyrMapper()
    {
    }

    ~SimpleMapper() { }

    SST_ELI_REGISTER_MODULE_DERIVED(
        SimpleMapper,
        "llyr",
        "simpleMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Greedy subgraph mapper.",
        SST::Llyr::LlyrMapper
    )

    void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph, LlyrGraph< ProcessingElement* > &graphOut);

private:


};

void SimpleMapper::mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph, LlyrGraph< ProcessingElement* > &graphOut)
{
    //Dummy node to make BFS easier
    ProcessingElement* tempPE = new ProcessingElement(DUMMY, 0, 0);
    graphOut.addVertex( 0, tempPE );

    tempPE = new ProcessingElement( LD, 1, 2 );
    graphOut.addVertex( 1, tempPE );

    tempPE = new ProcessingElement( LD, 2, 2 );
    graphOut.addVertex( 2, tempPE );

    tempPE = new ProcessingElement( ADD, 3, 2 );
    graphOut.addVertex( 3, tempPE );

    tempPE = new ProcessingElement( ST, 4, 2 );
    graphOut.addVertex( 4, tempPE );

//     tempPE = new ProcessingElement( ST, 22, 2 );
//     graphOut.addVertex( 22, tempPE );
//
//     tempPE = new ProcessingElement( ST, 8, 2 );
//     graphOut.addVertex( 8, tempPE );
//
//     tempPE = new ProcessingElement( ST, 9, 2 );
//     graphOut.addVertex( 9, tempPE );
//
//     tempPE = new ProcessingElement( ST, 10, 2 );
//     graphOut.addVertex( 10, tempPE );

    graphOut.addEdge( 0, 1 );
    graphOut.addEdge( 0, 2 );
    graphOut.addEdge( 1, 3 );
    graphOut.addEdge( 2, 3 );
    graphOut.addEdge( 3, 4 );

//     graphOut.addEdge( 4, 8 );
//     graphOut.addEdge( 4, 9 );
//     graphOut.addEdge( 4, 10 );
//     graphOut.addEdge( 8, 22 );
//     graphOut.addEdge( 9, 22 );
//     graphOut.addEdge( 10, 22 );

}


//     graphOut.addVertex( 0, LD );
//     graphOut.addVertex( 1, LD );
//     graphOut.addVertex( 3, ADD );
//     graphOut.addVertex( 4, ST );
//
//     graphOut.addEdge( 0, 3 );
//     graphOut.addEdge( 1, 3 );
//     graphOut.addEdge( 3, 4 );

}
} // namespace SST

#endif // _SIMPLE_MAPPER_H

