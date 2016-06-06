// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef INSTRUCTIONINFO_H
#define INSTRUCTIONINFO_H

#include "OpteronDefs.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library

//-------------------------------------------------------------------
/// @brief Holds the static information about an instruction type
///
/// This holds the information about how to execute an instruction
/// along with how to do the bookkeeping for it. Objects of this
/// class will be initialized at simulator initialization from a file
/// of instruction info.
//-------------------------------------------------------------------
class InstructionInfo
{
 public:
   enum OperandSize { OPSIZE8 = 1, OPSIZE16 = 2, OPSIZE32 = 4, OPSIZE64 = 8, OPSIZE128 = 16 };
   enum DataDirection {
      IREG2IREG = 1, IREG2MEM = 2, IREG2FREG = 4,
      FREG2FREG = 8, FREG2MEM = 16, FREG2IREG = 32,
      MEM2IREG = 64, MEM2MEM = 128, MEM2FREG = 256 };
   static bool separateSizeRecords;
   
   InstructionInfo();
   InstructionInfo(const InstructionInfo&);
   ~InstructionInfo();
   void dumpDebugInfo();
   void initStaticInfo(char *name, char *operands, char *operation, char *decodeUnit, 
                       char *execUnits, char *category);
   void initTimings(unsigned int baseLatency, unsigned int memLatency,
                    unsigned int throughputNum, unsigned int throughputDem);
   void initProbabilities(double occurProb, double loadProb, double storeProb,
                          double useHistogram[]);
   InstructionInfo* accumProbabilities(unsigned int opSize, double occurProb, 
                           unsigned long long occurs,
                           unsigned long long loads, unsigned long long stores, bool newiMix=false);
   void accumUseHistogram(unsigned long long occurs, double **depHistogram,
                                           bool partialTotal);
   InstructionInfo* findInstructionRecord(const char *mnemonic, unsigned int iOpSize);
   // Waleed: added the following methods
   InstructionInfo* findInstructionRecord(const char *mnemonic, unsigned int iOpSize, char *op1, char *op2, char *op3);
   unsigned int     findInstructionRecord(const char *mnemonic, unsigned int iOpSize, InstructionInfo** founds);
   //Waleed: the following should print mnemonic, operands, and opSize
   char* printInfo();

   unsigned int getUseDistance(double prob, unsigned int sourceReg);
   bool isConditionalJump();
   bool isUnConditionalJump();
   char* getName() {return name;}
   InstructionInfo* getNext() {return next;}
   void setNext(InstructionInfo *other) {next = other;}
   Category getCategory() {return category;}
   unsigned int getLatency() {return latency;}
   double getOccurProb() {return occurProbability;}
   double getLoadProb() {return loadProbability;}
   double getStoreProb() {return storeProbability;}
   unsigned int decodeCost() {return decodeUnitCost;}
   unsigned int getMaxOpSize();
   double getAvgUseDist();
	void setSourceOps(unsigned int d) {sourceOps=d;}
	unsigned int getSourceOps() {return sourceOps;}
	unsigned int getMops() {return mops;}
	void allocateHistograms(unsigned int d); 
   bool handlesLoad() {return (allowedDataDirs & (MEM2IREG|MEM2MEM|MEM2FREG)) != 0;}
   bool handlesStore() {return (allowedDataDirs & (IREG2MEM|FREG2MEM|MEM2MEM)) != 0;}
   bool needsLoadAddress() {return (!isStackOp || loadProbability < 0.99);}
   bool needsStoreAddress() {return (!isStackOp || storeProbability < 0.99);}
   bool needsFunctionalUnit(FunctionalUnitTypes fut) { return (execUnitMask & fut);}
   bool isFPUInstruction() {return execUnitMask & (FADD|FMUL|FSTORE);}
   unsigned long long getSimulationCount() {return actualOccurs;}
   void incSimulationCount() {actualOccurs++;}
   void incDependenceDists(unsigned int d) {actualDepDists+=d;}
   double getAverageDepDists() {return (double)actualDepDists/actualOccurs;}
   unsigned int throughput() {return throughputDem;}
   static InstructionInfo* createFromString(char* infoString);
 private:
   char* operands;   ///< Number of operands needed
   char* operation;  ///< Operation??
   char* decodeUnit; ///< Which decode unit needed?
   char* execUnits;  ///< Execution units that this insn type can use
                     // optional paths? how do we encode and/or/seq?
   Category category;       ///< High level instruction type category
   double occurProbability; ///< Probability of occurrence 
   double loadProbability;  ///< Probability this insn type does a load
   double storeProbability; ///< Probability this insn type does a store
   char *name;              ///< Instruction type name
   unsigned long long actualOccurs; ///< actual # of occurs in simulation
   unsigned long long actualDepDists; ///< actual distances of use deps in simulation
   unsigned long execUnitMask; ///< Bit mask that encodes FU usage
   unsigned int latency;    ///< Instruction type latency
   unsigned int throughputDem; ///< Throughput (HOW TO USE THIS???)
   bool isStackOp;

   unsigned int decodeUnitCost; ///< number of decode units needed (1/2/3 for single/double/vector)
   unsigned int opSize; ///< OR'd bits of OperandSize (8/16/32/64)
   unsigned int allowedDataDirs; ///< OR'd bits of DataDirection types
   unsigned long long totalOccurs; ///< accumulated total occurs from i-mix file
   unsigned int memLatency; ///< Memory latency (not used??)
   unsigned int throughputNum; ///< Throughput (HOW TO USE THIS???)
   double ** depHistograms; ///< Dependence histograms for each source register
   class InstructionInfo *next;  ///< List ptr
   // Waleed: added the following members to hold short names of operands
   char op1[6]; ///< Instruction's first operand
   char op2[6]; ///< Instruction's 2nd operand
   char op3[6]; ///< Instruction's 3rd operand
   char info[256]; ///< string representation of instruction(mnemonic, operands...etc) for tracing
   unsigned int sourceOps; ///< number of source reg operands
   unsigned int mops; ///< number of macro-ops

};
}//end namespace McOpteron
#endif
