// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __APPLE__
  //#ifdef __MACH__
    #include <sys/malloc.h>
  //#endif
#else
  #include <malloc.h>
#endif

#include "InstructionInfo.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
bool InstructionInfo::separateSizeRecords = false;

/// @brief Constructor (but use init() real data)
InstructionInfo::InstructionInfo()
{
   name = operands = operation = decodeUnit = execUnits = 0;
   latency = throughputNum = throughputDem = memLatency = 0;
   occurProbability = loadProbability = storeProbability = 0.0;
   category = UNKNOWN;
   isStackOp = false;
   opSize = 0; 
   totalOccurs = actualOccurs = 0;
   execUnitMask = 0;
   allowedDataDirs = 0;
   next = 0;
   depHistograms = 0;
   op1[0] = '\0';   
   op2[0] = '\0';   
   op3[0] = '\0';
   info[0]= '\0';  
	sourceOps = 0;   
	mops = 1; // initial macro-ops

}

void InstructionInfo::allocateHistograms(unsigned int s) { 
	//fprintf(stderr, "Now in allocateHis\n");
	//fprintf(stderr, "This instruction already has %u sources and depHistograms is %u set\n", sourceOps, (depHistograms)? 1:0 );

   setSourceOps(s);
	depHistograms = new double *[s] ;
   for( unsigned int h = 0 ; h < s ; h++ ) {
      depHistograms[h] = new double[HISTOGRAMSIZE];
      // initialize with zeros
//      memset(depHistograms[h], 0, sizeof(depHistograms[h]));
      memset(depHistograms[h], 0, sizeof(double) * HISTOGRAMSIZE);
   }
}

/// @brief Copy Constructor
InstructionInfo::InstructionInfo(const InstructionInfo& ii)
{
   name = strdup(ii.name);
   operands = strdup(ii.operands);
   operation = strdup(ii.operation);
   decodeUnit = strdup(ii.decodeUnit);
   execUnits = strdup(ii.execUnits);
   latency = ii.latency;
   throughputNum = ii.throughputNum;
   throughputDem = ii.throughputDem;
   memLatency = ii.memLatency;
   occurProbability = loadProbability = storeProbability = 0.0;
   category = ii.category;
   isStackOp = ii.isStackOp;
   opSize = ii.opSize; 
   totalOccurs = actualOccurs = 0;
   execUnitMask = ii.execUnitMask;
   allowedDataDirs = ii.allowedDataDirs;
   next = 0;
   //for (int i=0; i < HISTOGRAMSIZE; i++)
   //   toUseHistogram[i] = 0.0;
}


/// @brief Destructor
InstructionInfo::~InstructionInfo()
{
   if (name) free(name);
   if (operands) free(operands);
   if (operation) free(operation);
   if (decodeUnit) free(decodeUnit);
   if (execUnits) free(execUnits);
   if(depHistograms) { 
      for( unsigned int i = 0 ; i < sourceOps ; i++ )
		   delete [] depHistograms[i] ;
      delete [] depHistograms ;	
   }
}

/// @brief Dump debug info about instruction record
///
void InstructionInfo::dumpDebugInfo()
{
   fprintf(stderr, "II: name (%s) operands (%s) operation (%s) execUnits (%s)\n",
           name, operands, operation, execUnits);
   fprintf(stderr, "II: category %u opsize %u stackop %s unitmask %lu, datadirs %u\n",
           category, opSize, isStackOp?"T":"F", execUnitMask, allowedDataDirs);
}

/// @brief Return largest possible operand size
///
unsigned int InstructionInfo::getMaxOpSize()
{
   if (opSize & OPSIZE128) return 128;
   if (opSize & OPSIZE64) return 64;
   if (opSize & OPSIZE32) return 32;
   if (opSize & OPSIZE16) return 16;
   if (opSize & OPSIZE8) return 8;
   return 64; // never happen?
}

double InstructionInfo::getAvgUseDist()
{
   return 0.0;
}


