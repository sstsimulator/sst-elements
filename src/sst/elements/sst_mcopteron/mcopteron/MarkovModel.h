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


#ifndef MARKOVMODEL_H
#define MARKOVMODEL_H

#include <stdio.h>
#include <map>
#include <vector>

#include "InstructionInfo.h"
//class InstructionInfo;
//using namespace std; 

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Markov Model to generate instructions based on history of generated instructions
///
//
using std::map;
using std::vector;
class MarkovModel{
 private:
 	int order;
	InstructionInfo **history; // the last 'order' instructions generated

   // transition probabibilities for each history sequence/history 
   // that is, this store the prob that history transitions to a specific instruction
	map<InstructionInfo **, vector<InstructionInfo *> >transMap; 
	map<InstructionInfo **, vector<double> >transProb; //cdf's
	map<InstructionInfo **, double>seqProb; // probabilitie of occurrence for each sequence of 'order' instrs
	InstructionInfo *lookupList; // a pointer to the list holding instruction types
   InstructionInfo * getInstruction(char *mnemonic, unsigned int iOpSize, char *op1, char *op2, char *op3);
   void printSequence(InstructionInfo **seq); 
		
 public:
   MarkovModel(int order, InstructionInfo *head, const char* filename);
   ~MarkovModel();
	InstructionInfo *nextInstruction();
};
}//end namepsace McOpteron
#endif
