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

 
//#include <malloc.h>
#include <memory.h>
#include <cstdlib>
#include <map>

#include "MarkovModel.h"

//using namespace std;

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor
///  Read transitionProbability file and set up the object
///  The expected format of the file is as follows:
///  <empty_line>
///  Sequence prob: 1.147129
///     TEST 8 reg, reg
///     JNZ 8 ptr
///     MOV 8 reg, mem
///  <empty_line>
///     Transition Probabilities:
///     Prob            Instr the sequence transitions to
///     100.000000      CMP 8 reg, mem  
/// **Repeat the above sequence for every 3-instr sequence 
MarkovModel::MarkovModel(int order, InstructionInfo *head, const char* filename)
{

   char line[128];
   char mnemonic[24], op1[24], op2[24], op3[24];
   unsigned int iOpSize;
   double histProb, trProb, rTotal=0.0, maxProb=0.0;
   FILE *inf;
   InstructionInfo *it;
   InstructionInfo ** hist;
   bool skip = false;
   if(order <= 0) {   
      fprintf(stderr, "Error: illegal order passed to markove model...quiting...\n");
      exit(-1);
   }
   this->order = order;
	this->lookupList = head;  
   hist = 0;
   if (Debug>=3)
      fprintf(stderr, "\nNow Reading markov transition probabilities file (%s)\n\n", filename);

   inf = fopen(filename, "r");
   if (!inf) {
      fprintf(stderr, "Error opening file (%s)...quiting...\n", filename);
      exit(-1);
   }
   // skip first line ( first line of file always empty )
   if (!fgets(line, sizeof(line), inf)) {
      fprintf(stderr, "First line not read! (%s), aborting...\n", line);
      fclose(inf);
      exit(-1);
   }
   // now read the rest of the file
   while (!feof(inf)) {
      // read line
      if (!fgets(line, sizeof(line), inf)) 
         break;
      if (sscanf(line,"Sequence prob: %lf", &histProb) != 1) {
         fprintf(stderr, "Unknown line (%s), skipping\n", line);
         continue;
      }
      if (Debug>=3)
         fprintf(stderr, "\tReading a new sequence of %d instructions:\n", order); 
      
      // allocate mem for this sequence (hist)
      hist = new InstructionInfo *[order];

		// now read the sequence of 'order instructions
      for(int i=0; i<order; i++) { 
         iOpSize = 64;
         op1[0] = op2[0] = op3[0] = '\0';
         if (!fgets(line, sizeof(line), inf)) {
            fprintf(stderr, "Error reading file (%s), aborting...\n", line);
            fclose(inf);
            exit(-1);
         }
         if (sscanf(line, "%s %u %[^,]%*c %[^,]%*c %s",mnemonic,&iOpSize,op1,op2,op3) != 5) {
            if (sscanf(line, "%s %u %[^,]%*c %s",mnemonic,&iOpSize,op1,op2) != 4) {
               if (sscanf(line, "%s %u %s",mnemonic,&iOpSize,op1) != 3) {
                  if (sscanf(line, "%s %u",mnemonic, &iOpSize) != 2) {
                     fprintf(stderr,"Markov Trans Prob: BAD LINE: (%s)\n",line);
                     fprintf(stderr, "Error reading file (%s), aborting...\n", line);
                     fclose(inf);
                     exit(-1);                     
                  }
               }
            }
         }
         if(Debug>=3)
            fprintf(stderr, "Parsed instr: (%s) (%s) (%s) (%s)\n", mnemonic, op1, op2, op3);
			// now we have our instruction read
			it = getInstruction(mnemonic, iOpSize, op1, op2, op3);
         if (!it) {
            if(Debug>=3) { 
               fprintf(stderr, "Warning: instruction record not found for (%s) (%s) (%s) (%s)\n", 
                       mnemonic, op1, op2, op3);
               fprintf(stderr, "\tSequence Prob:%f ... skipping this sequence\n", histProb); 
            }
            skip = true; 
            break;
         }
         hist[i] = it;
      } // end for 
      if (Debug>=3)
         fprintf(stderr, "\tDone reading the new sequence of %d instructions\n", order); 
     
      // if there was a problem finding an instruction, skip to the next 
      if(skip) { 
         skip = false;
         continue;
      }
      // now we have a sequence
      seqProb[hist]=histProb/100.00; // update prob

      // keep track of the most probable sequence/histor
      if(histProb > maxProb) { 
         maxProb = histProb;
         history =  hist; 
      }
      // skip 3 lines
      if (!fgets(line, sizeof(line), inf)) {
         fprintf(stderr, "Error reading file (%s), aborting...\n", line);
         fclose(inf);
         exit(-1);
      }
      if (!fgets(line, sizeof(line), inf)) {
         fprintf(stderr, "Error reading file (%s), aborting...\n", line);
         fclose(inf);
         exit(-1);
      }
      if (!fgets(line, sizeof(line), inf)) {
         fprintf(stderr, "Error reading file (%s), aborting...\n", line);
         fclose(inf);
         exit(-1);
      }
      if (Debug>=3)
         fprintf(stderr, "\tReading instructions to which the above sequence transitions:\n"); 
      rTotal = 0.0;
      // now read the transitions for this sequence and their probabilities
      // what we are reading is actually 
      while (!feof(inf)) {
         iOpSize = 64;
         op1[0] = op2[0] = op3[0] = '\0';
         trProb = 0.0; 
         if (!fgets(line, sizeof(line), inf)) 
            break;
         if (sscanf(line, "%lf %s %u %[^,]%*c %[^,]%*c %s",&trProb, mnemonic,&iOpSize,op1,op2,op3) != 6) {
            if (sscanf(line, "%lf %s %u %[^,]%*c %s",&trProb, mnemonic,&iOpSize,op1,op2) != 5) {
               if (sscanf(line, "%lf %s %u %s",&trProb, mnemonic,&iOpSize,op1) != 4) {
                  if (sscanf(line, "%lf %s %u",&trProb, mnemonic, &iOpSize) != 3) {
                     break;
                  }
               }
            }
         }
         if(Debug>=3)
            fprintf(stderr, "Parsed instr: (%s) (%s) (%s) (%s) (%lf)\n", mnemonic, op1, op2, op3, trProb);
			// now we have our instruction read
			it = getInstruction(mnemonic, iOpSize, op1, op2, op3);
         if (!it) {
            fprintf(stderr, "\tWarning: instruction record not found ...skipping....\n");
            continue;
         }
         rTotal += trProb; 
         // now update records
         // insert at beginning of vectors because values are read in descending order
         transMap[hist].insert(transMap[hist].begin(), it);
         transProb[hist].insert(transProb[hist].begin(), trProb);
      }
      // create a cdf; 
      trProb = 0.0; 
      for(unsigned int i=0; i<transProb[hist].size(); i++)  { 
         trProb += transProb[hist][i]; 
			transProb[hist][i] =  trProb/rTotal;
      }
   }
   fclose(inf);
}