/// @brief Initializer
///
void InstructionInfo::initStaticInfo(char *name, char *operands, char *operation,
                                     char *decodeUnit, char *execUnits, char *category)
{
   if (name) {
      if (name[0] == '*')
         this->name = strdup(name+1);
      else
         this->name = strdup(name);
   }
   // Waleed: parse operands and save a simple form of operand 1,2, and 3
   // this is needed to simplify finding a record that match mnemonic, opSize, and operands
   // the operands must be simple such as reg, imm, mem only; the opSize takes care of the rest
   if (operands) { 
      this->operands = strdup(operands);
      char *ops_cpy = strdup(operands); 
      char *op1 = strtok(ops_cpy, ",");
      char *op2 = strtok(0, ",");
      char *op3 = strtok(0, ",");
      if (strstr(op1,"imm")) { 
         strcpy(this->op1,"imm");
      } 
      else if (strstr(op1,"none")) { 
         strcpy(this->op1,"none");
      } 
      else if (strstr(op1,"disp")) { 
         strcpy(this->op1,"disp");
      } 
      else if (strstr(op1,"reg") || strstr(op1,"xmm") || strstr(op1,"mm")) { 
         strcpy(this->op1,"reg");
      } 
      else if (strstr(op1,"mem")) { 
         strcpy(this->op1,"mem");
      } 
      else { 
         fprintf(stderr, "Uknown operand (%s) for mnemonic(%s)\n", op1, this->name); 
      }
      if(op2) { 
         if (strstr(op2,"imm")) { 
            strcpy(this->op2,"imm");
         } 
         else if (strstr(op2,"disp")) { 
            strcpy(this->op2,"disp");
         } 
         else if (strstr(op2,"reg") || strstr(op2,"xmm") || strstr(op2,"mm")) { 
            strcpy(this->op2,"reg");
         } 
         else if (strstr(op2,"mem")) { 
            strcpy(this->op2,"mem");
         } 
         else { // consider it immediate?
            strcpy(this->op2,"imm");
         }
      }   
      // op3 can only be an imm or reg
      if(op3) { 
         if (strstr(op3,"imm")) { 
            strcpy(this->op3,"imm");
         } 
         else { 
            strcpy(this->op3,"reg");
         }
      }
      free(ops_cpy);
   } // end processing operands
   
   if (operation) this->operation = strdup(operation);
   if (decodeUnit) this->decodeUnit = strdup(decodeUnit);
   if (execUnits) this->execUnits = strdup(execUnits);
   // process exec units to figure out execution paths
   if (execUnits) {
      if (strstr(execUnits,"ALU0")) {
         this->category = MULTINT;
         execUnitMask = ALU0;
      } else if (strstr(execUnits,"ALU2")) {
         this->category = SPECIALINT;
         execUnitMask = ALU2;
      } else if (strstr(execUnits,"AGU")) {
         this->category = GENERICINT;
         execUnitMask = AGU;
      } else if (strstr(execUnits,"FADD")) {
         this->category = FLOAT;
         execUnitMask = FADD;
      } else if (strstr(execUnits,"FMUL")) {
         this->category = FLOAT;
         execUnitMask = FMUL;
      } else if (strstr(execUnits,"FSTORE")) {
         this->category = FLOAT;
         execUnitMask = FSTORE;
      } else {
         this->category = GENERICINT;
         execUnitMask = ALU0 | ALU1 | ALU2;
      }
   }
   // check if a stack instruction (these do not need to
   // use an AGU for address generation)
   if (operation && strstr(operation, "STACK"))
      isStackOp = true;
   // process operands to find data direction and size
   while (operands) { // really an if, but we want to use break
      char *dest, *src, *opcopy;
      if (strstr(operands,"128") || strstr(operands,"xmm"))
         opSize |= OPSIZE128;
      else if (strstr(operands,"64") || (strstr(operands,"mm") && !strstr(operands,"imm")))
         opSize |= OPSIZE64;
      else if (strstr(operands,"32"))
         opSize |= OPSIZE32;
      else if (strstr(operands,"16"))
         opSize |= OPSIZE16;
      else if (strstr(operands,"8"))
         opSize |= OPSIZE8;
      if (!opSize)
         opSize |= OPSIZE64; // default
      opcopy = strdup(operands);
      dest = strtok(opcopy, ",");
      src = strtok(0,",");
      if (!dest) {
         free(opcopy);
         break;
      }
      if (strstr(dest,"reg")) {
         if (!src) // unary op
            allowedDataDirs |= IREG2IREG;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2IREG;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2IREG;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2IREG;
      }
      if (strstr(dest,"mm")) {
         if (!src) // unary op
            allowedDataDirs |= FREG2FREG;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2FREG;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2FREG;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2FREG;
      }
      if (strstr(dest,"mem")) {
         if (!src) // unary op
            allowedDataDirs |= MEM2MEM;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2MEM;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2MEM;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2MEM;
      }
      free(opcopy);
      break; // make the while an if
   }

   // process decode string and assign costs
   if (!decodeUnit || !strcmp(decodeUnit,"single"))
      decodeUnitCost = 1;
   else if (!strcmp(decodeUnit,"double"))
      decodeUnitCost = 2;
   else if (!strcmp(decodeUnit,"vector"))
      decodeUnitCost = 3;
   else
      decodeUnitCost = 1;

   // a quick way to determine the number of MOPs for this instruction
   // this needs to be fed from the instruction definition table
   mops = decodeUnitCost;

   if (Debug>2)
      fprintf(stderr, "IInfo-si: (%s) (%s)%u:%u (%s) (%s) (%u) (%s)\n",
              this->name, operands, opSize, allowedDataDirs, decodeUnit, 
              execUnits, this->category, operation);
}

