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

#ifndef _PARSER_H
#define _PARSER_H

#include <sst/core/sst_config.h>

#include <vector>
#include <string>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include "graph.h"
#include "llyrTypes.h"
#include "pes/peList.h"

namespace SST {
namespace Llyr {

struct alignas(double) CDFGVertex
{
   llvm::Instruction*  instruction_;
   std::string         valueName_;
   int64_t             intConst_;
   float               floatConst_;
   double              doubleConst_;
   std::string         instructionName_;
};

typedef LlyrGraph< CDFGVertex* > CDFG;

class Parser
{
public:
    Parser(std::string offloadString, SST::Output* output) :
                    output_(output), offloadString_(offloadString)
    {
        vertexList_ = new std::map< llvm::BasicBlock*, std::vector< CDFGVertex* > >;

        defNode_ = new std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >;
        useNode_ = new std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >;

        flowGraph_ = new std::map< llvm::BasicBlock*, CDFG* >;
        bbGraph_   = new LlyrGraph< llvm::BasicBlock* >;
    }

    ~Parser() {};

    void generateAppGraph(std::string functionName);
    LlyrGraph< opType > getApplicationGraph() { return applicationGraph_; };

protected:

private:
    SST::Output* output_;
    std::string offloadString_;
    std::string offloadTarget_;

    LlyrGraph< opType > applicationGraph_;
    LlyrGraph< llvm::BasicBlock* >* bbGraph_;
    std::map< llvm::BasicBlock*, CDFG* >* flowGraph_;

    std::map< llvm::BasicBlock*, std::vector< CDFGVertex* > >* vertexList_;
    std::map< llvm::Instruction*, CDFGVertex* >* instructionMap_;

    std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >* defNode_;
    std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >* useNode_;

    void generatebBasicBlockGraph(llvm::Function* func);
    void doTheseThings(llvm::Function* func);
};

} // namespace LLyr
} // namespace SST

#endif /* _PARSER_H */
