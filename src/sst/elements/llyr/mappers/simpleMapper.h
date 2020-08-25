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

#include <sst/core/module.h>

#include "../graph.h"
#include "../processingElement.h"
#include "llyrMapper.h"

#include <iostream>

namespace SST {
namespace Llyr {

// template<class T>
class SimpleMapper : public LlyrMapper
{

public:
    SimpleMapper(Params& params) : LlyrMapper()
    {
    }

    SST_ELI_REGISTER_MODULE_DERIVED(
        SimpleMapper,
        "llyr",
        "simpleMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Greedy subgraph mapper.",
        SST::Llyr::LlyrMapper
    )

//     void mapGraph(LlyrGraph<T> hardwareGraph, LlyrGraph<T> appGraph, LlyrGraph<T> graphOut);
    void mapGraph(LlyrGraph<opType> hardwareGraph, LlyrGraph<opType> appGraph, LlyrGraph<opType> &graphOut);

private:


};

// template<class T>
// void SimpleMapper<T>::mapGraph(LlyrGraph<T> hardwareGraph, LlyrGraph<T> appGraph, LlyrGraph<T> graphOut)
// {
//     std::cout << "FDSLAFKDLSKFLDKSFKDSLKFLDSKAFDSALFKDSLAKFDSKAFDSAFDKSLAFKDLSKFDLSA\n\n\n\n";
//
// }

void SimpleMapper::mapGraph(LlyrGraph<opType> hardwareGraph, LlyrGraph<opType> appGraph, LlyrGraph<opType> &graphOut)
{
    graphOut.addVertex( 0, LD );
    graphOut.addVertex( 1, LD );
    graphOut.addVertex( 3, ADD );
    graphOut.addVertex( 4, ST );

    graphOut.addEdge( 0, 3 );
    graphOut.addEdge( 1, 3 );
    graphOut.addEdge( 3, 4 );


}


}
} // namespace SST

#endif // _SIMPLE_MAPPER_H
