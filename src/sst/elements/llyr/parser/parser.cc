// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <regex>
#include <iostream>
#include <algorithm>

#include <llvm/Pass.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>

#include <llvm/Demangle/Demangle.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Scalar.h>

#include "parser.h"

namespace SST {
namespace Llyr {

void Parser::generateAppGraph(std::string functionName)
{
    bool foundOffload;
    llvm::SMDiagnostic Err;
    llvm::LLVMContext Context;

    std::unique_ptr< llvm::MemoryBuffer > irBuff = llvm::MemoryBuffer::getMemBuffer(offloadString_);
    std::unique_ptr< llvm::Module > mod(llvm::parseIR(irBuff->getMemBufferRef(), Err, Context));
    mod_ = mod.get();

    //get names for anonymous instructions
    auto pm = std::make_unique<llvm::legacy::FunctionPassManager>(mod_);
    pm->add(llvm::createPromoteMemoryToRegisterPass());
    pm->add(llvm::createInstructionNamerPass());
    pm->add(llvm::createIndVarSimplifyPass());
    pm->add(llvm::createLoopUnrollAndJamPass());
    pm->doInitialization();

    foundOffload = 0;
    for( auto functionIter = mod_->getFunctionList().begin(), functionEnd = mod_->getFunctionList().end(); functionIter != functionEnd; ++functionIter ) {
        if( output_->getVerboseLevel() > 64 ) {
            llvm::errs() << "Function Name: ";
            llvm::errs().write_escaped(functionIter->getName()) << "     ";
            llvm::errs().write_escaped(llvm::demangle(functionIter->getName().str() )) << '\n';
        }

        //check each located function to see if it's the offload target
        if( functionIter->getName().find(functionName) != std::string::npos ) {
            pm->run(*functionIter);

            generatebBasicBlockGraph(&*functionIter);
            expandBBGraph(&*functionIter);
            assembleGraph();
            mergeGraphs();
//             collapseInductionVars();

            foundOffload = 1;
            break;
        }
    }// function loop

    if( foundOffload == 0 ) {
        output_->fatal(CALL_INFO, -1, "Error: No offload target\n");
        exit(0);
    }

    output_->verbose(CALL_INFO, 1, 0, "Finished parsing...\n");

    printCDFG( "00_func-ins.dot" );
    printPyMapper( "00_amapper.dot" );

}// generateAppGraph

void Parser::generatebBasicBlockGraph(llvm::Function* func)
{
    output_->verbose(CALL_INFO, 1, 0, "Generating BB Graph...\n");

    if( output_->getVerboseLevel() > 64 ) {
        llvm::errs().write_escaped(llvm::demangle(func->getName().str() )) << '\n';
    }

    for( auto blockIter = func->getBasicBlockList().begin(), blockEnd = func->getBasicBlockList().end(); blockIter != blockEnd; ++blockIter ) {
        llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(blockIter->getTerminator());

        if( output_->getVerboseLevel() > 64 ) {
            llvm::errs() << "\t+++Basic Block Name(" << &*blockIter << "): ";
            llvm::errs().write_escaped(blockIter->getName()) << "  --->  " << &*Inst << '\n';
        }

        bbGraph_->addVertex(&*blockIter);
    }

    std::map< uint32_t, Vertex< llvm::BasicBlock* > >* vertex_map_ = bbGraph_->getVertexMap();
    typename std::map< uint32_t, Vertex< llvm::BasicBlock* > >::iterator vertexIterator;
    for(vertexIterator = vertex_map_->begin(); vertexIterator != vertex_map_->end(); ++vertexIterator) {
        if( output_->getVerboseLevel() > 64 ) {
            llvm::errs() << "\nBasic Block " << vertexIterator->second.getValue() << "\n";
        }

        uint32_t totalSuccessors = vertexIterator->second.getValue()->getTerminator()->getNumSuccessors();
        for( uint32_t successor = 0; successor < totalSuccessors; successor++ ) {
            llvm::BasicBlock* tempBB = vertexIterator->second.getValue()->getTerminator()->getSuccessor(successor);

            if( output_->getVerboseLevel() > 64 ) {
                llvm::errs() << "\tSuccessors " << successor << " of " << totalSuccessors << "\n";
                llvm::errs() << "\nBasic Block " << tempBB << "\n";
            }

            typename std::map< uint32_t, Vertex< llvm::BasicBlock* > >::iterator vertexIteratorInner;
            for(vertexIteratorInner = vertex_map_->begin(); vertexIteratorInner != vertex_map_->end(); ++vertexIteratorInner) {
                if( vertexIteratorInner->second.getValue() == tempBB ) {

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "\t\tFound:  " << vertexIteratorInner->second.getValue() << "\n";
                    }

                    bbGraph_->addEdge(vertexIterator->first, vertexIteratorInner->first);

                    break;
                }
            }
        }// successor loop
    }// basic block loop

