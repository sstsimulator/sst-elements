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

#include "../graph/graph.h"
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

    virtual void mapGraph(LlyrGraph< opType > hardwareGraph, LlyrGraph< AppNode > appGraph,
                          LlyrGraph< ProcessingElement* > &graphOut,
                          LlyrConfig* llyr_config) = 0;

    void addNode(opType op_binding, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config);
    void addNode(opType op_binding, int64_t intConst, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                 LlyrConfig* llyr_config);
    void addNode(opType op_binding, double dblConst, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                 LlyrConfig* llyr_config);

};

void LlyrMapper::addNode(opType op_binding, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                  LlyrConfig* llyr_config)
{
    ProcessingElement* tempPE;

    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][llyrMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    if( op_binding == LD ) {
        tempPE = new LoadProcessingElement( LD, nodeNum, llyr_config );
    } else if( op_binding == LD_ST ) {
        tempPE = new LoadProcessingElement( LD_ST, nodeNum, llyr_config );
    } else if( op_binding == ST ) {
        tempPE = new StoreProcessingElement( ST, nodeNum, llyr_config );
    } else if( op_binding == AND ) {
        tempPE = new LogicProcessingElement( AND, nodeNum, llyr_config );
    } else if( op_binding == OR ) {
        tempPE = new LogicProcessingElement( OR, nodeNum, llyr_config );
    } else if( op_binding == XOR ) {
        tempPE = new LogicProcessingElement( XOR, nodeNum, llyr_config );
    } else if( op_binding == NOT ) {
        tempPE = new LogicProcessingElement( NOT, nodeNum, llyr_config );
    } else if( op_binding == SLL ) {
        tempPE = new LogicProcessingElement( SLL, nodeNum, llyr_config );
    } else if( op_binding == SLR ) {
        tempPE = new LogicProcessingElement( SLR, nodeNum, llyr_config );
    } else if( op_binding == ROL ) {
        tempPE = new LogicProcessingElement( ROL, nodeNum, llyr_config );
    } else if( op_binding == ROR ) {
        tempPE = new LogicProcessingElement( ROR, nodeNum, llyr_config );
    } else if( op_binding == EQ ) {
        tempPE = new LogicProcessingElement( EQ, nodeNum, llyr_config );
    } else if( op_binding == NE ) {
        tempPE = new LogicProcessingElement( NE, nodeNum, llyr_config );
    } else if( op_binding == UGT ) {
        tempPE = new LogicProcessingElement( UGT, nodeNum, llyr_config );
    } else if( op_binding == UGE ) {
        tempPE = new LogicProcessingElement( UGE, nodeNum, llyr_config );
    } else if( op_binding == SGT ) {
        tempPE = new LogicProcessingElement( SGT, nodeNum, llyr_config );
    } else if( op_binding == SGE ) {
        tempPE = new LogicProcessingElement( SGE, nodeNum, llyr_config );
    } else if( op_binding == ULT ) {
        tempPE = new LogicProcessingElement( ULT, nodeNum, llyr_config );
    } else if( op_binding == ULE ) {
        tempPE = new LogicProcessingElement( ULE, nodeNum, llyr_config );
    } else if( op_binding == SLT ) {
        tempPE = new LogicProcessingElement( SLT, nodeNum, llyr_config );
    } else if( op_binding == SLE ) {
        tempPE = new LogicProcessingElement( SLE, nodeNum, llyr_config );
    } else if( op_binding == ADD ) {
        tempPE = new IntProcessingElement( ADD, nodeNum, llyr_config );
    } else if( op_binding == SUB ) {
        tempPE = new IntProcessingElement( SUB, nodeNum, llyr_config );
    } else if( op_binding == MUL ) {
        tempPE = new IntProcessingElement( MUL, nodeNum, llyr_config );
    } else if( op_binding == DIV ) {
        tempPE = new IntProcessingElement( DIV, nodeNum, llyr_config );
    } else if( op_binding == REM ) {
        tempPE = new IntProcessingElement( REM, nodeNum, llyr_config );
    } else if( op_binding == FADD ) {
        tempPE = new FPProcessingElement( FADD, nodeNum, llyr_config );
    } else if( op_binding == FSUB ) {
        tempPE = new FPProcessingElement( FSUB, nodeNum, llyr_config );
    } else if( op_binding == FMUL ) {
        tempPE = new FPProcessingElement( FMUL, nodeNum, llyr_config );
    } else if( op_binding == FDIV ) {
        tempPE = new FPProcessingElement( FDIV, nodeNum, llyr_config );
    } else if( op_binding == FMatMul ) {
        tempPE = new FPProcessingElement( FMatMul, nodeNum, llyr_config );
    } else if( op_binding == TSIN ) {
        tempPE = new ComplexProcessingElement( TSIN, nodeNum, llyr_config );
    } else if( op_binding == TCOS ) {
        tempPE = new ComplexProcessingElement( TCOS, nodeNum, llyr_config );
    } else if( op_binding == TTAN ) {
        tempPE = new ComplexProcessingElement( TTAN, nodeNum, llyr_config );
    } else if( op_binding == DUMMY ) {
        tempPE = new DummyProcessingElement( DUMMY, nodeNum, llyr_config );
    } else if( op_binding == BUFFER ) {
        tempPE = new ControlProcessingElement( BUFFER, nodeNum, llyr_config );
    } else if( op_binding == SEL ) {
        tempPE = new ControlProcessingElement( SEL, nodeNum, llyr_config );
    } else if( op_binding == RTR ) {
        tempPE = new ControlProcessingElement( RTR, nodeNum, llyr_config );
    } else if( op_binding == RET ) {
        tempPE = new ControlProcessingElement( RET, nodeNum, llyr_config );
    } else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to find specified operation\n");
        exit(0);
    }

    graphOut.addVertex( nodeNum, tempPE );

}// addNode