/// @brief Initializer
///
void InstructionInfo::initTimings(unsigned int baseLatency, unsigned int memLatency,
                                  unsigned int throughputNum, unsigned int throughputDem)
{
   this->latency = baseLatency;
   this->memLatency = memLatency;
   this->throughputNum = throughputNum;
   this->throughputDem = throughputDem;
}

/// @brief Initializer
///
void InstructionInfo::initProbabilities(double occurProb, double loadProb,
                                        double storeProb, double useHistogram[])
{
   occurProbability = occurProb;
   loadProbability = loadProb;
   storeProbability = storeProb;
   //for (int i=0; i < HISTOGRAMSIZE; i++)
   //   toUseHistogram[i] = useHistogram[i];
}

/// @brief Use this to group together multiple Pin instruction types
/// This now either accumulates probabilities OR it clones the 
/// instruction info object and generates one just for this opSize,
/// so each operand size will have its own instruction info record
InstructionInfo* InstructionInfo::accumProbabilities(unsigned int opSize,
                               double occurProb, unsigned long long occurs,
                               unsigned long long loads, unsigned long long stores, bool newiMix)
{
   int i; unsigned int mopSize;
   InstructionInfo* ni;
   // if no occurs, don't do anything
   if (occurs == 0)
      return this;
   mopSize = OPSIZE64;
   if (opSize == 8)   mopSize = OPSIZE8;
   if (opSize == 16)  mopSize = OPSIZE16;
   if (opSize == 32)  mopSize = OPSIZE32;
   if (opSize == 64)  mopSize = OPSIZE64;
   if (opSize == 128) mopSize = OPSIZE128;
   if (separateSizeRecords && this->opSize != mopSize && !newiMix) {
      // new behavior: create a new info record for this size
      // -- assuming it is subsumed and we'll separate it out
      fprintf(stderr,"removing %s/%d from old record\n",name,opSize);
      ni = new InstructionInfo(*this); // clone
      ni->next = next;
      next = ni;
      ni->opSize = mopSize;
      this->opSize &= ~mopSize; // remove this opSize from old record
      ni->accumProbabilities(opSize, occurProb, occurs, loads, stores);
      return ni;
   }
   // this is the old behavior: accumulate all operand-sizes together
   // overall i-mix probability can just be added in
   occurProbability += occurProb;
   // reduce existing load/store probs by correct fraction
   if (totalOccurs) {
      // Waleed: do not adjust these probls for newimix because that's already known and accounted for 
      // for any subsequent records coming here!! 
      if(!newiMix)       
         loadProbability  = loadProbability  * totalOccurs / (totalOccurs + occurs);
      if(!newiMix)       
         storeProbability = storeProbability * totalOccurs / (totalOccurs + occurs);
      for(unsigned int s=0; s<sourceOps; s++) {
		   for (i=0; i < HISTOGRAMSIZE; i++)
            depHistograms[s][i] = depHistograms[s][i] * totalOccurs / (totalOccurs + occurs);
      }
   }
   // update total observations
   totalOccurs += occurs;
   // add in new probs
   if(!newiMix) {   
      loadProbability  += (double) loads  / totalOccurs;
      storeProbability += (double) stores / totalOccurs;
   } 
   else {  // Waleed: these should not change for newiMix            
      loadProbability  = (double) loads  / occurs;
      storeProbability = (double) stores / occurs;
   }    
   return this;
}