    // bb_Graph should be complete here
    output_->verbose(CALL_INFO, 1, 0, "...Basic Block Graph Done.\n");
    bbGraph_->printDot("00_bb.dot");
}// generatebBasicBlockGraph


void Parser::expandBBGraph(llvm::Function* func)
{
    output_->verbose(CALL_INFO, 1, 0, "\n\nGenerating Flow Graph...\n");

    CDFGVertex* entryVertex;
    CDFGVertex* outputVertex;
    CDFGVertex* inputVertex;
    int32_t inputVertexID = -1;
    std::map< llvm::Instruction*, CDFGVertex* >* instructionMap_ = new std::map< llvm::Instruction*, CDFGVertex* >;

    uint32_t tempOpcode;
    for( auto blockIter = func->getBasicBlockList().begin(), blockEnd = func->getBasicBlockList().end(); blockIter != blockEnd; ++blockIter ) {
        (*flowGraph_)[&*blockIter] = new CDFG;
        CDFG &g = *((*flowGraph_)[&*blockIter]);

        (*useNode_)[&*blockIter] = new std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >;
        (*defNode_)[&*blockIter] = new std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >;

        if( output_->getVerboseLevel() > 64 ) {
            llvm::errs() << "\t+++Basic Block Name(" << &*blockIter << "): ";
            llvm::errs().write_escaped(blockIter->getName()) << '\n';
        }

        for( auto instructionIter = blockIter->begin(), instructionEnd = blockIter->end(); instructionIter != instructionEnd; ++instructionIter ) {
            tempOpcode = instructionIter->getOpcode();

            if( output_->getVerboseLevel() > 64 ) {
                llvm::errs() << "\t\t**(" << &*instructionIter << ")  " << *instructionIter   << "  --  ";
                llvm::errs() << "Opcode Name:  ";
                llvm::errs().write_escaped(instructionIter->getName()) << "  ";
                llvm::errs().write_escaped(std::to_string(instructionIter->getOpcode())) << "\n";
            }

            outputVertex = new CDFGVertex;
            std::string tutu;
            llvm::raw_string_ostream rso(tutu);
            instructionIter->print(rso);
            outputVertex->instructionName_ = rso.str();
            outputVertex->instruction_ = &*instructionIter;
            outputVertex->haveConst_ = 0;
            outputVertex->intConst_ = 0x00;
            outputVertex->floatConst_ = 0x00;
            outputVertex->doubleConst_ = 0x00;

            uint32_t outputVertexID = g.addVertex(outputVertex);
            (*vertexList_)[&*blockIter].push_back(outputVertex);

            if( g.numVertices() == 1 ) {
                entryVertex = outputVertex;
            }

            instructionMap_->insert( std::pair< llvm::Instruction*, CDFGVertex* >(&*instructionIter, outputVertex) );

            if( output_->getVerboseLevel() > 64 ) {
                llvm::errs() << "-------------------------------------------- Users List --------------------------------------------\n";

                for( llvm::User *U : instructionIter->users() ) {
                    if( llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(U) ) {
                        llvm::errs() << *instructionIter << " is used in instruction:\t";
                        llvm::errs() << "(" << &*Inst << ") " << *Inst << "\n";
                    }

                }

                llvm::errs() << "----------------------------------------------------------------------------------------------------\n";
            }

            //determine operation
            if( tempOpcode == llvm::Instruction::GetElementPtr ) {
                std::cout << "R#REWREFDSFDASFA" <<std::endl;
            }
            if( tempOpcode == llvm::Instruction::Alloca ) {                                     // BEGIN Allocate

                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                // create the node/use entries (this should be empty)
                (*useNode_)[&*blockIter]->insert( std::pair< CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair< CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                    CDFGVertex* tempVal = g.getVertex(outputVertexID)->getValue();

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "Node-Use Entry (" << tempVal->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }
                }

                for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                    CDFGVertex* tempVal = g.getVertex(outputVertexID)->getValue();

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "Node-Def Entry (" << tempVal->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            // END Allocate
            } else if( tempOpcode == llvm::Instruction::Ret ) {                                     // BEGIN Return
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;

                llvm::Value* tempOperand = llvm::cast<llvm::ReturnInst>(instructionIter)->getReturnValue();

                // Test for ret val
                // If a function returns void, the value returned is a null pointer
                if( tempOperand == 0x00) {
                    // create the node/use entries
                    (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );

                    //create the node/def entries
                    (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
                } else if( llvm::isa<llvm::Constant>(tempOperand) || llvm::isa<llvm::Argument>(tempOperand) || llvm::isa<llvm::GlobalValue>(tempOperand) ) {
                    // create the node/use entries
                    (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );

                    //create the node/def entries
                    (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
                } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                    std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));
                    if( it != instructionMap_->end() ) {
                        inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                        }
                    } else {
                        inputVertex = new CDFGVertex;
                        inputVertex->instruction_ = 0x00;
                        inputVertex->haveConst_ = 0;
                        inputVertex->intConst_ = 0x00;
                        inputVertex->floatConst_ = 0x00;
                        inputVertex->doubleConst_ = 0x00;

                        inputVertexID = g.addVertex(inputVertex);
                        (*vertexList_)[&*blockIter].push_back(inputVertex);

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                    //add variable to node use list
                    tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));

                    // create the node/use entries
                    (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                    //create the node/def entries
                    (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
                }

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            // END Return
            } else if( tempOpcode == llvm::Instruction::Call ) {                                      // BEGIN Call

                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::Value* tempOperand = operandIter->get();

                    if( llvm::isa<llvm::Constant>(tempOperand) ) {
                    // Don't care about these args at the moment

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                    } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                        std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "**** " << static_cast<llvm::Instruction*>(operandIter->get()) << "   --   " << llvm::cast<llvm::Instruction>(tempOperand) << "   --   ";
                        }


                        if( it != instructionMap_->end() ) {
                            inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                            }

                        } else {
                            inputVertex = new CDFGVertex;
                            inputVertex->instruction_ = 0x00;
                            inputVertex->haveConst_ = 0;
                            inputVertex->intConst_ = 0x00;
                            inputVertex->floatConst_ = 0x00;
                            inputVertex->doubleConst_ = 0x00;

                            inputVertexID = g.addVertex(inputVertex);
                            (*vertexList_)[&*blockIter].push_back(inputVertex);

                            // create the node/use entries
                            (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                            //create the node/def entries
                            (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                        //add variable to node use list
                        tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));
                    }
                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            //END Call
            } else if( tempOpcode == llvm::Instruction::Br ) {                                        // BEGIN Branch
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                if(llvm::cast<llvm::BranchInst>(instructionIter)->isConditional() ) {
                    llvm::Value* tempCond =llvm::cast<llvm::BranchInst>(instructionIter)->getCondition();

                    std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempCond));
                    if( it != instructionMap_->end() ) {
                        inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempCond));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "+src Found " << inputVertex->instruction_ << " in instructionMap_\n";
                        }
                    } else {
                        inputVertex = new CDFGVertex;
                        inputVertex->instruction_ = 0x00;
                        inputVertex->haveConst_ = 0;
                        inputVertex->intConst_ = 0x00;
                        inputVertex->floatConst_ = 0x00;
                        inputVertex->doubleConst_ = 0x00;

                        inputVertexID = g.addVertex(inputVertex);
                        (*vertexList_)[&*blockIter].push_back(inputVertex);

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                    //add variable to node use list
                    tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempCond));
                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            // END Branch
            } else if( tempOpcode == llvm::Instruction::Load ) {                                       // BEGIN Load
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                llvm::Value* tempSrc = llvm::cast<llvm::LoadInst>(instructionIter)->getPointerOperand();
                uint32_t alignment = llvm::cast<llvm::LoadInst>(instructionIter)->getAlignment();

                //Get src information
                if( llvm::isa<llvm::Instruction>(tempSrc) ) {
                    std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempSrc));
                    if( it != instructionMap_->end() ) {
                        inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempSrc));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "+src Found " << inputVertex->instruction_ << " in instructionMap_\n";
                        }
                    } else {

                        outputVertex->haveConst_ = 1;
                        outputVertex->intConst_ = alignment;
                        outputVertex->floatConst_ = 0x00;
                        outputVertex->doubleConst_ = 0x00;

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = alignment;
//                         inputVertex->floatConst_ = 0x00;
//                         inputVertex->doubleConst_ = 0x00;

//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                    //add variable to node use list
                    tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempSrc));
                } else {
                    //TODO fix arguments as source

                    outputVertex->haveConst_ = 1;
                    outputVertex->intConst_ = alignment;
                    outputVertex->floatConst_ = 0x00;
                    outputVertex->doubleConst_ = 0x00;
                    outputVertex->valueName_ = tempSrc->getName().str();

//                     inputVertex = new CDFGVertex;
//                     inputVertex->instruction_ = 0x00;
//                     inputVertex->haveConst_ = 1;
//                     inputVertex->intConst_ = alignment;
//                     inputVertex->floatConst_ = 0x00;
//                     inputVertex->doubleConst_ = 0x00;
//                     inputVertex->valueName_ = tempSrc->getName().str();

//                     inputVertexID = g.addVertex(inputVertex);
//                     g.addEdge(inputVertexID, outputVertexID);
// //                     if(inserted)
// //                     {
// //                         g[edgeDesc].value_t = tempSrc;
// //                     }
//
//                     (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                     // create the node/use entries
//                     (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                     //create the node/def entries
//                     (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                }//end src

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            //END Load
            } else if( tempOpcode == llvm::Instruction::Store ) {                                     // BEGIN Store
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                llvm::Value* tempDst = llvm::cast<llvm::StoreInst>(instructionIter)->getPointerOperand();
                llvm::Value* tempSrc = llvm::cast<llvm::StoreInst>(instructionIter)->getValueOperand();

                //Get destination dependency
                if( llvm::isa<llvm::Instruction>(tempDst) ) {
                    std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempDst));
                    if( it != instructionMap_->end() ) {
                        inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempDst));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "+dst Found " << inputVertex->instruction_ << " in instructionMap_\n";
                        }
                    } else {
                        inputVertex = new CDFGVertex;
                        inputVertex->instruction_ = 0x00;
                        inputVertex->haveConst_ = 0;
                        inputVertex->intConst_ = 0x00;
                        inputVertex->floatConst_ = 0x00;
                        inputVertex->doubleConst_ = 0x00;

                        inputVertexID = g.addVertex(inputVertex);
                        (*vertexList_)[&*blockIter].push_back(inputVertex);

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                    //add variable to node use list
                    tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempDst));

                } else if( llvm::isa<llvm::Argument>(tempDst) ) {

                        outputVertex->haveConst_ = 1;
                        outputVertex->intConst_ = 0xFF;
                        outputVertex->floatConst_ = 0x00;
                        outputVertex->doubleConst_ = 0x00;
                        outputVertex->valueName_ = tempSrc->getName().str();

//                     inputVertex = new CDFGVertex;
//                     inputVertex->instruction_ = 0x00;
//                     inputVertex->haveConst_ = 1;
//                     inputVertex->intConst_ = 0xFF;
//                     inputVertex->floatConst_ = 0x00;
//                     inputVertex->doubleConst_ = 0x00;
//                     inputVertex->valueName_ = tempSrc->getName().str();
//
//                     inputVertexID = g.addVertex(inputVertex);
//                     ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                     edgeProp->value_ = llvm::cast<llvm::StoreInst>(instructionIter)->getValueOperand();
//                     g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                     (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                     // create the node/use entries
//                     (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                     //create the node/def entries
//                     (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                } else {
                    if( llvm::isa<llvm::ConstantInt>(tempDst) ) {                           // signed/unsigned ints
                        llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempDst);

                            outputVertex->instruction_ = 0x00;
                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = tempConst->getSExtValue();
                            outputVertex->floatConst_ = 0x00;
                            outputVertex->doubleConst_ = 0x00;

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = tempConst->getSExtValue();
//                         inputVertex->floatConst_ = 0x00;
//                         inputVertex->doubleConst_ = 0x00;
//
//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // Insert edge for const here since we can't discover it when we walk the graph
//                         ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                         edgeProp->value_ = 0x00;
//                         g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                    } else if( llvm::isa<llvm::ConstantFP>(tempDst) ) {                          // floats and doubles
                        llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempDst);

                            outputVertex = new CDFGVertex;
                            outputVertex->instruction_ = 0x00;
                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = 0x00;
                            if(tempDst->getType()->isFloatTy()) {
                                outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                            } else {
                                outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                            }

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = 0x00;
//                         if(tempDst->getType()->isFloatTy()) {
//                             inputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
//                         } else {
//                             inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
//                         }
//
//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // Insert edge for const here since we can't discover it when we walk the graph
//                         ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                         edgeProp->value_ = 0x00;
//                         g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                    } else {
                        inputVertex = new CDFGVertex;
                        inputVertex->instruction_ = 0x00;
                        inputVertex->haveConst_ = 0;
                        inputVertex->intConst_ = 0x00;
                        inputVertex->floatConst_ = 0x00;
                        inputVertex->doubleConst_ = 0x00;

                        inputVertexID = g.addVertex(inputVertex);
                        (*vertexList_)[&*blockIter].push_back(inputVertex);

                        // Insert edge for const here since we can't discover it when we walk the graph
                        ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                        edgeProp->value_ = 0x00;
                        g.addEdge(inputVertexID, outputVertexID, edgeProp);

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                }// END dst dep check

                //Get source dependency
                if( llvm::isa<llvm::Instruction>(tempSrc) ) {
                    std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempSrc));
                    if( it != instructionMap_->end() ) {
                        inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempSrc));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "+src Found " << inputVertex->instruction_ << " in instructionMap_\n";
                        }
                    } else {
//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 0;
//                         inputVertex->intConst_ = 0x00;
//                         inputVertex->floatConst_ = 0x00;
//                         inputVertex->doubleConst_ = 0x00;
//
//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                    //add variable to node use list
                    tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempSrc));
                } else if( llvm::isa<llvm::Argument>(tempSrc) ) {

                    outputVertex->haveConst_ = 1;
                    outputVertex->intConst_ = 0xFF;
                    outputVertex->floatConst_ = 0x00;
                    outputVertex->doubleConst_ = 0x00;
                    outputVertex->valueName_ = tempSrc->getName().str();

//                     inputVertex = new CDFGVertex;
//                     inputVertex->instruction_ = 0x00;
//                     inputVertex->haveConst_ = 1;
//                     inputVertex->intConst_ = 0xFF;
//                     inputVertex->floatConst_ = 0x00;
//                     inputVertex->doubleConst_ = 0x00;
//                     inputVertex->valueName_ = tempSrc->getName().str();

//                     inputVertexID = g.addVertex(inputVertex);
//                     (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                     ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                     edgeProp->value_ = llvm::cast<llvm::StoreInst>(instructionIter)->getValueOperand();
//                     g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                     // create the node/use entries
//                     (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                     //create the node/def entries
//                     (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                } else {
                    if( llvm::isa<llvm::ConstantInt>(tempSrc) ) {                             // signed/unsigned ints
                        llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempSrc);

                        outputVertex->haveConst_ = 1;
                        outputVertex->intConst_ = tempConst->getSExtValue();
                        outputVertex->floatConst_ = 0x00;
                        outputVertex->doubleConst_ = 0x00;

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = tempConst->getSExtValue();
//                         inputVertex->floatConst_ = 0x00;
//                         inputVertex->doubleConst_ = 0x00;

//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // Insert edge for const here since we can't discover it when we walk the graph
//                         ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                         edgeProp->value_ = 0x00;
//                         g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                    } else if( llvm::isa<llvm::ConstantFP>(tempSrc) ) {                         // floats and doubles
                        llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempSrc);

                        outputVertex->haveConst_ = 1;
                        outputVertex->intConst_ = 0x00;
                        if(tempSrc->getType()->isFloatTy()) {
                            outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                        } else {
                            outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                        }

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = 0x00;
//     //                         if(tempSrc->getType()->isFloatTy())
//     //                            inputVertex->floatConst = tempConst->getValueAPF().convertToFloat();
//     //                         else
//                         inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();

