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

#ifndef _LLYR_MAPPER_H
#define _LLYR_MAPPER_H

#include <sst/core/sst_config.h>
#include <sst/core/module.h>
#include <sst/core/interfaces/simpleMem.h>

#include "../graph.h"
#include "../lsQueue.h"
#include "../llyrTypes.h"
#include "pes/peList.h"

namespace SST {
namespace Llyr {

class LlyrMapper : public SST::Module
{

public:
    SST_ELI_REGISTER_MODULE_API(SST::Llyr::LlyrMapper);

    LlyrMapper() : Module() {}
    virtual ~LlyrMapper() {}

    virtual void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< opType > appGraph,
                          LlyrGraph< ProcessingElement* > &graphOut,
                          LlyrConfig* llyr_config) = 0;

    void addNode(opType op_binding, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);

};

void LlyrMapper::addNode(opType op_binding, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config)
{
    ProcessingElement* tempPE;

    if( op_binding == LD ) {
        tempPE = new LoadProcessingElement( LD, nodeNum, llyr_config );
    } else if( op_binding == LD_ST ) {
        tempPE = new LoadProcessingElement( LD_ST, nodeNum, llyr_config );
    } else if( op_binding == ST ) {
        tempPE = new StoreProcessingElement( ST, nodeNum, llyr_config );
    } else if( op_binding == SLL ) {
        tempPE = new LogicProcessingElement( SLL, nodeNum, llyr_config );
    } else if( op_binding == SLR ) {
        tempPE = new LogicProcessingElement( SLR, nodeNum, llyr_config );
    } else if( op_binding == ROL ) {
        tempPE = new LogicProcessingElement( ROL, nodeNum, llyr_config );
    } else if( op_binding == ROR ) {
        tempPE = new LogicProcessingElement( ROR, nodeNum, llyr_config );
    } else if( op_binding == ADD ) {
        tempPE = new IntProcessingElement( ADD, nodeNum, llyr_config );
    } else if( op_binding == SUB ) {
        tempPE = new IntProcessingElement( SUB, nodeNum, llyr_config );
    } else if( op_binding == MUL ) {
        tempPE = new IntProcessingElement( MUL, nodeNum, llyr_config );
    } else if( op_binding == DIV ) {
        tempPE = new IntProcessingElement( DIV, nodeNum, llyr_config );
    } else if( op_binding == FPADD ) {
        tempPE = new FPProcessingElement( FPADD, nodeNum, llyr_config );
    } else if( op_binding == FPSUB ) {
        tempPE = new FPProcessingElement( FPSUB, nodeNum, llyr_config );
    } else if( op_binding == FPMUL ) {
        tempPE = new FPProcessingElement( FPMUL, nodeNum, llyr_config );
    } else if( op_binding == FPDIV ) {
        tempPE = new FPProcessingElement( FPDIV, nodeNum, llyr_config );
    } else if( op_binding == DUMMY ) {
        tempPE = new DummyProcessingElement( DUMMY, nodeNum, llyr_config );
//     } else if( op_binding == BUFFER ) {
//         operation = FPADD;
//     } else {
//         operation = OTHER;
//     }
    } else {
        exit(0);
    }

    graphOut.addVertex( nodeNum, tempPE );

}// addNode

}// namespace Llyr
}// namespace SST

#endif // _LLYR_MAPPER_H