// find next instruction based on current history
InstructionInfo * MarkovModel::nextInstruction( ) { 
   InstructionInfo *ret = 0; 
   InstructionInfo **hist=0; 
   bool found = false; 

   if(Debug>=2)
	   fprintf(stderr, "Markov Model: Generating a next instruction\n");

   //printSequence(history); 

#if 0
	map<InstructionInfo **, vector<double> >::iterator iter; 
   for(iter=transProb.begin(); iter!=transProb.end(); iter++) { 
      hist = iter->first;
		found = true; 
      for(int i=0; i<order; i++)
          found = found && (hist[i] == history[i]); 

      if(found)
		   break;

		found = false; 
      
   }
#endif 
   // sanity check; we should always hit in the table because we make sure 'history' exists before we exit this func
   if(transProb.find(history) != transProb.end()) { 
      found = true; 
      hist = history; 
   }
   if(!found && Debug >=2) {
      fprintf(stderr, "Markov Model: current sequence not found in transition table\n");
	   printSequence(history);
   }

   // now look at CDF and find instr
   double p = genRandomProbability();
   if(Debug>=3)
	   fprintf(stderr, "Markov Model: prob:%lf\n", p);
   for(unsigned int i=0; i<transProb[hist].size() && found; i++) { 
	   if(transProb[hist][i]>=p) {
         ret = transMap[hist][i];
         if(Debug>=3)
	         fprintf(stderr, "\tNext Instr: %s\n", ret->getName());
         break;
      }	
   } 

   // if there's no ret --> there is a problem a problem
   if(Debug>=2 && !ret)
	   fprintf(stderr, "Markov Model: a next instruction could not be found in transition table\n");

   //now update history based on ret
   if(ret) { 
      found = false; 
      // look for a recod that matches the new next sequence 
      map<InstructionInfo **, vector<double> >::iterator iter; 
      for(iter=transProb.begin(); iter!=transProb.end(); iter++) { 
         hist = iter->first;
		   found = true; 
         for(int i=0; i<(order-1); i++)
             found = found && (hist[i] == history[i+1]); 
         found = found && (hist[order-1] == ret); 
         if(found) { 
            history = hist; 
		      break;
         }
		   found = false;       
      }
      if(!found && Debug >=2) {
         fprintf(stderr, "Markov Model: next sequence not found in transition table:\n");
         for(int i=1; i<order; i++)
             fprintf(stderr, "\t%s\n",  history[i]->getName()); 
         fprintf(stderr, "\t%s\n",  ret->getName()); 
      }
   }     

   if(Debug>=2)
	   fprintf(stderr, "Markov Model: next instruction is (%s)\n", (ret)?ret->getName():0);
   return ret; 
}

 
/// @brief: print
///
void MarkovModel::printSequence(InstructionInfo **seq)
{
   for(int i=0; i<order; i++)
	    fprintf(stderr, "sequence[%d]=%s\n", i, seq[i]->getName()); 
}