//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         // Insert edge for const here since we can't discover it when we walk the graph
//                         ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                         edgeProp->value_ = 0x00;
//                         g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    } else {
                        inputVertex = new CDFGVertex;
                        inputVertex->instruction_ = 0x00;
                        inputVertex->haveConst_ = 0;
                        inputVertex->intConst_ = 0x00;
                        inputVertex->floatConst_ = 0x00;
                        inputVertex->doubleConst_ = 0x00;

                        inputVertexID = g.addVertex(inputVertex);
                        (*vertexList_)[&*blockIter].push_back(inputVertex);

                        // Insert edge for const here since we can't discover it when we walk the graph
                        ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                        edgeProp->value_ = 0x00;
                        g.addEdge(inputVertexID, outputVertexID, edgeProp);

                        // create the node/use entries
                        (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        //create the node/def entries
                        (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }

                }// END src dep check

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

                llvm::errs() << "???????????? STORE END\n";

            //END Store
            } else if( tempOpcode == llvm::Instruction::GetElementPtr ){                                     // BEGIN GEP
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::Value* tempOperand = operandIter->get();

                    if( llvm::isa<llvm::Constant>(tempOperand) ) {
                        if( llvm::isa<llvm::ConstantInt>(tempOperand) ) {                             // signed/unsigned ints
                            llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = tempConst->getSExtValue();
                            outputVertex->floatConst_ = 0x00;
                            outputVertex->doubleConst_ = 0x00;

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = tempConst->getSExtValue();
//                             inputVertex->floatConst_ = 0x00;
//                             inputVertex->doubleConst_ = 0x00;

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        } else if( llvm::isa<llvm::ConstantFP>(tempOperand) ) {                         // floats and doubles
                            llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = 0x00;
                            if(tempOperand->getType()->isFloatTy()) {
                                outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                            } else {
                                outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                            }

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = 0x00;
//     //                            if(tempOperand->getType()->isFloatTy())
//     //                               inputVertex->floatConst = tempConst->getValueAPF().convertToFloat();
//     //                            else
//                             inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                    } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                        std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "**** " << static_cast<llvm::Instruction*>(operandIter->get()) << "   --   " <<
                            llvm::cast<llvm::Instruction>(tempOperand) << "   --   ";
                        }

                        if( it != instructionMap_->end() ) {
                            inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                            }
                        } else {
                            inputVertex = new CDFGVertex;
                            inputVertex->instruction_ = 0x00;
                            inputVertex->haveConst_ = 0;
                            inputVertex->intConst_ = 0x00;
                            inputVertex->floatConst_ = 0x00;
                            inputVertex->doubleConst_ = 0x00;

                            inputVertexID = g.addVertex(inputVertex);
                            (*vertexList_)[&*blockIter].push_back(inputVertex);

                            // create the node/use entries
                            (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                            //create the node/def entries
                            (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                        //add variable to node use list
                        tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));
                    }
                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            //END GEP
            } else if( tempOpcode == llvm::Instruction::ICmp || tempOpcode == llvm::Instruction::FCmp ) {   // BEGIN Int/Float Compare
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::Value* tempOperand = operandIter->get();

                    if( llvm::isa<llvm::Constant>(tempOperand) ) {
                        if( llvm::isa<llvm::ConstantInt>(tempOperand) ) {                              // signed/unsigned ints
                            llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = tempConst->getSExtValue();
                            outputVertex->floatConst_ = 0x00;
                            outputVertex->doubleConst_ = 0x00;

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = tempConst->getSExtValue();
//                             inputVertex->floatConst_ = 0x00;
//                             inputVertex->doubleConst_ = 0x00;

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        } else if( llvm::isa<llvm::ConstantFP>(tempOperand) ) {                          // floats and doubles
                            llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = 0x00;
                            if(tempOperand->getType()->isFloatTy()) {
                                outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                            } else {
                                outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                            }

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = 0x00;
//     //                            if(tempOperand->getType()->isFloatTy())
//     //                               inputVertex->floatConst = tempConst->getValueAPF().convertToFloat();
//     //                            else
//                             inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }
                    } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                        std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "**** " << static_cast<llvm::Instruction*>(operandIter->get()) << "   --   " << llvm::cast<llvm::Instruction>(tempOperand) << "   --   ";
                        }

                        if( it != instructionMap_->end() ) {
                            inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                            }
                        } else {
                            inputVertex = new CDFGVertex;
                            inputVertex->instruction_ = 0x00;
                            inputVertex->haveConst_ = 0;
                            inputVertex->intConst_ = 0x00;
                            inputVertex->floatConst_ = 0x00;
                            inputVertex->doubleConst_ = 0x00;

                            inputVertexID = g.addVertex(inputVertex);
                            (*vertexList_)[&*blockIter].push_back(inputVertex);

                            // create the node/use entries
                            (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                            //create the node/def entries
                            (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                        //add variable to node use list
                        tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));
                    } else if( llvm::isa<llvm::Argument>(tempOperand) || llvm::isa<llvm::GlobalValue>(tempOperand) ) {

                        outputVertex->haveConst_ = 1;
                        outputVertex->intConst_ = 0xFF;
                        outputVertex->floatConst_ = 0x00;
                        outputVertex->doubleConst_ = 0x00;

//                         inputVertex = new CDFGVertex;
//                         inputVertex->instruction_ = 0x00;
//                         inputVertex->haveConst_ = 1;
//                         inputVertex->intConst_ = 0xFF;
//                         inputVertex->floatConst_ = 0x00;
//                         inputVertex->doubleConst_ = 0x00;

//                         inputVertexID = g.addVertex(inputVertex);
//                         (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                         ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                         edgeProp->value_ = 0x00;
//                         g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                         // create the node/use entries
//                         (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                         //create the node/def entries
//                         (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                    }
                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            // END Int/Float Compare
            } else if( llvm::Instruction::isCast(tempOpcode) ) {                                   // BEGIN llvm::cast (Instruction.def)
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::Value* tempOperand = operandIter->get();

                    if( llvm::isa<llvm::Constant>(tempOperand) ) {
                        if( llvm::isa<llvm::ConstantInt>(tempOperand) ) {                              // signed/unsigned ints
                            llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = tempConst->getSExtValue();
                            outputVertex->floatConst_ = 0x00;
                            outputVertex->doubleConst_ = 0x00;

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = tempConst->getSExtValue();
//                             inputVertex->floatConst_ = 0x00;
//                             inputVertex->doubleConst_ = 0x00;

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                        }
                        else if( llvm::isa<llvm::ConstantFP>(tempOperand) )                           // floats and doubles
                        {
                            llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = 0x00;
                            if(tempOperand->getType()->isFloatTy()) {
                                outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                            } else {
                                outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                            }

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = 0x00;
//     //                            if(tempOperand->getType()->isFloatTy())
//     //                               inputVertex->floatConst = tempConst->getValueAPF().convertToFloat();
//     //                            else
//                             inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();

//                                 inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                    } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                        std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "**** " << static_cast<llvm::Instruction*>(operandIter->get()) << "   --   " << llvm::cast<llvm::Instruction>(tempOperand) << "   --   ";
                        }

                        if( it != instructionMap_->end() ) {
                            inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                            }

                        } else {
                            inputVertex = new CDFGVertex;
                            inputVertex->instruction_ = 0x00;
                            inputVertex->haveConst_ = 0;
                            inputVertex->intConst_ = 0x00;
                            inputVertex->floatConst_ = 0x00;
                            inputVertex->doubleConst_ = 0x00;

                            inputVertexID = g.addVertex(inputVertex);
                            (*vertexList_)[&*blockIter].push_back(inputVertex);

                            // create the node/use entries
                            (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                            //create the node/def entries
                            (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                        //add variable to node use list
                        tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));
                    }

                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            //END CAST
            } else if( llvm::Instruction::isBinaryOp(tempOpcode) ) {                // BEGIN binary operators & logical operators -- two operands
                std::vector< llvm::Instruction* > *tempUseVector = new std::vector< llvm::Instruction* >;
                std::vector< llvm::Instruction* > *tempDefVector = new std::vector< llvm::Instruction* >;

                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::Value* tempOperand = operandIter->get();

                    if( llvm::isa<llvm::Constant>(tempOperand) ) {
                        if( llvm::isa<llvm::ConstantInt>(tempOperand) ) {                              // signed/unsigned ints
                            llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = tempConst->getSExtValue();
                            outputVertex->floatConst_ = 0x00;
                            outputVertex->doubleConst_ = 0x00;

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = tempConst->getSExtValue();
//                             inputVertex->floatConst_ = 0x00;
//                             inputVertex->doubleConst_ = 0x00;

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );

                        } else if( llvm::isa<llvm::ConstantFP>(tempOperand) ){                           // floats and doubles
                            llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(tempOperand);

                            outputVertex->haveConst_ = 1;
                            outputVertex->intConst_ = 0x00;
                            if(tempOperand->getType()->isFloatTy()) {
                                outputVertex->doubleConst_ = (double) tempConst->getValueAPF().convertToFloat();
                            } else {
                                outputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();
                            }

//                             inputVertex = new CDFGVertex;
//                             inputVertex->instruction_ = 0x00;
//                             inputVertex->haveConst_ = 1;
//                             inputVertex->intConst_ = 0x00;
//     //                            if(tempOperand->getType()->isFloatTy())
//     //                               inputVertex->floatConst = tempConst->getValueAPF().convertToFloat();
//     //                            else
//                             inputVertex->doubleConst_ = tempConst->getValueAPF().convertToDouble();

//                             inputVertexID = g.addVertex(inputVertex);
//                             (*vertexList_)[&*blockIter].push_back(inputVertex);
//
//                             // Insert edge for const here since we can't discover it when we walk the graph
//                             ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
//                             edgeProp->value_ = 0x00;
//                             g.addEdge(inputVertexID, outputVertexID, edgeProp);
//
//                             // create the node/use entries
//                             (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
//
//                             //create the node/def entries
//                             (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );
                        }
                    } else if( llvm::isa<llvm::Instruction>(tempOperand) ) {
                        std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->find(llvm::cast<llvm::Instruction>(tempOperand));

                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "**** " << static_cast<llvm::Instruction*>(operandIter->get()) << "   --   " << llvm::cast<llvm::Instruction>(tempOperand) << "   --   ";
                        }

                        if( it != instructionMap_->end() ) {
                            inputVertex = instructionMap_->at(llvm::cast<llvm::Instruction>(tempOperand));

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "+Found " << inputVertex->instruction_ << " in instructionMap_\n";
                            }
                        } else {
                            inputVertex = new CDFGVertex;
                            inputVertex->instruction_ = 0x00;
                            inputVertex->haveConst_ = 0;
                            inputVertex->intConst_ = 0x00;
                            inputVertex->floatConst_ = 0x00;
                            inputVertex->doubleConst_ = 0x00;

                            inputVertexID = g.addVertex(inputVertex);
                            (*vertexList_)[&*blockIter].push_back(inputVertex);

                            // create the node/use entries
                            (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );

                            //create the node/def entries
                            (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(inputVertex, new std::vector< llvm::Instruction* >) );
                        }

                        //add variable to node use list
                        tempUseVector->push_back(llvm::cast<llvm::Instruction>(tempOperand));
                    }

                }

                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempUseVector) );

                //create the node/def entries
                tempDefVector->push_back(&*instructionIter);
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, tempDefVector) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }

            //END ALU
            } else {
                // create the node/use entries
                (*useNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );

                //create the node/def entries
                (*defNode_)[&*blockIter]->insert( std::pair<CDFGVertex*, std::vector< llvm::Instruction* >* >(outputVertex, new std::vector< llvm::Instruction* >) );

                if( output_->getVerboseLevel() > 64 ) {
                    for( auto nodeUseEntry = (*useNode_)[&*blockIter]->at(outputVertex)->begin(); nodeUseEntry != (*useNode_)[&*blockIter]->at(outputVertex)->end(); nodeUseEntry++ ) {
                        llvm::errs() << "Node-Use Entry (" << outputVertex->instruction_ << "):  " << *nodeUseEntry << "\n";
                    }

                    for( auto nodeDefEntry = (*defNode_)[&*blockIter]->at(outputVertex)->begin(); nodeDefEntry != (*defNode_)[&*blockIter]->at(outputVertex)->end(); nodeDefEntry++ ) {
                        llvm::errs() << "Node-Def Entry (" << outputVertex->instruction_ << "):  " << *nodeDefEntry << "\n";
                    }
                }
            }

            if( output_->getVerboseLevel() > 64 ) {
                llvm::errs() <<   "********************************************* Ins Map  *********************************************\n";
                for( std::map< llvm::Instruction*,CDFGVertex* >::iterator it = instructionMap_->begin(); it != instructionMap_->end(); ++it ) {
                    llvm::errs() << it->first;
                    llvm::errs() << "  ";
                }
                llvm::errs() << "\n****************************************************************************************************\n";

                llvm::errs() << "\t\t\tnum operands " << instructionIter->getNumOperands() << "\n";
                for( auto operandIter = instructionIter->op_begin(), operandEnd = instructionIter->op_end(); operandIter != operandEnd; ++operandIter ) {
                    llvm::errs() << "\t\t\top get  " << operandIter->get() << "\n";
                    llvm::errs() << "\t\t\top uses " << operandIter->get()->getNumUses() << "\n";
                    llvm::errs() << "\t\t\top dump ";
                    operandIter->get()->dump();

                    if( operandIter->get()->hasName() == 1 ) {
                        llvm::errs() << "\t\t\tfound  ";
                        llvm::errs().write_escaped(operandIter->get()->getName().str()) << "  ";
                    } else {
                        llvm::errs() << "\t\t\tempty \n";
                    }
                    llvm::errs() << "\n";
                }

                llvm::errs() << "\n";
            }
        }
    }

    // should be complete here
    output_->verbose(CALL_INFO, 1, 0, "...Flow Graph Done.\n");

}//END expandBBGraph

