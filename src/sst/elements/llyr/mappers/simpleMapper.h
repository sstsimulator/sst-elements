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
#include "llyrMapper.h"

#include <iostream>

namespace SST {
namespace Llyr {

// template<class T>
class SimpleMapper : public LlyrMapper
{

public:
    SimpleMapper() : LlyrMapper();

    SST_ELI_REGISTER_MODULE_DERIVED(
        SimpleMapper,
        "llyr",
        "simpleMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Greedy subgraph mapper.",
        "SST::Llyr::LlyrMapper"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"hardware graph", "Name to give this module", ""},
        {"application graph", "Name to give this module", ""},
    )

//     void mapGraph(LlyrGraph<T> hardwareGraph, LlyrGraph<T> appGraph, LlyrGraph<T> graphOut);
    void mapGraph(LlyrGraph<std::string> hardwareGraph, LlyrGraph<std::string> appGraph, LlyrGraph<opType> graphOut);

private:


};

// template<class T>
// void SimpleMapper<T>::mapGraph(LlyrGraph<T> hardwareGraph, LlyrGraph<T> appGraph, LlyrGraph<T> graphOut)
// {
//     std::cout << "FDSLAFKDLSKFLDKSFKDSLKFLDSKAFDSALFKDSLAKFDSKAFDSAFDKSLAFKDLSKFDLSA\n\n\n\n";
//
// }

void SimpleMapper::mapGraph(LlyrGraph<std::string> hardwareGraph, LlyrGraph<std::string> appGraph, LlyrGraph<opType> graphOut)
{
    std::cout << "FDSLAFKDLSKFLDKSFKDSLKFLDSKAFDSALFKDSLAKFDSKAFDSAFDKSLAFKDLSKFDLSA\n\n\n\n";
}


}
} // namespace SST

#endif // _SIMPLE_MAPPER_H