/// @brief: Destructor
///
MarkovModel::~MarkovModel()
{
   // de-allocate memory used for transition prob table
	map<InstructionInfo **, double>::iterator iter; 
   for(iter=seqProb.begin(); iter!=seqProb.end(); iter++) 
      delete [] iter->first; 
}


InstructionInfo * MarkovModel::getInstruction(char *mnemonic, unsigned int iOpSize, char *op1, char *op2, char *op3) { 

   InstructionInfo *it; 
	// now fix up operands ( NOTE: this MUST be the same way done when reading the newImixFile
   if (op1[0] =='\0') // no-operands
      strcpy(op1,"none");
   if (strstr(op1,"ptr")) // Fore jumps/calls
     strcpy(op1,"disp");
   if (strstr(op2,"ptr")) // FOR jumps/calls
     strcpy(op2,"disp");
   // a quick fix those instrs appearing with only 'imm'
   if (op2[0]=='\0' && strstr(op1,"imm") && !strstr(mnemonic, "PUSH")) { 
      strcpy(op1,"reg");
      strcpy(op2,"imm");
   }
   // now fix up mnemonic
   // convert all conditional jumps into a generic JCC
   if (mnemonic[0] == 'J' && strcmp(mnemonic,"JMP") && !strstr(mnemonic,"CXZ"))
      strcpy(mnemonic,"JCC");
   // convert all conditional cmov's into a generic CMOVCC
   else if (strstr(mnemonic,"CMOV") == mnemonic ) 
      strcpy(mnemonic,"CMOVCC");
   // convert all conditional set's into a generic SETCC
   else if (strstr(mnemonic,"SET") == mnemonic)
      strcpy(mnemonic,"SETCC");
   // convert all conditional loop's into a generic LOOPCC
   else if (strstr(mnemonic,"LOOP") == mnemonic)
      strcpy(mnemonic,"LOOPCC");
   // remove any '_NEAR' or '_XMM' extensions
   char *p;
   if ((p=strstr(mnemonic,"_NEAR")))
      *p = '\0';
   if ((p=strstr(mnemonic,"_XMM")))
      *p = '\0';
				 		
   //fprintf(stderr, "\nNow Looking for (%s) (%s) (%s) (%s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);
   // have all data, add it to a record
   // Waleed: to do this, find the exact instruction using the mnemonic, iOpsize, and operands!
   it = lookupList->findInstructionRecord(mnemonic, iOpSize, op1, op2, op3);
   //fprintf(stderr, "Done Looking\n");
   if (!it) {
      fprintf(stderr, "ERROR: instruction record for (%s %s %s %s ,%u) not found!\n",
            mnemonic, op1, op2, op3, iOpSize);
   }
   return it; 
}
}//end namespace McOpteron