void Parser::assembleGraph(void)
{
    // Need to assemble the actual graph -- insert edges for def-use chains
    // This is done for each vertex in the BB graph and then merged
    BBGraph &bbg = *bbGraph_;

    auto vertexMap = bbg.getVertexMap();
    for(auto bbGraphIter = vertexMap->begin(); bbGraphIter != vertexMap->end(); ++bbGraphIter) {
//         std::cout << bbGraphIter->first << "[label=\"";
//         std::cout << bbGraphIter->second.getValue();
//         std::cout << "\"];\n";

        if( output_->getVerboseLevel() > 64 ) {
            llvm::errs() << "\nConstructing graph for basic block " << bbGraphIter->second.getValue() << "...\n";
        }

        llvm::BasicBlock* basicBlock = bbGraphIter->second.getValue();
        CDFG &g = *((*flowGraph_)[basicBlock]);

        if( output_->getVerboseLevel() > 64 ) {
            // Prints list of all instructions in the basic block before processing
            for( auto revIt = (*vertexList_)[basicBlock].rbegin(); revIt != (*vertexList_)[basicBlock].rend(); ++revIt) {
                llvm::errs() << "\t" << (*revIt)->instruction_  << "\n";
            }
            llvm::errs() << "\n\n";
        }

        for( auto revIt = (*vertexList_)[basicBlock].rbegin(); revIt != (*vertexList_)[basicBlock].rend(); ++revIt) {

            if( output_->getVerboseLevel() > 64 ) {
                llvm::errs() << "\n\tInstruction:  " << (*revIt)->instruction_  << "\n";
            }

            // For each vertex, iterate through the instruction's use list
            // Insert an edge between the uses and the next def
            // Defs may be origin node, previous store, etc
            for( auto nodeUseEntry = (*useNode_)[basicBlock]->at(*revIt)->begin(); nodeUseEntry != (*useNode_)[basicBlock]->at(*revIt)->end(); ++nodeUseEntry ) {
                if( output_->getVerboseLevel() > 64 ) {
                    llvm::errs() << "\t\tNode Use Entry:  " << *nodeUseEntry << "\n";
                }

                // Check for actual instructions used in this vertex
                for( auto innerRevIt = revIt; innerRevIt != (*vertexList_)[basicBlock].rend(); ++innerRevIt ) {
                    llvm::Instruction* innerInst = (*innerRevIt)->instruction_;

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "\t\t\tinner:  " << innerInst << "\n";
                    }

                    if( innerRevIt == revIt || innerInst == 0x00 ) {
                        continue;
                    }

                    // FIXME Assume Allocate -> Store -> Load chain and skip edge between allocate and load
                    if( (*revIt)->instruction_->getOpcode() == llvm::Instruction::Load && innerInst->getOpcode() == llvm::Instruction::Alloca ) {
                        continue;
                    }

                    if( innerInst == *nodeUseEntry ) {
                        if( output_->getVerboseLevel() > 64 ) {
                            llvm::errs() << "\t\t\t\tfound:  " << innerInst << "\n";
                        }

                        ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                        edgeProp->value_ = 0x00;
                        g.addEdge(g[*innerRevIt], g[*revIt], edgeProp);
                    }

                    // Check for RAW and WAW deps
                    uint32_t opccode = (*revIt)->instruction_->getOpcode();
                    if( opccode == llvm::Instruction::Load || opccode == llvm::Instruction::Store ) {
                        for( auto operandIter = innerInst->op_begin(), operandEnd = innerInst->op_end(); operandIter != operandEnd; ++operandIter ) {
                            if( innerInst->getOpcode() == llvm::Instruction::Store ) {
                                if( *operandIter == *nodeUseEntry ) {

                                    if( output_->getVerboseLevel() > 64 ) {
                                        llvm::errs() << "\t\t\t\t(" << (*innerRevIt)->instruction_ << ") operand " << *operandIter << "\n";
                                    }

                                    ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                                    edgeProp->value_ = 0x00;
                                    g.addEdge(g[*innerRevIt], g[*revIt], edgeProp);
                                }
                            }
                        }
                    }
                }
            }
        }//END dep chain

        // Finally, check for orphaned nodes
        // Want to connect them with any previous node that is non-zero and has zero out-edges
        output_->verbose(CALL_INFO, 1, 0, "\nChecking for orphans...\n");

        auto cdfgVertexMap = g.getVertexMap();
        for(auto cdfgGraphIter = cdfgVertexMap->begin(); cdfgGraphIter != cdfgVertexMap->end(); ++cdfgGraphIter) {

            llvm::Instruction* tempIns = cdfgGraphIter->second.getValue()->instruction_;
            if( tempIns != 0x00 ) {
                if( tempIns->getOpcode() != llvm::Instruction::Alloca ) {
                    if( cdfgGraphIter->second.getInDegree() <= 0 && cdfgGraphIter->second.getOutDegree() <= 0 ) {
                        for( auto revIt = (*vertexList_)[basicBlock].rbegin(); revIt != (*vertexList_)[basicBlock].rend(); ++revIt) {
                            if( (*revIt)->instruction_ != 0x00 && g.getVertex(g[*revIt])->getOutDegree() <= 0 && (*revIt)->instruction_ != tempIns) {

                                if( output_->getVerboseLevel() > 64 ) {
                                    llvm::errs() << "@@@@@@@@@@@@@@@@@@@@@@@@@    " << (*revIt)->instruction_;
                                    llvm::errs() << "   to   " << tempIns << "\n";
                                }

                                ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                                edgeProp->value_ = 0x00;
                                g.addEdge(g[*revIt], cdfgGraphIter->first, edgeProp);
                            }
                        }
                    }
                }
            }
        }//END orphan check
    }
}//END assembleGraph

