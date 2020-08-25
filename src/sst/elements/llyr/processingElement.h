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


#ifndef _LLYR_PE_H
#define _LLYR_PE_H

#include <vector>
#include <cstdint>

namespace SST {
namespace Llyr {

typedef enum {
    ADD,
    SUB,
    MUL,
    DIV,
    FPADD,
    FPSUB,
    FPMUL,
    FPDIV,
    OTHER
} opType;


class ProcessingElement {
public:
    ProcessingElement();
    virtual ~ProcessingElement();

protected:


private:
    uint32_t processorId;

    std::vector< std::vector< opType > >* inputQueues;
    std::vector< std::vector< opType > >* outputQueues;

};


}
}

#endif
