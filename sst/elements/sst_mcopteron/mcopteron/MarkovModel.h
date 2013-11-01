
#ifndef MARKOVMODEL_H
#define MARKOVMODEL_H

#include <stdio.h>
#include <map>
#include <vector>

#include "InstructionInfo.h"
#include "Random.h"
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
  void setRandFunc(RandomFunc_t _R){rand.setrand_function(_R);}
  void setSeedFunc(SeedFunc_t _S){rand.setseed_function(_S);}
  void seedRandom(uint32_t _s){rand.seed(_s);}
 private:
  double genRandomProbability(){return rand.next();}
  Random rand;
};
}//end namepsace McOpteron
#endif