void Parser::mergeGraphs()
{
    output_->verbose(CALL_INFO, 1, 0, "\nMerging graphs\n");

    functionGraph_ = new CDFG;
    BBGraph &bbg = *bbGraph_;
    auto bbVertexMap = bbg.getVertexMap();
    for(auto bbGraphIter = bbVertexMap->begin(); bbGraphIter != bbVertexMap->end(); ++bbGraphIter) {
        llvm::BasicBlock* basicBlock = bbGraphIter->second.getValue();
        CDFG &g = *((*flowGraph_)[basicBlock]);
        CDFG::copyGraph(g, *functionGraph_);
    }

    functionGraph_->printDot("00_func.dot");

    // Connect the individual basic blocks back together
    output_->verbose(CALL_INFO, 1, 0, "...adding edges...\n");

    auto funcVertexMap = functionGraph_->getVertexMap();
    for( auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap ->end(); ++vertexIterator ) {
        llvm::Instruction* tempIns = vertexIterator->second.getValue()->instruction_;

        if( tempIns != 0x00 ) {
            if( tempIns->getOpcode() == llvm::Instruction::Br ) {
                llvm::BasicBlock* currentBB = tempIns->getParent();

                if( output_->getVerboseLevel() > 64 ) {
                    llvm::errs() << "Ins " << &*tempIns << " located in " << &*currentBB << "\n";
                }

                // Identify all successor basic bloacks and identify all entry instructions for each one
                // An entry instruction is a non-zero instruction with no in-edges
                llvm::BasicBlock* nextBB;
                std::vector < llvm::Instruction* > connectorList;
                uint32_t totalSuccessors = currentBB->getTerminator()->getNumSuccessors();
                for( uint32_t successor = 0; successor < totalSuccessors; successor++ ) {
                    nextBB = currentBB->getTerminator()->getSuccessor(successor);

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "-----Next BB:  " << nextBB << "\n";
                    }

                    CDFG &g = *((*flowGraph_)[nextBB]);
                    auto vertexMap = g.getVertexMap();
                    for( auto vertexIteratorInner = vertexMap->begin(); vertexIteratorInner != vertexMap ->end(); ++vertexIteratorInner ) {
                        bool found = 0;
                        llvm::Instruction* targetIns = vertexIteratorInner->second.getValue()->instruction_;
                        std::vector< CDFGVertex* >::reverse_iterator targetIter = std::find((*vertexList_)[nextBB].rbegin(), (*vertexList_)[nextBB].rend(), vertexIteratorInner->second.getValue());

                        if( targetIns != 0x00 ) {
                            // If there's only a single instruction then we can link directly
                            // otherwise we need to check for backward deps
                            if( (*vertexList_)[nextBB].size() == 1 ) {
                                connectorList.push_back(targetIns);
                            } else {
                                for( auto operandIter = targetIns->op_begin(), operandEnd = targetIns->op_end(); operandIter != operandEnd; operandIter++ ) {
                                    if( llvm::isa<llvm::Instruction>(*operandIter) ) {
                                        for( std::vector< CDFGVertex* >::reverse_iterator revIt = targetIter + 1; revIt != (*vertexList_)[nextBB].rend(); ++revIt) {
                                            if( (*revIt)->instruction_ == *operandIter ) {
                                                found = 1;
                                                break;
                                            }
                                        }
                                    }
                                }

                                // If the operands do not depend on a previous ins, then we can safely link
                                if( found == 0 && targetIns->getOpcode() > 10 ) {
                                    connectorList.push_back(targetIns);
                                }
                            }
                        }
                    }

                    if( output_->getVerboseLevel() > 64 ) {
                        llvm::errs() << "Dumping connector list for " << &*currentBB << ":  ";
                        for( auto connectorIter = connectorList.begin(); connectorIter != connectorList.end(); connectorIter++ ) {
                            llvm::errs() << *connectorIter << ", ";
                        }
                        llvm::errs() << "\n";
                    }
                }

                for( auto connectorIter = connectorList.begin(); connectorIter != connectorList.end(); connectorIter++ ) {
                    for( auto vertexIteratorInner = funcVertexMap->begin(); vertexIteratorInner != funcVertexMap ->end(); ++vertexIteratorInner ) {
                        if( *connectorIter == vertexIteratorInner->second.getValue()->instruction_ ) {

                            if( output_->getVerboseLevel() > 64 ) {
                                llvm::errs() << "\tConnecting (" << &*currentBB;
                                llvm::errs() << ") " << vertexIterator->second.getValue()->instruction_ << " in " << vertexIterator->second.getValue()->instruction_->getParent();
                                llvm::errs() << " to (" << &*nextBB;
                                llvm::errs() << ") " << vertexIteratorInner->second.getValue()->instruction_ << " in " << vertexIteratorInner->second.getValue()->instruction_->getParent() << "\n";
                            }

                            ParserEdgeProperties* edgeProp = new ParserEdgeProperties;
                            edgeProp->value_ = 0x00;
                            functionGraph_->addEdge(vertexIterator->first, vertexIteratorInner->first, edgeProp  );
                        }
                    }
                }
            }
        }
    }

    output_->verbose(CALL_INFO, 1, 0, "...merge finished\n");
    functionGraph_->printDot("00_func-m.dot");

}//END mergeGraphs