void LlyrMapper::addNode(opType op_binding, int64_t intConst, uint32_t nodeNum, LlyrGraph< ProcessingElement* > &graphOut,
                         LlyrConfig* llyr_config)
{
    ProcessingElement* tempPE;

    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][llyrMapper]: ");
    SST::Output* output_ = new SST::Output(prefix, llyr_config->verbosity_, 0, Output::STDOUT);

    if( op_binding == LDADDR ) {
        std::queue< LlyrData > tempData;
        tempData.push(LlyrData(intConst));
        tempPE = new LoadProcessingElement( LD, nodeNum, llyr_config, tempData );
    } else if( op_binding == STADDR ) {
        std::queue< LlyrData > tempData;
        tempData.push(LlyrData(intConst));
        tempPE = new StoreProcessingElement( ST, nodeNum, llyr_config, tempData );
    } else if( op_binding == ADDCONST ) {
        tempPE = new IntConstProcessingElement( ADDCONST, nodeNum, llyr_config, intConst );
    } else if( op_binding == SUBCONST ) {
        tempPE = new IntConstProcessingElement( SUBCONST, nodeNum, llyr_config, intConst );
    } else if( op_binding == MULCONST ) {
        tempPE = new IntConstProcessingElement( MULCONST, nodeNum, llyr_config, intConst );
    } else if( op_binding == DIVCONST ) {
        tempPE = new IntConstProcessingElement( DIVCONST, nodeNum, llyr_config, intConst );
    } else if( op_binding == REMCONST ) {
        tempPE = new IntConstProcessingElement( REMCONST, nodeNum, llyr_config, intConst );
    } else {
        output_->fatal(CALL_INFO, -1, "Error: Unable to find specified operation\n");
        exit(0);
    }

    graphOut.addVertex( nodeNum, tempPE );

}// addNode


}// namespace Llyr
}// namespace SST

#endif // _LLYR_MAPPER_H