/// @brief accumulates the use distance histogram
/// this used to be in accumProbabilities but with the new
/// imix file, it is useful to be separate
void InstructionInfo::accumUseHistogram(unsigned long long occurs, double ** useHistogram,
                                        bool partialTotal)
{
   int i;
   for(unsigned int s = 0; s < sourceOps; s++) { 
      for (i=0; i < HISTOGRAMSIZE; i++) {
         if (partialTotal) {
            // we are accumulating as we go, so we have to proportionally shrink
            // what we have so far
            depHistograms[s][i] *= (double) (totalOccurs - occurs) / (totalOccurs);
            depHistograms[s][i] += useHistogram[s][i] * (double) occurs / (totalOccurs + 0.001);
         } else {
            // full total is known, just use it as is
            depHistograms[s][i] += useHistogram[s][i] * (double) occurs / (totalOccurs + 0.001);
         }
      }
   }      //toUseHistogram[i] += useHistogram[i] * occurs / 
      //                    (totalOccurs + occurs + 0.00001);
      // JEC: This is not quite right because histogram might not have
      // as many entries as occurs, since the use might never be recorded
      // if it gets too far. But it is close.
}

// TODO: use instruction size and operands (not passing these yet! -- need
// at least the load/store flags)
InstructionInfo* InstructionInfo::findInstructionRecord(const char *mnemonic,
                                                        unsigned int iOpSize)
{
   char *searchName=0; 
   char *p;
   OperandSize opSize;
   InstructionInfo *it = this, *first = 0;
   if (!mnemonic) return 0;
   // convert all conditional jumps into a generic JCC
   if (mnemonic[0] == 'J' && strcmp(mnemonic,"JMP") && !strstr(mnemonic,"CXZ"))
      searchName = strdup("JCC");
   // convert all conditional cmov's into a generic CMOVCC
   else if (strstr(mnemonic,"CMOV") == mnemonic)
      searchName = strdup("CMOVCC");
   // convert all conditional set's into a generic SETCC
   else if (strstr(mnemonic,"SET") == mnemonic)
      searchName = strdup("SETCC");
   // convert all conditional loop's into a generic LOOPCC
   else if (strstr(mnemonic,"LOOP") == mnemonic)
      searchName = strdup("LOOPCC");
   else
      searchName = strdup(mnemonic);
   // remove any '_NEAR' or '_XMM' extensions
   if ((p=strstr(searchName,"_NEAR")))
      *p = '\0';
   if ((p=strstr(searchName,"_XMM")))
      *p = '\0';
   switch (iOpSize) {
    case 8: opSize = OPSIZE8; break;
    case 16: opSize = OPSIZE16; break;
    case 32: opSize = OPSIZE32; break;
    case 64: opSize = OPSIZE64; break;
    case 128: opSize = OPSIZE128; break;
    default: opSize = OPSIZE64; 
   }
   if (Debug>2) fprintf(stderr, "findII: searching for (%s)...", searchName);
   first = 0;
   while (it) {
      if (!strcmp(it->name, searchName)) {
         if (!first)
            first = it;
         if (opSize & it->opSize) // matched name and op size, so done!
            break;
      }
      it = it->next;
   }
   // old search code -- maybe no good? (relies on table order too much?)
   //while (it && strcmp(it->name, searchName)) {
   //   it = it->next;
   //}
   //first = it;
   //while (it && !strcmp(it->name, searchName) && !(opSize & it->opSize)) {
   //   it = it->next;
   //}
   if (!it || strcmp(it->name, searchName))
      it = first; // reset back to first one found
   if (Debug>2) fprintf(stderr, "%p (%s)\n", it, it->name);
   free(searchName);
   return it;
}