void Parser::printCDFG( const std::string fileName ) const
{
    //open a file for writing (truncate the current contents)
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);

    //check to be sure file is open
    if ( !outputFile ) {
        output_->fatal(CALL_INFO, -1, "Error: Cannot open file %s\n", fileName.c_str());
        exit(0);
    }

    outputFile << "digraph G {" << "\n";

    auto funcVertexMap = functionGraph_->getVertexMap();
    for( auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap ->end(); ++vertexIterator ) {

        outputFile << vertexIterator->first << "[label=\"";
        outputFile << vertexIterator->second.getValue()->instructionName_;
        outputFile << "\"];\n";
    }

    for(auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap->end(); ++vertexIterator) {
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << (*it)->getDestination();
            outputFile << "\n";
        }
    }

    outputFile << "}";
    outputFile.close();


}//END printCDFG

void Parser::printVertex ( const CDFGVertex* vertexIn ) const
{
    std::cerr << vertexIn->instruction_ << std::endl;
    std::cerr << "\t" << vertexIn->instructionName_ << std::endl;
    std::cerr << "\t" << vertexIn->valueName_ << std::endl;
    std::cerr << "\t\t" << vertexIn->haveConst_ << std::endl;
    std::cerr << "\t\t" << vertexIn->intConst_ << std::endl;
    std::cerr << "\t\t" << vertexIn->floatConst_ << std::endl;
    std::cerr << "\t\t" << vertexIn->doubleConst_ << std::endl;
//     std::cerr << "\t" << vertexIn->leftArg_ << std::endl;
//     std::cerr << "\t" << vertexIn->rightArg_ << std::endl;
}

