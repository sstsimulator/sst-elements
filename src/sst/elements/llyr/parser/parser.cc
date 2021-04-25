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

#include <sst_config.h>

#include <regex>
#include <iostream>

#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Demangle/Demangle.h"

#include "parser.h"

namespace SST {
namespace Llyr {

void Parser::generateAppGraph(std::string functionName)
{
    llvm::SMDiagnostic Err;
    llvm::LLVMContext Context;
    std::unique_ptr< llvm::MemoryBuffer > irBuff = llvm::MemoryBuffer::getMemBuffer(offloadString_);

    std::unique_ptr< llvm::Module > mod(llvm::parseIR(irBuff->getMemBufferRef(), Err, Context));

    for( auto functionIter = mod->getFunctionList().begin(), functionEnd = mod->getFunctionList().end(); functionIter != functionEnd; ++functionIter )
    {
        llvm::errs() << "Function Name: ";
        llvm::errs().write_escaped(functionIter->getName()) << "     ";
        llvm::errs().write_escaped(llvm::demangle(functionIter->getName().str() )) << '\n';

        //check each located function to see if it's the offload target
        if( functionIter->getName().find(functionName) != std::string::npos ) {
            generatebBasicBlockGraph(&*functionIter);
            doTheseThings(&*functionIter);

            break;
        }
    }// function loop

    std::cout << "Finished parsing..." << std::endl;

}// generateAppGraph

void Parser::generatebBasicBlockGraph(llvm::Function* func)
{
    std::cout << "Generating BB Graph..." << std::endl;
    llvm::errs().write_escaped(llvm::demangle(func->getName().str() )) << '\n';

    for( auto blockIter = func->getBasicBlockList().begin(), blockEnd = func->getBasicBlockList().end(); blockIter != blockEnd; ++blockIter )
    {
        llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(blockIter->getTerminator());

        llvm::errs() << "\t+++Basic Block Name(" << &*blockIter << "): ";
        llvm::errs().write_escaped(blockIter->getName()) << "  --->  " << &*Inst << '\n';

        bbGraph_->addVertex(&*blockIter);
    }

    std::map< uint32_t, Vertex< llvm::BasicBlock* > >* vertex_map_ = bbGraph_->getVertexMap();
    typename std::map< uint32_t, Vertex< llvm::BasicBlock* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        llvm::errs() << "\nBasic Block " << vertexIterator->second.getValue() << "\n";

        uint32_t totalSuccessors = vertexIterator->second.getValue()->getTerminator()->getNumSuccessors();
        for( uint32_t successor = 0; successor < totalSuccessors; successor++ ) {
            llvm::errs() << "\tSuccessors " << successor << " of " << totalSuccessors << "\n";

            llvm::BasicBlock* tempBB = vertexIterator->second.getValue()->getTerminator()->getSuccessor(successor);
            llvm::errs() << "\nBasic Block " << tempBB << "\n";

            typename std::map< uint32_t, Vertex< llvm::BasicBlock* > >::iterator vertexIteratorInner;
            for(vertexIteratorInner = vertex_map_->begin(); vertexIteratorInner != vertex_map_->end(); ++vertexIteratorInner) {
                if( vertexIteratorInner->second.getValue() == tempBB ) {
                    llvm::errs() << "\t\tFound:  " << vertexIteratorInner->second.getValue() << "\n";

                    bbGraph_->addEdge(vertexIterator->first, vertexIteratorInner->first);

                    break;
                }
            }
        }// successor loop
    }// basic block loop

    // bb_Graph should be complete here
    bbGraph_->printDot("00_test.dot");
}// generatebBasicBlockGraph


void Parser::doTheseThings(llvm::Function* func)
{
    std::cout << "Generating Other Graph..." << std::endl;
    instructionMap_ = new std::map< llvm::Instruction*, CDFGVertex* >;

    for( auto blockIter = func->getBasicBlockList().begin(), blockEnd = func->getBasicBlockList().end(); blockIter != blockEnd; ++blockIter ) {
        (*flowGraph_)[&*blockIter] = new CDFG;
        CDFG &g = *((*flowGraph_)[&*blockIter]);

        (*useNode_)[&*blockIter] = new std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >;
        (*defNode_)[&*blockIter] = new std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >;


    }


}// doTheseThings

} // namespace llyr
} // namespace SST