// Waleed: a new version of the method that matches based on operands and opSize too
// if a record is not found based on operands, skip it, ie return a null
InstructionInfo* InstructionInfo::findInstructionRecord(const char *mnemonic,
                                                        unsigned int iOpSize, char *opp1, char *opp2, char *opp3)
{
   char *searchName=0; 
   OperandSize opSize;
   InstructionInfo *it = this, *first = 0;
   if (!mnemonic) return 0;
   // the following is already done before
#if 0	
   char *p;
   // convert all conditional jumps into a generic JCC
   if (mnemonic[0] == 'J' && strcmp(mnemonic,"JMP") && !strstr(mnemonic,"CXZ"))
      searchName = strdup("JCC");
   // convert all conditional cmov's into a generic CMOVCC
   else if (strstr(mnemonic,"CMOV") == mnemonic)
      searchName = strdup("CMOVCC");
   // convert all conditional set's into a generic SETCC
   else if (strstr(mnemonic,"SET") == mnemonic)
      searchName = strdup("SETCC");
   // convert all conditional loop's into a generic LOOPCC
   else if (strstr(mnemonic,"LOOP") == mnemonic)
      searchName = strdup("LOOPCC");
   else
      searchName = strdup(mnemonic);
		
   // remove any '_NEAR' or '_XMM' extensions
   if ((p=strstr(searchName,"_NEAR")))
      *p = '\0';
   if ((p=strstr(searchName,"_XMM")))
      *p = '\0';
#endif

	searchName = strdup(mnemonic);

   switch (iOpSize) {
    case 8: opSize = OPSIZE8; break;
    case 16: opSize = OPSIZE16; break;
    case 32: opSize = OPSIZE32; break;
    case 64: opSize = OPSIZE64; break;
    case 128: opSize = OPSIZE128; break;
    default: opSize = OPSIZE64; 
   }
   if (Debug>2) 
      fprintf(stderr, "findII: searching for %s %s %s %s and opSize %u\n", searchName, opp1, opp2, opp3, opSize);
   first = 0;
   int flag = 0; 
   while (it) {
      // match based on mnemonic
      if (!strcmp(it->name, searchName)) {
         // comment out the following two lines to only return matches based at least on operands!
         //if (!first)
         //   first = it;
         // match based on operands
         if(!strcmp(it->op1, opp1) && !strcmp(it->op2, opp2) && !strcmp(it->op3, opp3) ) { 
            // match also based on opSize
            if (opSize & it->opSize) { 
               flag = 0; 
               break;
            }   
            // okay now we'll reset 'first', just in case we don't find an exact match
            // that is, flag=1 if we match based on mnemonic and operands but not size
            first = it; 
            flag = 1;            
         }
      }
      it = it->next;
   }

   // if a match was not found and this instruction has one operand, then we will try to match it up with
   // the same instruction with 2-operands; this is due to the problem of PIN generating one-operand isntructions
   // for two-operand instructions such as 'ADD imm' where it should be 'ADD reg, imm'
   if(opp2[0] == '\0' && opp3[0] == '\0' && (!it || strcmp(it->name, searchName)) && !flag) { 
      // make the 2nd op to be a 'reg' and search now
      strcpy(opp2,"reg");
      it = this;
      first = 0; 
      while (it) {
         // match based on mnemonic
         if (!strcmp(it->name, searchName)) {
            // match based on operands
            if(!strcmp(it->op1, opp1) && !strcmp(it->op2, opp2) && !strcmp(it->op3, opp3) ) { 
               // match also based on opSize
               if (opSize & it->opSize) { 
                  flag = 0; 
                  break;
               }   
               first = it; 
               flag = 1;
            }
         }
         it = it->next;
      }
   }
   // if we still don't hit, try with 'imm'
   if(opp2[0] == '\0' && opp3[0] == '\0' && (!it || strcmp(it->name, searchName)) && !flag) { 
      // make the 2nd op to be a 'reg' and search now
      strcpy(opp2,"imm");
      it = this; 
      first = 0; 
      while (it) {
         // match based on mnemonic
         if (!strcmp(it->name, searchName)) {
            // match based on operands
            if(!strcmp(it->op1, opp1) && !strcmp(it->op2, opp2) && !strcmp(it->op3, opp3) ) { 
               // match also based on opSize
               if (opSize & it->opSize) { 
                  flag = 0; 
                  break;
               }   
               first = it; 
               flag = 1;
            }
         }
         it = it->next;
      }
   }

   // that's all, now finish up
   if ((!it || strcmp(it->name, searchName)) && !flag) { 
      // only found the mnemonic?
      if (Debug>2) fprintf(stderr, "Warning: a match based on operands/opSize was not found\n"); 
      it = first; // reset back to first one found
   }
   // found a match based on operands but not on opsize?
   if (flag) {
      if (Debug>2) fprintf(stderr, "Warning: a match was found based on operands but not OpSize\n"); 
      it = first;      
   }
   if (Debug>2 && it) fprintf(stderr, "%p (%s)\n", it, it->name);
   free(searchName);
   return it;
}