void Parser::printPyMapper( const std::string fileName ) const
{
    std::ofstream outputFile(fileName.c_str(), std::ios::trunc);         //open a file for writing (truncate the current contents)
    if ( !outputFile )                                                   //check to be sure file is open
        std::cerr << "Error opening file.";

    outputFile << "// model intput" << "\n";
    outputFile << "strict digraph  {" << "\n";

    //need this for type size but there should be a better way
    llvm::DataLayout* dataLayout = new llvm::DataLayout(mod_);

    auto funcVertexMap = functionGraph_->getVertexMap();
    for( auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap ->end(); ++vertexIterator ) {

        llvm::Instruction* tempInstruction = vertexIterator->second.getValue()->instruction_;
        if( tempInstruction != NULL ) {
            std::cout << "vertex: " << vertexIterator->first << "\n";

            //temp const vector
            std::map< uint32_t, std::string > constVector;
            //write node ID
            outputFile << vertexIterator->first << " [";
            outputFile << std::flush;

            //if this is a call, we want to swipper-swap the op with the operand
            const char* pos = strstr(tempInstruction->getOpcodeName(), "call");
            std::string newOpcode;

            //write operands
            bool first = 0;
            outputFile << "input=" << "\"";
            for( auto operandIter = tempInstruction->op_begin(), operandEnd = tempInstruction->op_end(); operandIter != operandEnd; ++operandIter ) {
                if( first != 0 ) {
                    outputFile << ":";
                } else {
                    first = 1;
                }

                std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopA" << std::endl;

                if( llvm::isa<llvm::Constant>(operandIter) ) {
                    std::cout << operandIter->getOperandNo() << "\":";
                    std::cout << vertexIterator->second.getValue()->intConst_ << " ";
                    std::cout << vertexIterator->second.getValue()->floatConst_ << " ";
                    std::cout << vertexIterator->second.getValue()->doubleConst_ << " ";
                    std::cout << std::endl;

                    if( llvm::isa<llvm::ConstantInt>(operandIter) ) {
                        llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(operandIter);

                        constVector.emplace( operandIter->getOperandNo(), tempConst->getNameOrAsOperand() );
                        std::cout << tempConst->getNameOrAsOperand() << " -- inboop" << std::endl;
                    } else if( llvm::isa<llvm::ConstantFP>(operandIter) ) {
                        llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(operandIter);

                        //this is super hacky ^-^
                        constVector.emplace( operandIter->getOperandNo(), std::to_string(std::stod(tempConst->getNameOrAsOperand())) );
                        std::cout << std::to_string(std::stod(tempConst->getNameOrAsOperand())) << " -- fpboop" << std::endl;
                    } else if( llvm::isa<llvm::ConstantExpr>(operandIter) ) {
                        outputFile << operandIter->get()->getNameOrAsOperand();
                        std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopB" << std::endl;
                    } else if( llvm::isa<llvm::GlobalValue>(operandIter) ) {
                        if( tempInstruction->getOpcodeName() == pos ) {
                            outputFile << "";
                            newOpcode = operandIter->get()->getNameOrAsOperand();
                        } else {
                            outputFile << operandIter->get()->getNameOrAsOperand();
                            std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopG" << std::endl;
                        }
                    } else {
                        output_->fatal(CALL_INFO, -1, "Error: No valid operand\n");
                        exit(0);
                    }
                } else {
                    outputFile << operandIter->get()->getNameOrAsOperand();
                    std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopC" << std::endl;
                }

            }//end for
            outputFile << "\"" << ", ";

            //write constants
            outputFile << "consts=" << "\"";
            for( auto it = constVector.begin(); it != constVector.end();  ) {
                outputFile << it->second << ":" << it->first;
                ++it;
                if( it != constVector.end() ) {
                    outputFile << " ";
                }
            }
            outputFile << "\"" << ", ";

            //write outputs
            llvm::Value* returnval = llvm::cast<llvm::Value>(tempInstruction);
            outputFile << "output=" << "\"";
            if( returnval->hasName() == 1 ) {
                outputFile << returnval->getName().str();
            }
            outputFile << "\"" << ", ";

            //write op
            outputFile << "op=" << "\"";
            if( tempInstruction->getOpcodeName() == pos ) {
                outputFile << newOpcode;
            } else {
                outputFile << tempInstruction->getOpcodeName();
            }
            outputFile << "\"" << ", ";

            //write type
            outputFile << "type=" << "\"";
            if( tempInstruction->getType()->isSized() ) {
                outputFile << dataLayout->getTypeStoreSize(tempInstruction->getType());
            }
            outputFile << "\"" << "];";

            //finish
            outputFile << "\n";
        }
    }

    outputFile << std::endl;
    for(auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap->end(); ++vertexIterator) {
        std::vector< Edge* >* adjacencyList = vertexIterator->second.getAdjacencyList();

        for( auto it = adjacencyList->begin(); it != adjacencyList->end(); ++it ) {
            outputFile << vertexIterator->first;
            outputFile << "->";
            outputFile << (*it)->getDestination();
            outputFile << "\n";
        }
    }

    outputFile << "}";
    outputFile.close();

}//END printPyMapper

void Parser::collapseInductionVars()
{
    std::cout << "\n\n   ---Collapse Testing---\n" << std::flush;

    auto funcVertexMap = functionGraph_->getVertexMap();
    for( auto vertexIterator = funcVertexMap->begin(); vertexIterator != funcVertexMap ->end(); ++vertexIterator ) {

        llvm::Instruction* tempInstruction = vertexIterator->second.getValue()->instruction_;
        if( tempInstruction != NULL ) {
            std::cout << "vertex: " << vertexIterator->first << "\n";

            //write operands
            for( auto operandIter = tempInstruction->op_begin(), operandEnd = tempInstruction->op_end(); operandIter != operandEnd; ++operandIter ) {
                std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopA" << std::endl;

                if( llvm::isa<llvm::Constant>(operandIter) ) {
                    std::cout << operandIter->getOperandNo() << ": ";
                    std::cout << vertexIterator->second.getValue()->intConst_ << " ";
                    std::cout << vertexIterator->second.getValue()->floatConst_ << " ";
                    std::cout << vertexIterator->second.getValue()->doubleConst_ << " ";
                    std::cout << std::endl;

                    if( llvm::isa<llvm::ConstantInt>(operandIter) ) {
                        llvm::ConstantInt* tempConst = llvm::cast<llvm::ConstantInt>(operandIter);

                        std::cout << tempConst->getNameOrAsOperand() << " -- inboop" << std::endl;

                    } else if( llvm::isa<llvm::ConstantFP>(operandIter) ) {
                        llvm::ConstantFP* tempConst = llvm::cast<llvm::ConstantFP>(operandIter);

                        std::cout << tempConst->getNameOrAsOperand() << " -- fpboop" << std::endl;

                    } else if( llvm::isa<llvm::ConstantExpr>(operandIter) ) {

                        std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopB" << std::endl;

                    } else if( llvm::isa<llvm::GlobalValue>(operandIter) ) {

                        std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopG" << std::endl;
                    } else {
                        output_->fatal(CALL_INFO, -1, "Error: No valid operand\n");
                        exit(0);
                    }
                } else {

                    std::cout << operandIter->get()->getNameOrAsOperand() << " -- boopC" << std::endl;
                }

            }//end for
        }
    }

}//END collapseInductionVars

} // namespace llyr
} // namespace SST