// Waleed: a version of the find method that returns an array of found records
// that matching is only based on nmemonic and OpSize because the usedistances are only
// measured and distinguished using those two parameters(mnemonic+OpSize)
unsigned int InstructionInfo::findInstructionRecord(const char *mnemonic,
                                                        unsigned int iOpSize, InstructionInfo** founds)
{
   char *searchName=0; 
   char *p;
   OperandSize opSize;
//   InstructionInfo *it = this, *first = 0;
   InstructionInfo *it = this;
   if (!mnemonic) return 0;
   // convert all conditional jumps into a generic JCC
   if (mnemonic[0] == 'J' && strcmp(mnemonic,"JMP") && !strstr(mnemonic,"CXZ"))
      searchName = strdup("JCC");
   // convert all conditional cmov's into a generic CMOVCC
   else if (strstr(mnemonic,"CMOV") == mnemonic)
      searchName = strdup("CMOVCC");
   // convert all conditional set's into a generic SETCC
   else if (strstr(mnemonic,"SET") == mnemonic)
      searchName = strdup("SETCC");
   // convert all conditional loop's into a generic LOOPCC
   else if (strstr(mnemonic,"LOOP") == mnemonic)
      searchName = strdup("LOOPCC");
   else
      searchName = strdup(mnemonic);
   // remove any '_NEAR' or '_XMM' extensions
   if ((p=strstr(searchName,"_NEAR")))
      *p = '\0';
   if ((p=strstr(searchName,"_XMM")))
      *p = '\0';
   switch (iOpSize) {
    case 8: opSize = OPSIZE8; break;
    case 16: opSize = OPSIZE16; break;
    case 32: opSize = OPSIZE32; break;
    case 64: opSize = OPSIZE64; break;
    case 128: opSize = OPSIZE128; break;
    default: opSize = OPSIZE64; 
   }
   if (Debug>2) fprintf(stderr, "findII: searching for (%s)...", searchName);
//   first = 0;
   int counter=0; 
   while (it) {
      // match based on mnemonic and opSize and add found record to result array
      if (!strcmp(it->name, searchName) && (opSize & it->opSize)) 
         founds[counter++] = it;      
      it = it->next;
   }
   if (Debug>2 && it) fprintf(stderr, "%p (%s)\n", it, it->name);
   free(searchName);
   // update return size
   return counter;
}


/// @brief Create and initialize an object from string data
///
InstructionInfo* InstructionInfo::createFromString(char* infoString)
{
   return 0;
}

/// @brief Return true if this instruction is a condition jump type
///
bool InstructionInfo::isConditionalJump()
{
   if (operation && strstr(operation,"COND"))
      return true;
   else
      return false;
}

/// @brief Return true if this instruction is an uncoditional jump type (ie jmp or call/ret)
///
bool InstructionInfo::isUnConditionalJump()
{
   if (operation && strstr(operation,"JMP"))
      return true;
   else if(operation && strstr(operation, "STACK")) { 
      if(name && (strstr(name, "CALL") || strstr(name, "RET")))
         return true; 
   } 
   
   return false;
}

/// @brief Given a probability, retrieve matching use distance
///
/// This samples the CDF depHistograms for a specifi source register to retrieve the distance
/// to the instruction that produces what this register
///
unsigned int InstructionInfo::getUseDistance(double prob, unsigned int sourceReg)
{
   unsigned int i;
   for (i=0; i < HISTOGRAMSIZE; i++)
      if (prob <= depHistograms[sourceReg][i])
         break;
   if (i == HISTOGRAMSIZE)
      return 0;
   return i;
}
// Waleed: this is a new method
char* InstructionInfo::printInfo() { 
   if(this->op3[0] !='\0')
      sprintf(this->info, "%s\t%s,%s,%s\t%u", this->name, this->op1, this->op2, this->op3, this->opSize);
   else if(this->op2[0] !='\0')
      sprintf(this->info, "%s\t%s,%s\t%u", this->name, this->op1, this->op2, this->opSize);
   else 
      sprintf(this->info, "%s\t%s\t%u", this->name, this->op1, this->opSize);
   return this->info; 
}
}//End namespace McOpteron
