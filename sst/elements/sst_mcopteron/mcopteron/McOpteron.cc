// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <iostream>
using std::cout;
using std::endl;
using std::cerr;
//#define EXTERN 	//Scoggin: Removed due to internalization of TraceTokens
#include "McOpteron.h" 
//#undef EXTERN		//Scoggin: Removed due to internalization of TraceTokens
//#include <MarkovModel.h>
//#include <FunctionalUnit.h>
//#include <InstructionQueue.h>
//#include <InstructionInfo.h>
//#include <LoadStoreUnit.h>
//#include <MemoryModel.h>
//#include <ReorderBuffer.h>
//#include <ConfigVars.h>

//bool McOpteron::treatImmAsNone = false; //Scoggin: Relocated to inside the constructor
namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor
///
McOpteron::McOpteron()
{
   totalInstructions = 0;
   currentCycle = 0;
   functionalUnitsHead = 0;
   instructionQueuesHead = 0;
   instructionClassProbabilities = 0;
   instructionClassesHead = instructionClassesTail = 0;
   numInstructionClasses = 0;
   infoLEA = 0;
   instructionClasses = 0;
   //dependenciesHead = dependenciesTail = 0;
   loadStoreUnit = 0;
   memoryModel = 0;
   lastToken = 0;
   lastCPI = measuredCPI = 0.0;
   probBranchMispredict = 0.0;
   probTakenBranch = 0.0;
   branchMissPenalty = 0; 
   traceF = 0;
   fakeAddress = 0x10000;
   fetchStallCycles = 0;
   config = 0;
   repeatTrace = false;
	fetchSizeProbabilities = instructionSizeProbabilities = 0; 
   markovModel = 0;

   maxInstrSize=-1;
   nextAvailableFetch=0;
   treatImmAsNone = false;	//Scoggin: Moved this here from global space
   local_debug=0;		//Scoggin: Added to pass Debug Level around in library form
   fetchBuffSize=-1;
   fetchBufferIndex=-1;
   decodeWidth=-1;
   reorderBuffer=0;
   fakeIBuffer=0;
   instructionsPerCycle=0;
   instructionsPerFetch=0;
   fetchedBuffer=0;
   decodeBuffer=0;
}


/// @brief Destructor
///
McOpteron::~McOpteron()
{
   fprintf(stdout, "CPU: stalls due to fetching: %llu\n", fetchStallCycles);
   delete loadStoreUnit;
   delete memoryModel;
   delete reorderBuffer;
   delete fakeIBuffer;
   delete config;
   if(markovModel)
      delete markovModel;
}

#if 0
/// @brief Check for operand dependencies between instructions
///
/// This walks the current dependency list and checks for a record
/// for the given instruction, and returns it if found. It also
/// prunes any records that have already been consumed.
///
Dependency* McOpteron::checkForDependencies(InstructionCount insn)
{
   Dependency *d, *n, *prev = 0;
   unsigned int searchCount = 0;
   d = dependenciesHead; 
   if (Debug>=4) fprintf(stderr,"CheckForDeps: insn=%llu  firstdep=%llu\n",
                         insn, d? d->consumer : 0);
   while (d) {
      if (d->consumer == insn) {
         if (Debug>=4) fprintf(stderr,"CheckForDeps: found after %u recs\n", searchCount);
         return d;
      }
      // prune as we go
      if (d->consumed && d->numProducers == d->numReady) {
         n = d->next;
         if (prev)
            prev->next = d->next;
         else 
            dependenciesHead = d->next;
         if (dependenciesTail == d) {
            if (d->next)
               dependenciesTail = d->next;
            else if (prev)
               dependenciesTail = prev;
            else
               dependenciesTail = dependenciesHead;
         }
         memset(d,0,sizeof(Dependency));
         delete d;
      } else {
        n = d->next;
        prev = d;
      }
      searchCount++;
      d = n;
   }
   if (Debug>=4) fprintf(stderr,"CheckForDeps: not found after %u recs\n", searchCount);
   return 0;
}
#endif

#if 0 
/// @brief Add a new operand dependency to the dep list
///
Dependency* McOpteron::addNewDependency(Token *t, unsigned int *distances)
{
	InstructionCount insn = t->instructionNumber();
   Dependency *d = NULL;
   unsigned int sourceOps = t->getType()->getSourceOps();
   // if insn has source registers
   if(sourceOps>0) { 	
      d = new Dependency();
      d->tkn = t;
		d->numInputs = sourceOps;
      d->numReady = 0;
      d->producers = new InstructionCount[sourceOps];
      for(unsigned int i=0; i<sourceOps; i++) { // get distance for each op
         unsigned int distance;
         if(distances)
            distance = distances[i];
         else
            distance = t->getType()->getUseDistance(genRandomProbability(), i); 
         if(distance>insn) {
            d->producers[i] = 0;
            d->numReady++;
         }
         else
             d->producers[i]= insn-distance; 
      }
      d->next = 0;
      if (dependenciesHead) {
         dependenciesTail->next = d;
         dependenciesTail = d;
      } else {
         dependenciesHead = dependenciesTail = d;
      }
   } 
   return d;
}
#endif


/// @brief Create instruction mix probability CDF
///
/// This assumes the main linked list of instruction types
/// has been created, and it works off that list.
/// - TODO: sort the instructions so that more probable 
///   entries are at the beginning of the array
///
void McOpteron::createInstructionMixCDF()
{
   InstructionInfo *ii = instructionClassesHead;
   numInstructionClasses = 0;
   // first count the number of instruction types
   while (ii) {
      numInstructionClasses++;
      ii = ii->getNext();
   }
   if (numInstructionClasses == 0)
      return;
   if (instructionClassProbabilities)
      delete[] instructionClassProbabilities;
   if (instructionClasses)
      delete[] instructionClasses;
   // now create CDF and info ptr arrays
   instructionClassProbabilities = new double[numInstructionClasses];
   instructionClasses = new InstructionInfo*[numInstructionClasses];
   unsigned int i = 0;
   double base = 0.0;
   // now make the CDF array and assign ptr array at same time
   ii = instructionClassesHead;
   while (ii) {
      base += ii->getOccurProb();
      instructionClassProbabilities[i] = base;
      instructionClasses[i++] = ii;
      ii = ii->getNext();
   }
   // check to make sure probabilities added up right
   if (base < 0.99999 || base > 1.00001) {
      fprintf(stderr, "CDF error: instruction mix probabilities do not add up to 1! (%g)\n", base);
      //exit(1);
   }
   // force last probability to be above 1 (rather than 0.99999)
   instructionClassProbabilities[i-1] = 1.00001;
   return;
}

/// @brief Create instruction size probability CDF
///  Format expected is as follows
///  Size_in_bytes [tab] Frequency
///
void McOpteron::createInstrSizeCDF(string filename)
{
   FILE *inf;
	unsigned long long bytes, freq; 
	unsigned long long max=0;  
	double rTotal=0, total=0; 
   //if (Debug>=2) fprintf(stderr, "\nCreating Instr Size CDF....\n");
   if (Debug>=2) cerr<<endl<<"Now Reading instruction size file ("<<filename<<")"<<endl<<endl;
	inf = fopen(filename.c_str(), "r");
   if (!inf) { 
      cerr<<"Error opening instruction size file ("<<filename<<")...quiting..."<<endl; 
      //fprintf(stderr, "Error: can not open/find instruction size distribution file! aborting...\n");
      exit(-1);
   }	
   while(fscanf(inf,"%llu\t%llu", &bytes, &freq) == 2) {
      if(bytes>max) max = bytes; 
      total += (double) freq; 
   }
   fclose(inf);
   // sanity check
   if(total <= 0.0) { 
      fprintf(stderr, "Error reading instruction size distribution file! aborting...\n");
      exit(1);
   }
   // open the file again to read in values
   instructionSizeProbabilities = new double[max+1]; // include a spot for 0
   for(unsigned long long i=0; i<=max; i++) { 
      instructionSizeProbabilities[i] = 0;
   }
   inf = fopen(filename.c_str(), "r");
   while(fscanf(inf,"%llu\t%llu", &bytes, &freq) == 2) 
      instructionSizeProbabilities[bytes] = (double)freq; 
   // now create the CDF
   for(unsigned long long i=0; i<=max; i++) { 
      rTotal += (instructionSizeProbabilities[i]>0)? instructionSizeProbabilities[i] : 0; 
      instructionSizeProbabilities[i] = rTotal/total;
   }
	maxInstrSize = (int) max; 	
	fclose(inf);  	
   return;
}

/// @brief Sample Instruction Size Distribution and return # of bytes
///  
///
int McOpteron::generateInstrSize()
{
   double p;
   int size = 0;

   // sample instruction size distribution
   p = genRandomProbability();
	
   for(size=0; size<=maxInstrSize; size++ ) { 
      if(instructionSizeProbabilities[size]>=p)
         break;
   }
   // sanity check
   if(size == 0) { 
      fprintf(stderr, "Error generating instruction size...aborting...\n");
      cerr<<" Random p was "<<p<<", maxInstrSize is "<<maxInstrSize<<", iSP[0] is " <<
                  instructionSizeProbabilities[0]<< endl;  
      exit(1);
   }
	return size; 
}	 	 

/// @brief Create fetch size probability CDF
///  Format expected is as follows
///  Size_in_instructions [tab] Frequency
///
void McOpteron::createFetchSizeCDF(string filename)
{
	
	FILE *inf;
	unsigned long long size, freq;
	unsigned entries = 0;  
	double rTotal=0, total=0; 
   if (Debug>=2) cerr<<endl<<"Now Reading Fetch Size file ("<<filename<<")"<<endl<<endl;
	inf = fopen(filename.c_str(), "r");
   if (!inf) { 
     cerr<<"Error opening fetch size file ("<<filename<<")...quiting..."<<endl; 
      exit(-1);
   }	
   // compute total
   while(fscanf(inf,"%llu\t%llu", &size, &freq) == 2) {
      total += (double) freq; 
      if(size>entries)
		   entries=size;
   }	
   fclose(inf);
   // sanity check
   if(total <= 0.0) { 
      fprintf(stderr, "Error reading fetch size distribution file! aborting...\n");
      exit(1);
   }
   fetchSizeProbabilities = new double[entries+1]; 
   // initialize with zeros
   for(unsigned i=0; i<=entries; i++) 
      fetchSizeProbabilities[i] = 0.0; 

   // open the file again to read in values
   inf = fopen(filename.c_str(), "r");
   while(fscanf(inf,"%llu\t%llu", &size, &freq) == 2) 
      fetchSizeProbabilities[size] = (double)freq; 

   // now create the CDF
   for(unsigned long long i=0; i<=entries; i++) { 
      rTotal += fetchSizeProbabilities[i]; 
      fetchSizeProbabilities[i] = rTotal/total;
   }
	fclose(inf);
   return;
}

/// @brief Sample Fetch Size Distribution and return # of instructions to fetch
///  
///
int McOpteron::generateFetchSize()
{
   double p;
   int size = 0;

   p = genRandomProbability();
	
   for(size=0; ; size++ ) { 
      if(fetchSizeProbabilities[size]>=p)
         break;
   }
   // sanity check
   if(size == 0) { 
      fprintf(stderr, "Error generating fetch size...aborting...\n");
      exit(1);
   }
	return size; 
}	 	 

int McOpteron::readConfigFile(string filename)
{
   if (!config)
      config = new ConfigVars();
   if (config->readConfigFile(filename.c_str())) {
      cerr<<"Error: failed to process config file ("<<filename<<")...Quitting"<<endl;
      //fprintf(stderr, "Error: failed to process config file (%s)...Quitting\n", filename);
      exit(1);
   }
   return 0;
}
   

/// @brief Initialize model 
///
//int McOpteron::init(const char *appDirectory, const char* definitionFilename,
//                    const char* mixFilename, const char *traceFilename,
//                    bool repeatTrace, const char* newIMixFilename, const char* InstrSizeFilename,
//						  const char* FetchSizeFilename, const char* TransProbFilename)
int McOpteron::init(string appDirectory, string definitionFilename,
                    string mixFilename, string traceFilename,
                    bool repeatTrace, string newIMixFilename, string InstrSizeFilename,
						  string FetchSizeFilename, string TransProbFilename)
{
   FunctionalUnit *fu;
   InstructionQueue *iq;
   //char *fname;
   //fname = (char*) malloc(strlen(appDirectory)+32);
   string fname;
   Debug=local_debug;		//Scoggin: Added to pass Debug around in library form
   if (Debug>=0) {
      cerr<<endl<<"Initializing McOpteron model...."<<endl;//fprintf(stderr, "\nInitializing McOpteron model....\n");
      if(definitionFilename.size()) 
        cerr<<"Instruction Definition File: "<<definitionFilename<<endl;    
      if(mixFilename.size()) 
        cerr<<"Instruction Dependence File: "<<mixFilename<<endl;     
      if(traceFilename.size()) 
        cerr<<"Instruction Trace File: "<<traceFilename<<endl;     
      if(newIMixFilename.size()) 
        cerr<<"Instruction Mix   File: "<<newIMixFilename<<endl;     
      if(InstrSizeFilename.size()) 
        cerr<<"Instruction Size  File: "<<InstrSizeFilename<<endl;    
      if(FetchSizeFilename.size()) 
        cerr<<"Fetch Block Size  File: "<<FetchSizeFilename<<endl;     
      if(TransProbFilename.size()) 
        cerr<<"Markov Model based on Instr Transition Prob File: "<<TransProbFilename<<endl;    
      cerr<<"Sample Random Number: "<<genRandomProbability()<<endl;
      cerr<<"Size(int): "<< sizeof(int) <<endl;
      cerr<<"Size(long):"<< sizeof(long) <<endl;
      cerr<<"Size(*int):"<< sizeof(int*) <<endl;
      cerr<<"Size(double):"<< sizeof(double) <<endl;
   }   
   readConfigFile("cpuconfig.ini");
   //strcpy(fname, appDirectory);
   //strcat(fname, "/");
   //strcat(fname, "appconfig.ini");
   fname=appDirectory+"/appconfig.ini";
   readConfigFile(fname);
   
   // Create and init memory model and 32-slot load-store unit and 72-slot reorder buffer
   
   // Load-store unit on Opteron is 12-slot LS1 plus 32-slot LS2; it sounds like insns in
   // LS1 immediately move to LS2 after probing the cache, so they wait in LS1 only until
   // their address is ready; our LSQ thus is modeled with 32 slots since that seems to
   // be where instructions sit the longest, though LS1 could be a bottleneck for some apps.
   // - we might be able to limit insns that don't have their address yet, to model LS1
   
   if (config->useDomain("Memory")) {
      fprintf(stderr, "Error: no configuration for Memory...quitting\n");
      exit(1);
   } else {
      int l1latency, l2latency, l3latency, memlatency, tlb1latency, tlb2latency;
      const char *v;

      READIntConfigVar("L1Latency", l1latency);
      READIntConfigVar("L2Latency", l2latency);
      READIntConfigVar("L3Latency", l3latency);
      READIntConfigVar("MemoryLatency", memlatency);
      READIntConfigVar("TLB1MissLatency", tlb1latency);
      READIntConfigVar("TLB2MissLatency", tlb2latency);
      if (Debug>0) fprintf(stderr, "MEM: Latencies: %d %d %d %d %d %d\n", 
         l1latency, l2latency, l3latency, memlatency, tlb1latency, tlb2latency);
      memoryModel = new MemoryModel();
      //
      // http://www.anandtech.com/IT/showdoc.aspx?i=3162&p=4
      // said L2 is 12 cycles and L3 is 44-48 cycles; 
      // memory is ~60ns? (120 cycles @ 2GHz)
      // - L1 is 3 cycles including address generation, so really 2
      // 
      memoryModel->initLatencies(tlb1latency, tlb2latency, l1latency, l2latency,
                                 l3latency, memlatency);
   }
   if (config->useDomain("Branch")) {
      fprintf(stderr, "Error: no configuration for Branch Predictor...quitting\n");
      exit(1);
   } else {
      const char *v;
      int brmp;
      READRealConfigVar("BrMissPenalty", brmp);
      branchMissPenalty = brmp;
   }

   if (config->useDomain("Application")) {
      fprintf(stderr, "Error: no configuration for Memory...quitting\n");
      exit(1);
   } else {
      double stfwd, dl1, dl2, dl3, dtlb1, dtlb2, ic, itlb1, itlb2;
      double bmp, tbr;
      const char *v;
      //int instperfetch;
      READRealOptionalVar("MeasuredCPI", measuredCPI);
      //Waleed: moved reading the InstructionsPerFetch to appconfig.ini
      //READIntConfigVar("InstructionsPerFetch", instperfetch);
      instructionsPerFetch = 3; //instperfetch;
      READRealConfigVar("DStoreForwardRate", stfwd);
      READRealConfigVar("DL1MissRate", dl1);
      READRealConfigVar("DTLB1MissRate", dtlb1);
      READRealConfigVar("DTLB2MissRate", dtlb2);
      READRealConfigVar("ICacheMissRate", ic);
      READRealConfigVar("ITLB1MissRate", itlb1);
      READRealConfigVar("ITLB2MissRate", itlb2);
      READRealConfigVar("L2MissRate", dl2);
      READRealConfigVar("L3MissRate", dl3);
      memoryModel->initProbabilities(stfwd, dl1, dtlb1, dtlb2,
                                     ic, itlb1, itlb2, dl2, dl3);
      READRealConfigVar("BranchMispredictRate", bmp);
      probBranchMispredict = bmp;
      // Waleed: add following
      READRealConfigVar("TakenBranchRate", tbr);
      probTakenBranch = tbr;
   }

   // The Opteron has a two-level load/store unit, but insns quickly move from
   // the 12-entry LS1 into the 32-entry LS2; essentially it sounds like at 
   // the LS1 they wait for addresses to be generated, at in LS2 wait for the
   // memop to complete. We could consider limiting the number of instructions
   // waiting for an address to be 12 while keeping the size of the LSQ at 32
   if (config->useDomain("FetchDecode")) {
      fprintf(stderr, "Error: no configuration for FetchDecode...quitting\n");
      exit(1);
   } else {
      int instpercycle;      
      const char *v;
      READIntConfigVar("InstructionsPerCycle", instpercycle);
      instructionsPerCycle = instpercycle;
      // Waleed: read new variables
      READIntConfigVar("FetchBufferSize", fetchBuffSize);
      READIntConfigVar("MaxInstructionsDecoded", decodeWidth);

   }

   // The Opteron has a two-level load/store unit, but insns quickly move from
   // the 12-entry LS1 into the 32-entry LS2; essentially it sounds like at 
   // the LS1 they wait for addresses to be generated, at in LS2 wait for the
   // memop to complete. We could consider limiting the number of instructions
   // waiting for an address to be 12 while keeping the size of the LSQ at 32
   if (config->useDomain("LoadStoreQueue")) {
      fprintf(stderr, "Error: no configuration for LoadStoreQueue...quitting\n");
      exit(1);
   } else {
      int numslots, memops;
      const char *v;
      
      READIntConfigVar("NumSlots", numslots);
      READIntConfigVar("MemOpsPerCycle", memops);
      loadStoreUnit = new LoadStoreUnit(numslots, memops, memoryModel);
   }
   // Reorder buffer as 24 lanes with 3 entries each; we are not exactly modeling
   // 3-at-a-time retirement, but just force retirement to be in order and capping
   // the max # per cycle at 3. Hopefully close enough.
   if (config->useDomain("ReorderBuffer")) {
      fprintf(stderr, "Error: no configuration for ReorderBuffer...quitting\n");
      exit(1);
   } else {
      int numslots, numretire;
      const char *v;
      
      READIntConfigVar("NumSlots", numslots);
      READIntConfigVar("RetirePerCycle", numretire);
      reorderBuffer = new ReorderBuffer(numslots, numretire);
      fakeIBuffer = new ReorderBuffer(100, 1); // for retiring fake LEAs
      // Waleed: sanity check, numretire must be equal to decode width
      if(numretire != decodeWidth ) { 
         fprintf(stderr, "Error: Decode width must be equal to maximum retire/commit width...quitting\n");
         exit(1);
      }
   }

   // Create physical model from configuration file
   // Official Opteron: three integer queues (regular, multiply,
   // and special) each with an ALU and AGU, then the floating point queue
   // with FADD, FMUL, and FSTORE functional units
   if (config->useDomain("Architecture")) {
      fprintf(stderr, "Error: no configuration for Architecture...quitting\n");
      exit(1);
   } else {
      int numintqs, numfloatqs;
      char **qnames, **qunits;
      int *qsizes, *qrates; int i;
      const char *v; char *tptr;
      char varname[24];
      //FunctionalUnit *fuTail = 0;
      InstructionQueue * iqTail = 0;
      // get number of int and float queues
      READIntConfigVar("NumIntegerQueues", numintqs);
      READIntConfigVar("NumFloatQueues", numfloatqs);
      // allocate config var arrays
      qnames = new char*[numintqs+numfloatqs];
      qunits = new char*[numintqs+numfloatqs];
      qsizes = new int[numintqs+numfloatqs];
      qrates = new int[numintqs+numfloatqs];
      // read in int queue configs
      for (i = 0; i < numintqs; i++) {
         sprintf(varname,"IntQueue%dName",i+1);
         READStringConfigVar(varname, qnames[i]);
         sprintf(varname,"IntQueue%dUnits",i+1);
         READStringConfigVar(varname, qunits[i]);
         sprintf(varname,"IntQueue%dSize",i+1);
         READIntConfigVar(varname, qsizes[i]);
         sprintf(varname,"IntQueue%dRate",i+1);
         READIntConfigVar(varname, qrates[i]);
      }
      // read in float queue configs
      for (; i < numintqs+numfloatqs; i++) {
         sprintf(varname,"FloatQueue%dName",i-numintqs+1);
         READStringConfigVar(varname, qnames[i]);
         sprintf(varname,"FloatQueue%dUnits",i-numintqs+1);
         READStringConfigVar(varname, qunits[i]);
         sprintf(varname,"FloatQueue%dSize",i-numintqs+1);
         READIntConfigVar(varname, qsizes[i]);
         sprintf(varname,"FloatQueue%dRate",i-numintqs+1);
         READIntConfigVar(varname, qrates[i]);
      }
      // now create the int queues and functional units
      for (i=0; i < numintqs; i++) {
         InstructionQueue::QType qtype = InstructionQueue::INT;
         if (strstr(qunits[i],"ALUSP"))
            qtype = InstructionQueue::INTSP;
         else if (strstr(qunits[i],"ALUMULT"))
            qtype = InstructionQueue::INTMUL;
         iq = new InstructionQueue(qtype, qnames[i], i+1, qsizes[i], qrates[i]);
         if (iqTail) {
            iqTail->setNext(iq);
            iqTail = iq;
         } else {
            instructionQueuesHead = iqTail = iq;
         }
         if (Debug>0) fprintf(stderr, "  Creating queue %d(%s)\n",i+1,qnames[i]);
         v = strtok_r(qunits[i],",",&tptr);
         while (v) {
            fu = 0;
            if (!strcmp(v,"AGU")) 
               fu = new FunctionalUnit(AGU, "Regular AGU", i+1);
            else if (!strcmp(v,"ALU")) 
               fu = new FunctionalUnit(ALU1, "Regular ALU", i+1);
            else if (!strcmp(v,"ALUSP")) 
               fu = new FunctionalUnit(ALU2, "Special ALU", i+1);
            else if (!strcmp(v,"ALUMULT")) 
               fu = new FunctionalUnit(ALU0, "Multiply ALU", i+1);
            if (!fu) {
               fprintf(stderr,"Error: unknown functional unit (%s).quitting\n",
                       v);
               exit(1);
            }
            if (Debug>0) fprintf(stderr, "  Added func unit (%s)\n", v);
            fu->setNext(functionalUnitsHead);
            functionalUnitsHead = fu;
            iq->addFunctionalUnit(fu);
            v = strtok_r(0,",",&tptr);
         }
      }
      // now create the float queues and functional units
      for (; i < numintqs+numfloatqs; i++) {
         InstructionQueue::QType qtype = InstructionQueue::FLOAT;
         iq = new InstructionQueue(qtype, qnames[i], i+1, qsizes[i], qrates[i]);
         if (iqTail) {
            iqTail->setNext(iq);
            iqTail = iq;
         } else {
            instructionQueuesHead = iqTail = iq;
         }
         if (Debug>0) fprintf(stderr, "  Creating queue %d(%s)\n",i+1,qnames[i]);
         v = strtok_r(qunits[i],",",&tptr);
         while (v) {
            fu = 0;
            if (!strcmp(v,"FADD")) 
               fu = new FunctionalUnit(FADD, "Float Adder", i+1);
            else if (!strcmp(v,"FMUL")) 
               fu = new FunctionalUnit(FMUL, "Float Multiplier", i+1);
            else if (!strcmp(v,"FSTORE")) 
               fu = new FunctionalUnit(FSTORE, "Float Store", i+1);
            if (!fu) {
               fprintf(stderr,"Error: unknown functional unit (%s).quitting\n",
                       v);
               exit(1);
            }
            if (Debug>0) fprintf(stderr, "  Added func unit (%s)\n", v);
            fu->setNext(functionalUnitsHead);
            functionalUnitsHead = fu;
            iq->addFunctionalUnit(fu);
            v = strtok_r(0,",",&tptr);
         }
      }
   }

   // Waleed:
   // initialize the fetchedBuffer
   fetchedBuffer = new Token*[fetchBuffSize]; 
   for(int i=0; i<fetchBuffSize; i++) 
	   fetchedBuffer[i] = NULL;   

   // Waleed:
   // initialize the decodeBuffer
   decodeBuffer = new Token*[decodeWidth]; 
   for(int i=0; i<decodeWidth; i++) 
	   decodeBuffer[i] = NULL;   

   // Waleed:
   fetchBufferIndex=0;
   usedFetchBuffBytes=0;	
	
	// Now make the instruction info 

   // read static instruction definition information
   if (Debug>0) cerr<<"Instruction Definition File: "<<definitionFilename<<endl;
   readIDefFile(definitionFilename, (newIMixFilename.size())?true:false);   

   //
   // If given a trace file, open it
   //
   if (traceFilename.size()) {
      traceF = fopen(traceFilename.c_str(), "r");
      if (!traceF) {
         cerr<<"Error: cannot open trace file ("<<traceFilename<<"), exiting."<<endl;
         //fprintf(stderr, "Error: cannot open trace file (%s), exiting.\n", traceFilename);
         exit(0);
      }
      this->repeatTrace = repeatTrace;

		// create instruction size probabilities used in fetch
      if(InstrSizeFilename.size())
		   createInstrSizeCDF(InstrSizeFilename); 

		// create fetch size probabilities used in fetch
      if(FetchSizeFilename.size())
		   createFetchSizeCDF(FetchSizeFilename); 

      // set up a direct ptr to the LEA instruction (used for 
      // FP insns with memory accesses
      infoLEA = instructionClassesHead->findInstructionRecord("LEA", 64);
      if (!infoLEA) {
         fprintf(stderr, "Error: instruction record for LEA/64 not found! Quitting\n");
         exit(0);
      }
      if (Debug>0) fprintf(stderr, "Done initializing\n");
      return 0;
   }

	
	if (newIMixFilename.size()) {
      //strcpy(fname, appDirectory);
      //strcat(fname, "/");
      //strcat(fname, newIMixFilename);
      fname=appDirectory+"/"+newIMixFilename;
      //if (Debug>0) fprintf(stderr, "New IMix Input file: %s\n", fname);
      readNewIMixFile(fname);
		// create instruction size probabilities used in fetch
      if(InstrSizeFilename.size())
		   createInstrSizeCDF(InstrSizeFilename); 
		// create fetch size probabilities used in fetch
      if(FetchSizeFilename.size())
		   createFetchSizeCDF(FetchSizeFilename); 
   }
   
   // set up Markov-based token generator
   
   if(TransProbFilename.size()){
      if(Debug>0) cerr<<"transFile found, creating MarkovModel"<<endl; 
      markovModel = new MarkovModel(3, instructionClassesHead, TransProbFilename.c_str());
   }

   // read application specific instruction mix, etc. information
   // if we read the new imix file, only use the use distance info
   //strcpy(fname, appDirectory);
   //strcat(fname, "/");
   //strcat(fname, mixFilename);
   fname=appDirectory+"/"+mixFilename;
   //if (Debug>0) fprintf(stderr, "IMix Input file: %s\n", fname);
   readIMixFile(fname, newIMixFilename);
   
   // use instruction type list to make a CDF
   createInstructionMixCDF();
   
   // set up a direct ptr to the LEA instruction (used for 
   // FP insns with memory accesses
   infoLEA = instructionClassesHead->findInstructionRecord("LEA", 64);
   if (!infoLEA) {
      fprintf(stderr, "Error: instruction record for LEA/64 not found! Quitting\n");
      exit(0);
   }


   if (Debug>0) fprintf(stderr, "Done initializing\n");
   
   cerr<<endl;
   cout<<endl;
   return 0;
}


/// @brief Read the app statistics input file
///
/// This expects an input file with lines like this:<br>
///   Instruction: MNEMONIC SIZE OPERANDS<br>
///   Occurs: #   Loads: #   Stores: #<br>
///   Use distances<br>
///   [multiple 2-number lines, each with a distance and count]<br>
///   Total uses: 0<br>
///   ===<br>
///   [repeat for each instruction]<br>
///
int McOpteron::readIMixFile(string filename, string newIMixFilename)
{
   char line[128];
   char mnemonic[24], iClass[24], op1[24], op2[24], op3[24];
   unsigned int iOpSize, useDist, i;
   unsigned long long occurs, loads, stores, totalUses;
   unsigned int sourceOps, curSource; 
   unsigned long long numUses, totalInstructions;
   double iProbability;
	double ** depHistograms = 0; 
   FILE *inf;
   InstructionInfo *it;

   if (Debug>=2) cerr<<endl<<"Now Reading usedist file ("<<filename<<")"<<endl<<endl;
      //fprintf(stderr, "\nNow Reading usedist file (%s)\n\n", filename);

   inf = fopen(filename.c_str(), "r");
   if (!inf) {
      cerr<<"Error opening usedist file ("<<filename<<")...quiting.."<<endl;
      //fprintf(stderr, "Error opening usedist file (%s)...quiting...\n", filename);
      exit(-1);
   }
   if (!fgets(line, sizeof(line), inf)) {
      fprintf(stderr, "First line not read! (%s), aborting...\n", line);
      fclose(inf);
      return -1;
   }
   if (sscanf(line, "Total instruction count: %llu", &totalInstructions) != 1) {
      fprintf(stderr, "First line not right! (%s), aborting...\n", line);
      fclose(inf);
      return -2;
   }

   while (!feof(inf)) {
      if (!fgets(line, sizeof(line), inf))
         break;
      iOpSize = 64;
      sourceOps = 0; 
      iClass[0] = op1[0] = op2[0] = op3[0] = '\0';
      //if (sscanf(line,"Instruction: %lg %s %u %s", &iProbability, mnemonic, 
      //           &iOpSize, iClass) != 4) {
      //   fprintf(stderr, "Unknown line (%s), skipping\n", line);
      //   continue;
      //}
      if (sscanf(line, "Instruction: %lg %s %u %[^,]%*c %[^,]%*c %s",&iProbability,mnemonic,&iOpSize,op1,op2,op3) != 6) { 	
         if (sscanf(line, "Instruction: %lg %s %u %[^,]%*c %s",&iProbability,mnemonic,&iOpSize,op1,op2) != 5) {
            if (sscanf(line, "Instruction: %lg %s %u %s",&iProbability,mnemonic,&iOpSize,op1) != 4) {          
               if (sscanf(line, "Instruction: %lg %s %u",&iProbability, mnemonic, &iOpSize) != 3) {
                 fprintf(stderr,"Unknown line (%s), skipping\n",line);
                 continue;
               }
            }
         }
      }
		//fprintf(stderr, "\nInstruction (%s) (%s) (%s) (%s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);
      if (Debug>=3)
         fprintf(stderr, "\nInstruction (%s %s %s %s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);
      if (!fgets(line, sizeof(line), inf))
         break;
      if (sscanf(line,"Occurs: %llu Loads: %llu Stores: %llu InputRegs: %u", 
                 &occurs, &loads, &stores, &sourceOps) != 4)
         break;
      if (Debug>=3)
         fprintf(stderr, " occur %llu  loads %llu  stores %llu InputRegs %u\n",
                 occurs, loads, stores, sourceOps);
      if(sourceOps == 0) { 
         // skip next three lines
         if (!fgets(line, sizeof(line), inf))
            break;
         if (!fgets(line, sizeof(line), inf))
            break;
         if (!fgets(line, sizeof(line), inf))
            break;
		} 
		else { 
      if (!fgets(line, sizeof(line), inf))
         break;
      if (!strstr(line,"Use distances"))
         break;
		// now read the next line
      if (!fgets(line, sizeof(line), inf))
         break;
      if (sscanf(line,"Input register: %u", &curSource) != 1)
         break;	
      //fprintf(stderr, "\nReady to parse input regs = %u\n", curSource); 
		// allocate memory for historams (one for each input register)
		depHistograms = new double *[sourceOps] ;
      for( unsigned int h = 0 ; h < sourceOps ; h++ ) 
         depHistograms[h] = new double[HISTOGRAMSIZE];

		// now read histogram for each input register
		while(sourceOps !=0 && curSource>=0 && curSource<sourceOps) { 		
      i = 0; totalUses = 0;
      unsigned int nextSource, flag=0;
      // Now read in use distance histogram; it can have skipped entries
      // which are just zero entries
      while (!feof(inf)) {
         if (!fgets(line, sizeof(line), inf))
            break;
         if (sscanf(line,"Input register: %u", &nextSource) == 1) { // reached next input reg
            flag = 1;
            break;
         }
         if (strstr(line,"Total uses:")) { // reached end
            break;
         }
         if (sscanf(line, "%u %llu", &useDist, &numUses) != 2)
            break;
         if (Debug>=3)
            fprintf(stderr, " (%u) (%u) (%llu)\n", curSource, useDist, numUses);
         // assign previous total uses (for CDF) to missing entries
         while (i < useDist) 
            depHistograms[curSource][i++] = totalUses;
         // now fill in this entry (i == useDist) (for CDF, use total uses so far)
         totalUses += numUses;
         depHistograms[curSource][i++] = totalUses;
      }
      if (i==0 && totalUses==0) {
			// no use dist info, so set dist == 0
			// this should not happen but it's here just in case
         totalUses = 1;
         depHistograms[curSource][i++] = 1;
      }
		// fill in rest of entries
      while (i < HISTOGRAMSIZE)
         depHistograms[curSource][i++] = totalUses;
      
      // now make probabilities
      for (i=0; i < HISTOGRAMSIZE; i++)
         depHistograms[curSource][i] = depHistograms[curSource][i] / (double) totalUses;

		// now read in histogram for next register
      if(nextSource != curSource && flag) { 
         curSource = nextSource;
         flag = 0;
         continue;   
      }
      // are we done with this instruction?
      if (strstr(line,"Total uses:")) {
         if (!fgets(line, sizeof(line), inf))
            break;
         if (strncmp(line,"===", 3)==0)
            break;
      } else { // I should not be here, but this is to prevent infinite loop
         if (!fgets(line, sizeof(line), inf))
            break;
         if (strncmp(line,"===", 3)==0)
            break;
      }		
		} // end while loop for every reg operand
		} // end else sourceOps
      // have all data, add it to a record
      if (newIMixFilename.size()) {
         //fprintf(stderr, "\nInstruction (%s) (%s) (%s) (%s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);
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
         //fprintf(stderr, "\nNow Looking for (%s) (%s) (%s) (%s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);

         //Waleed: now fix up mnemonic
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
         it = instructionClassesHead->findInstructionRecord(mnemonic, iOpSize, op1, op2, op3);
         //fprintf(stderr, "Done Looking\n");
         if (!it) {
            fprintf(stderr, "ERROR: instruction record for (%s %s %s %s ,%u) not found!\n",
                 mnemonic, op1, op2, op3, iOpSize);
            // now deallocate memory
     	      if(sourceOps > 0 && depHistograms) { 
				for( unsigned int i = 0 ; i < sourceOps ; i++ )
               delete [] depHistograms[i] ;
            delete [] depHistograms ;
				}
            continue;
         }
			it->allocateHistograms(sourceOps); 			
         it->accumUseHistogram(occurs, depHistograms, false);
      }
      else {
         fprintf(stderr, "ERROR: must use newImix!\n");
         continue;
      }
      // now deallocate memory if needed
      if(sourceOps != 0 && depHistograms) { 
		   for( unsigned int i = 0 ; i < sourceOps ; i++ )
			   delete [] depHistograms[i] ;
         delete [] depHistograms ;		
      }	
   }   
   fclose(inf);
   return 0;
}


/// @brief Read the app statistics input file
///
/// This expects an input file with lines like this:<br>
///   first line: Instr    Total    Percent<br>
///   MNEMONIC SIZE [OP[, OP[, OP]]]  Count  Percent<br>
///   [repeat for each instruction]<br>
///   last line: TOTAL  Number  100.00<br>
///   
///
int McOpteron::readNewIMixFile(string filename)
{
   char line[128];
   char mnemonic[24], op1[24], op2[24], op3[24];
   unsigned int iOpSize=0; //, i; 
   char *r; // fget result, never used but supresses warnings
   unsigned long long occurs;
   unsigned long long totalInstructions;
   double iProbability, loadProb, storeProb;
   FILE *inf;
   InstructionInfo *it;

   //if (Debug>=3)
   //   fprintf(stderr, "\nNow reading the InstrMix (IMIX) file (%s)\n", filename);
   if (Debug>=3) cerr<<endl<<"Now Reading InstrMix (IMIX) file ("<<filename<<")"<<endl<<endl;
   // first get total instructions from last line of file
   inf = fopen(filename.c_str(), "r");
   if (!inf) { 
      cerr<<"Error opening IMIX file ("<<filename<<")...quiting..."<<endl;
      //fprintf(stderr, "Error opening IMIX file (%s)...quiting...\n", filename);
      exit(-1);
   }
   while (!feof(inf))
      r = fgets(line, sizeof(line), inf);
   fclose(inf);
   if (sscanf(line, "TOTAL %llu", &totalInstructions) != 1) {
      fprintf(stderr, "Total line in new imix not right! (%s), aborting...\n", line);
      exit(0);
   }
   if (Debug>=3) fprintf(stderr,"total instructions: %llu\n", totalInstructions);

   // now get instructions
   inf = fopen(filename.c_str(), "r");
   if (!inf)
      return -1;
   //r = fgets(line, sizeof(line), inf);  // do not skip skip first line
   while (!feof(inf)) {
      r = fgets(line, sizeof(line), inf);
      op1[0] = '\0';  op2[0] = '\0'; op3[0] = '\0';
      if (sscanf(line, "%s %u %llu %lf",mnemonic, &iOpSize, &occurs,&iProbability) != 4) {
         if (sscanf(line, "%s %u %s %llu %lf",mnemonic,&iOpSize,op1,&occurs,&iProbability) != 5) {
            if (sscanf(line, "%s %u %[^,]%*c %s %llu %lf",mnemonic,&iOpSize,op1,op2,&occurs,&iProbability) != 6) {
               if (sscanf(line, "%s %u %[^,]%*c %[^,]%*c %s %llu %lf",mnemonic,&iOpSize,op1,op2,op3,&occurs,&iProbability) != 7) {
                 fprintf(stderr,"IMIX: BAD LINE: (%s)\n",line);
                 continue;
               }
            }
         }
      }
      if (!strcmp(mnemonic,"TOTAL"))
         break;
		//fprintf(stderr, "I have:\n(%s %s %s %s)\n", mnemonic, op1, op2, op3); 
      iProbability = (double)occurs/totalInstructions;
      char cop1[6],cop2[6],cop3[6]; 
      cop1[0] =  cop2[0] = cop3[0] = '\0';  
      // find operand size (order is important here because 128 has an 8 in it
      // and some instructions have both 64 and 32 bit operands (we choose 64 first)
      if(iOpSize == 0 ) { 
      // if no opsize was read?
      fprintf(stderr, "Warning: OpSize Was not read from instr Mix, default is used\n"); 
      iOpSize = 64; // default
      }
#if 0		
      if (strstr(op1,"128") || strstr(op2,"128")) {
         iOpSize = 128;
      } else if (strstr(op1,"64") || strstr(op2,"64")) {
         iOpSize = 64;
      } else if (strstr(op1,"32") || strstr(op2,"32")) {
         iOpSize = 32;
      } else if (strstr(op1,"16") || strstr(op2,"16")) {
         iOpSize = 16;
      } else if (strstr(op1,"8") || strstr(op2,"8")) 
         iOpSize = 8;
      //Waleed: added the following condition for xmm
      if (strstr(op1,"xmm") || strstr(op2,"xmm")) 
         iOpSize = 128;
      // Waleed: fix up operands 
      // convert near|far to disp and convert ptr to m
      // also, convert instructions with one 'imm' op to 2 operands 'reg', 'imm'
      // finally, convert all occurrences of Jcc/CMOVcc/SETcc to just JCC or CMOVCC or SETcc
      if (!strstr(op1,"m") && (strstr(op1,"near") || strstr(op1,"far") ) ) 
         strcpy(op1,"disp");
      if (!strstr(op2,"m") && (strstr(op2,"near") || strstr(op2,"far") ) ) 
         strcpy(op2,"disp");
      if (strstr(op1,"ptr")) // Fore LEA?
         strcpy(op1,"reg");
      if (strstr(op2,"ptr")) // FOR LEA
         strcpy(op2,"reg");
      // a quick fix those instrs appearing with only 'imm'
      if (op2[0]=='\0' && strstr(op1,"imm")) { 
         strcpy(op1,"reg");
         strcpy(op2,"imm");
      }
#endif
		if (strstr(op1,"ptr")) // Fore jumps/calls
         strcpy(op1,"disp");
      if (strstr(op2,"ptr")) // FOR jumps/calls
         strcpy(op2,"disp");
      // a quick fix those instrs appearing with only 'imm'
      if (op2[0]=='\0' && strstr(op1,"imm") && !strstr(mnemonic, "PUSH")) { 
         strcpy(op1,"reg");
         strcpy(op2,"imm");
      }
		
      //Waleed: now fix up mnemonic
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

      // Waleed: fixup operands and extract simple form of operands to be used to look up a record later
      if(op1[0]=='\0') { 
         strcpy(cop1,"none");
			//op2[0] ='\0'; 
			//op3[0] ='\0';
      }
      else 
         strcpy(cop1,op1);
      if(op2[0] !='\0') 
         strcpy(cop2,op2);
      if(op3[0] !='\0')
         strcpy(cop3,op3);

      // Waleed: determine loadProb and storeProb
      if (strstr(cop1,"mem")) {
         storeProb = 1.0;
      } 
      else {
         storeProb = 0.0;
      } 
      if (strstr(cop2,"mem")) {
         loadProb = 1.0;
      } 
      else {
         loadProb = 0.0;
      } 
      // Waleed: fix loadProb for isntructions other than MOV* where 
      // if the 1st operand is mem, it means a load and a store such as ADD mem, reg
      if (strstr(cop1,"mem") && !strstr(mnemonic, "MOV")) 
         loadProb = 1.0;
      // Waleed: now consider PUSH and POP instructions which implicitly access memory!
      if (strstr(mnemonic,"POP")) {
         loadProb = 1.0;
      } 
      else if (strstr(mnemonic,"PUSH")) {
         storeProb = 1.0;
      }
      if (Debug>=3)
         fprintf(stderr, "%s:%d %s%g,%s%g %llu  %lg\n",mnemonic,iOpSize,
                 op1,storeProb,op2,loadProb,occurs,iProbability);

      //fprintf(stderr, "I have:\n(%s %s %s %s)\n", mnemonic, cop1, cop2, cop3); 
		// have all data, add it to a record
      // Waleed: to do this, find the exact instruction using the mnemonic, iOpsize, and operands!
      it = instructionClassesHead->findInstructionRecord(mnemonic, iOpSize, cop1, cop2, cop3);
      if (!it) {
         fprintf(stderr, "IMIX: Warning, instruction record for (%s %s %s %s ,%u) not found in static table!\n",
                 mnemonic, op1, op2, op3, iOpSize);
         // quit
         exit(1);
         continue;
      }
      it->accumProbabilities(iOpSize, iProbability, occurs, 
            loadProb * occurs, storeProb * occurs, true);
   }
   fclose(inf);
   return 0;
}


/// @brief Read the instruction definition input file
///
/// This expects an input file with lines like this:<br>
///   MNEMONIC operands operation decodeunit execunits 
///            baselatency memlatency throughput category
///
//int McOpteron::readIDefFile(const char *filename, bool newImixFile)
int McOpteron::readIDefFile(string filename, bool newImixFile)
{
   char line[512];
   char mnemonic[24], operands[100], operation[24], decodeUnit[24],
        execUnits[32], category[24];
   unsigned int i, baseLatency, memLatency, throughputNum, throughputDem;
   FILE *inf = 0;
   InstructionInfo *it;

   if (Debug>=2) cerr<<endl<<"Now Reading instruction definition file ("<<filename<<")"<<endl<<endl;		//Scoggin: this debug message was missing.

   inf = fopen(filename.c_str(), "r");
   if (!inf) {
      cerr<<"Error opening defintion file ("<<filename<<")...quiting..."<<endl;
      //fprintf(stderr, "Error opening definition file (%s)...quiting...\n", filename);
      exit(-1);
   }
   
   i = 0;
   while (!feof(inf)) {
      if (!fgets(line, sizeof(line), inf))
         break;
      i++;
      // skip comment lines
      if (line[0] == '/' && (line[1] == '*' || line[1] == '/'))
         continue; 
      if (sscanf(line,"%s %s %s %s %s %u %u %u/%u %s", mnemonic, operands,
                 operation, decodeUnit, execUnits, &baseLatency, &memLatency,
                 &throughputNum, &throughputDem, category) != 10) {
         if (strlen(line) > 5)
            fprintf(stderr, "Error on line %u  (%s), skipping\n", i, line);
         continue;
      }
      if (Debug>=3)
         fprintf(stderr, "I %s %s %s %s %s %u %u %u/%u %s\n", mnemonic, operands,
                 operation, decodeUnit, execUnits, baseLatency, memLatency,
                 throughputNum, throughputDem, category);
      // have all data, now add it to a record
      if(!newImixFile) { 
         it = new InstructionInfo();
         it->initStaticInfo(mnemonic, operands, operation, decodeUnit, execUnits, category);
         it->initTimings(baseLatency, memLatency, throughputNum, throughputDem);
         if (instructionClassesHead) {
            instructionClassesTail->setNext(it);
            instructionClassesTail = it;
         } 
         else {
            instructionClassesHead = instructionClassesTail = it;
         }         
      } 
      else { 
         // Waleed: add one record for every unique operand types found in the def file
         // First, parse operands and iterate thru each combination   
         char *op_1, *op_2, *op_3, *ptr1, *ptr2, *ptr3;
         char *op1 = strtok(operands, ",");
         char *op2 = strtok(0, ",");
         char *op3 = strtok(0, ",");
         char new_operands[25];           
         for(ptr1= NULL, op_1 = strtok_r(op1, "/()", &ptr1); op_1; op_1 = strtok_r(0, "/()", &ptr1)){ 
            if(op2) {
               char *saved_op2 = strdup(op2);
               for(ptr2 = NULL,op_2 = strtok_r(saved_op2, "/()", &ptr2); op_2 ; op_2 = strtok_r(0, "/()", &ptr2)){
                  if(op3) {
                     char *saved_op3 = strdup(op3);
                     for(ptr3 = NULL,op_3 = strtok_r(saved_op3, "/()", &ptr3); op_3; op_3 = strtok_r(0, "/()", &ptr3)){ 
                        it = new InstructionInfo();
                        sprintf(new_operands, "%s,%s,%s", op_1, op_2, op_3);
                        it->initStaticInfo(mnemonic, new_operands, operation, decodeUnit, execUnits, category);
                        it->initTimings(baseLatency, memLatency, throughputNum, throughputDem);
                        if (instructionClassesHead) {
                           instructionClassesTail->setNext(it);
                           instructionClassesTail = it;
                        } 
                        else {
                           instructionClassesHead = instructionClassesTail = it;
                        }         
                     }
                     free(saved_op3);
                  } 
                  else { // only two operands
                     it = new InstructionInfo();
                     sprintf(new_operands, "%s,%s", op_1, op_2);
                     it->initStaticInfo(mnemonic, new_operands, operation, decodeUnit, execUnits, category);
                     it->initTimings(baseLatency, memLatency, throughputNum, throughputDem);
                     if (instructionClassesHead) {
                        instructionClassesTail->setNext(it);
                        instructionClassesTail = it;
                     } 
                     else {
                        instructionClassesHead = instructionClassesTail = it;
                     }         
                  } // end else   
               }   // end for loop for two operands  
               free(saved_op2);
            }
            else { // only one operand
               it = new InstructionInfo();
               sprintf(new_operands, "%s", op_1);
               it->initStaticInfo(mnemonic, new_operands, operation, decodeUnit, execUnits, category);
               it->initTimings(baseLatency, memLatency, throughputNum, throughputDem);
               if (instructionClassesHead) {
                  instructionClassesTail->setNext(it);
                  instructionClassesTail = it;
               } 
               else {
                  instructionClassesHead = instructionClassesTail = it;
               }         
            } // end else
         } // end outer for loop
      } // end else
   } // end while
   fclose(inf);
   return 0;
}

void McOpteron::printStaticIMix()
{
   unsigned int i=1;
   InstructionInfo *it;
   it = instructionClassesHead;
   while (it) {
      fprintf(stderr, "%d: %s-%u: Trace prob: %.3g \n", i++,
                  it->getName(), it->getMaxOpSize(), it->getOccurProb());
      it = it->getNext();
   }
}

/// @brief Finalize simulation and report statistics
///
int McOpteron::finish(bool printInstMix)
{
   unsigned int i=1;
   FunctionalUnit *fu;
   InstructionQueue *iq;
   InstructionInfo *it;
   
   if (traceF)
      fclose(traceF);

   fprintf(stdout, "\n");
   fprintf(stdout, "Total cycles simulated: %llu\n", currentCycle);
   fprintf(stdout, "Total instructions generated: %llu\n", totalInstructions);
   //fprintf(stdout, "Total instructions retired  : %llu\n", retiredInstructions);
   fprintf(stdout, "Predicted CPI : %g\n", currentCPI());
   fprintf(stdout, "Measured  CPI : %g\n", measuredCPI);

   //fprintf(stdout, "Predicted CPIr: %g\n", (double) currentCycle /
   //          retiredInstructions);
   if (measuredCPI > 0.001)
      fprintf(stdout,"Fraction of measured CPI: %g\n", currentCPI()/measuredCPI);
   fprintf(stdout, "\n");
   
   // deconstruct instruction info
   while (instructionClassesHead) {
      it = instructionClassesHead;
      instructionClassesHead = it->getNext();
      if ((Debug>=3 || printInstMix) && 
          (it->getSimulationCount() > 0 || it->getOccurProb() > 1e-14))
         fprintf(stderr, "%d: %s-%u: Trace prob: %.3g \t actual prob: %.3g \t avg use-dist: %g\n",
                  i++, it->getName(), it->getMaxOpSize(), it->getOccurProb(),
                  (double) it->getSimulationCount() / totalInstructions,
                  it->getAverageDepDists());
      //else i++;
      delete it;
   }

   // deconstruct physical model
   if (Debug>0) cerr<<endl<<"Deleting stuff"<<endl;//fprintf(stderr,"\nDeleting stuff\n");
   while (instructionQueuesHead) {
      iq = instructionQueuesHead;
      instructionQueuesHead = iq->getNext();
      delete iq;
   }
   //if (Debug>0) fprintf(stderr, ".\n");
   while (functionalUnitsHead) {
      fu = functionalUnitsHead;
      functionalUnitsHead = fu->getNext();
      delete fu;
   }
   //if (Debug) fprintf(stderr, ".\n");

   delete fetchedBuffer; 

   return 0;
}


/// @brief Get current cycle count
///
CycleCount McOpteron::currentCycles()
{
   return currentCycle;
}


/// @brief Get current CPI
///
double McOpteron::currentCPI()
{
   return (double) currentCycle / totalInstructions;
}

//=========================================================
//
// Main simulation routines: everything starts at simCycle()
//
//=========================================================

/// @brief Simulate one cycle
///
/// This function works backwards up the pipeline since
/// we need to open things up to move things forward, and
/// software doesn't all happen at once.
///
int McOpteron::simCycle()
{
   Debug=local_debug;     //Scoggin: Added to pass Debug around in library form
   // print out a progress dot
   if (currentCycle % 100000 == 0) { 
      fprintf(stderr,"%ld cycles, %g cpi\n", (long) currentCycle,
              (double) currentCycle / totalInstructions); 
   }
   currentCycle++;
   if (Debug>=2) fprintf(stderr, "\n\n======= Simulating cycle %llu ====== \n\n",
                        currentCycle);
   bool brMispredicted = false; 
  // Update reorder buffer (and fake buffer for FP memops)
   fakeIBuffer->updateStatusFake(currentCycle);
   // if we find a mis-predicted branch, apply penalty
   if(reorderBuffer->updateStatus(currentCycle)){
      if (Debug>=2) fprintf(stderr, "===Mispredicted Branch Occured, now purging queues===\n");
      brMispredicted = true; 
      // empty fake ROB
      fakeIBuffer->cancelAllEntries(currentCycle);
      // empty ld-st queue
      loadStoreUnit->flush(currentCycle);
      // empty instruction queues
      flushInstructions();

      lastToken = 0; 
      // clear Decode buffer
      for(int i=0; i<decodeWidth; i++) {
         if(decodeBuffer[i]) 
            decodeBuffer[i]->cancelInstruction(currentCycle);          
         // now clear token
   	   decodeBuffer[i] = NULL;  
      }         

      // clear fetch buffer
      for(int i=0; i<fetchBuffSize; i++) {
         // following is only used to set output dependency if there's any before removing token
         // this is needed because future tokens might still have operand dependence on these tokens 
         if(fetchedBuffer[i]) 
            fetchedBuffer[i]->cancelInstruction(currentCycle);          
         // now clear token
   	   fetchedBuffer[i] = NULL;  
      }         
      // ROB, ld-st queus, and fetch buffers should all be already cleared at this point
      nextAvailableFetch = currentCycle + branchMissPenalty;
      if(Debug>=3) cerr<<"  nextAvilableFetch = "<<currentCycle<<" + "<<branchMissPenalty<<" = "<<nextAvailableFetch<<endl; 
   }
   if (Debug>=2) fprintf(stderr, "===Updated reorder buffer===\n");
   // Update load-store queue (must be done after reorder buffer update
   // so that we remove canceled insns)
   loadStoreUnit->updateStatus(currentCycle);
   if (Debug>=2) fprintf(stderr, "===Updated load/store unit===\n");

   // if a br misprediction occured and queues were flushed,
   // apply penalty and fetch new instructions!
#if 0 
  if(brMispredicted) { 
      if (Debug>=2) fprintf(stderr, "===Mispredicted Branch Occured, now purging queues===\n");
      // clear fetch buffer
      for(int i=0; i<fetchBuffSize; i++) 
	   fetchedBuffer[i] = NULL;  
      lastToken = 0; 
      flushInstructions();
      // ROB, ld-st queus, and fetch buffers should all be already cleared at this point
      nextAvailableFetch = currentCycle + branchMissPenalty; 
   }
#endif

   // Update all functional units to see if any come free
   updateFunctionalUnits();
   if (Debug>=2) fprintf(stderr, "===Updated functional units===\n");

   // Assign new instructions to available functional units
   // - this is the heart of the simulation, really
   scheduleNewInstructions();
   if (Debug>=2) fprintf(stderr, "===Scheduled new instructions===\n");

   // Dispatch: looks into the decodeBuffer to optimally dispatch them to instruction queues
   dispatchInstructions();
   if (Debug>=2) fprintf(stderr, "===Dispatched Instructions in Decode Buffer===\n");
   // Decode: looks into the fetchedBuffer to see if there any instructions need to be decoded
   //       : will try to generate 3 macro-ops(MOPs) per cycle from either the DirectPath or VectorPath decoders
   //       : the generated MOPs will be inserted into the decodeBuffer
   //       : anything left in the fetchedBuffer will need to be decode on next cycle
   decodeInstructions();
   if (Debug>=2) fprintf(stderr, "===Decoded Instructions in Fetch Buffer===\n");
   // Fetch: fetches a fixed number of bytes(32) and translate that into x number of instructions
   //      : also, generates those x instructions/tokens and puts them into a fetchedBuffer    
   fetchInstructions();
   if (Debug>=2) fprintf(stderr, "===Fetched Instruction into Fetch Buffer===\n");
   //refillInstructionQueues();
   // Check for finishing conditions
   if (!(currentCycle % 500000)) {
      double cpi = (double) currentCycle / totalInstructions;
      if (fabs(lastCPI - cpi) < 0.01) 
         return 1;
      lastCPI = cpi;
   }

   if (allQueuesEmpty() && traceF) // quit when trace is done
      return 1;
   else
      return 0;
}


/// @brief Check if all instruction queues are empty
///
bool McOpteron::allQueuesEmpty()
{
   InstructionQueue *iq = instructionQueuesHead;
   while (iq) {
      if (! iq->isEmpty())
         return false;
      iq = iq->getNext();
   }
   return true;
}


/// @brief Update all functional units to current cycle
///
/// The idea here is to allow functional units to decide
/// if they are busy or available at this cycle.
/// Functional units are busy only if new insns can't be
/// issued to them yet; due to pipelining an insn may still
/// be "in" the unit, but the unit is ready for another one.
///
int McOpteron::updateFunctionalUnits()
{
   FunctionalUnit *fu = functionalUnitsHead;
   while (fu) {
      fu->updateStatus(currentCycle);
      fu = fu->getNext();
   }
   return 0;
}


/// @brief Schedule new instructions onto functional units
///
/// This function allows queues to place new instructions on
/// available functional units.
///
int McOpteron::scheduleNewInstructions()
{
   InstructionQueue *iq = instructionQueuesHead;
   while (iq) {
      iq->scheduleInstructions(currentCycle);
      iq = iq->getNext();
   }
   // Waleed: perform a second pass to make sure we get all instructions
   // There's a problem when an instruction on one IQ depends on an instruction on another IQ
   // that is checked later, even when the instruction on the second queue finishes at this cycle
   // we won't be able to catch it unless we perform this second pass!!
   iq = instructionQueuesHead;
   while (iq) {
      iq->scheduleInstructions(currentCycle);
      iq = iq->getNext();
   }

   return 0;
}

/// @brief Flush instructions from instruction queues
///
///
int McOpteron::flushInstructions()
{
   InstructionQueue *iq = instructionQueuesHead;
   while (iq) {
      iq->FlushAllInstructions(currentCycle);
      iq = iq->getNext();
   }
   return 0;
}

/// @brief Fetch new instructions and put them into a buffer
///
///
int McOpteron::fetchInstructions()
{
   /// Waleed: there are 2 types of decoders (DirectPath and VectorPath)
   /// each capable of generating 3MOPs(macro-ops) per cycle 
   /// Both can not work in parallel
   /// The maximum through put is 3 MOPs(macro-ops) per cycle as follows:
   /// For DirectPath:
   /// 3 singlePath instructions per cycle
   /// or 1 double and 2 single path instructions
   /// if we have 2 doublePath instructions in the buffer, then 
   /// only one will be decoded per cycle giving a throughput of 2 MOPs per cycle
   /// For VectorPath:
   /// Upto 3 MOPs per cycle can be generated
   /// a vector path instruction exclusively uses all the decoders!!
   /// finally, the # of fetched bytes is 32 in K10 and 16 in K8
   /// this info was obtained from online documentations (agner manuals) and
   /// http://www.xbitlabs.com/articles/cpu/display/amd-k10_4.html

   /// Waleed:
   /// So, to simulate fetch and decode, we need to do the following:
   /// First, we fetch 32 bytes of code which translates into x number of instructions
   /// Then, we determine what those instructions are!
   /// Then, we determine which are single/double/vector paths
   /// Based on that, we decode what we can during this cycle and the rest are decoded
   /// on the next cycles; note that fetch must be stalled until the 32 bytes are all consumed!!!
   /// If there's a taken branch in this 32-byte buffer, then there's one cycle bubble before we fetch
   /// the new instructions!! The 32-byte buffer can only have contigous code (see agner manuals)

   if (Debug>=2) fprintf(stderr, "Begin Fetch:\n");

   // number of instructions fetched
   int fetchedInsns = 0;
   // if fetchedBuffer is empty, then fetch new set 
   if(fetchedBuffer[0]== NULL) {
      if (nextAvailableFetch > currentCycle) {
         // need to stall for fetching, then stall
         if (Debug>=3) fprintf(stderr,"Stalling for fetch, now %llu next %llu\n", currentCycle, nextAvailableFetch);
         fetchStallCycles++;
         return 0; // leave -- nothing to do when stalling!
      } 
      else {
         // go ahead and fetch
         // update for next fetch (notice the cycle+1 parameter; we are figuring
         // out if the NEXT cycle will have a stall; this is because the stall may
         // be for multiple cycles and so we have to re-use the value of 
         // nextAvailableFetch in the above if statement over multiple calls)
         nextAvailableFetch = memoryModel->serveILoad(currentCycle+1,0,16);
         if(Debug>=3) cerr<<"nextAvailableFetch = "<<nextAvailableFetch<<endl;
         // generate number of instructions for this fetch 
			// this is based on fetching up to 32 bytes of code 

         // fetch based on instruction sizes or fetch block sizes
         assert(instructionSizeProbabilities || fetchSizeProbabilities); 
         if(instructionSizeProbabilities) {
            int curBytes = 0; 
	         while((curBytes += generateInstrSize()) <=32) 
               fetchedInsns++; 
         }
         else if(fetchSizeProbabilities) 
            fetchedInsns = generateFetchSize(); 
			   
         assert(fetchedInsns >= 1 && fetchedInsns <= fetchBuffSize ); 
         if (Debug>=2) fprintf(stderr, "%d tokens can be fetched:\n", fetchedInsns);

         // do we need to insert a fetch bubble (ie for jumps)
         bool bubbleNeeded = false;

         // generate tokens and put them into buffer, check for jumps/taken branches
         for(int i=0; i<fetchedInsns && fetchedInsns<fetchBuffSize; i++){ 
            fetchedBuffer[i] =  generateToken();
            // need to insert a fetch bubble(one cycle delay)
            // unconditional jump
            if(fetchedBuffer[i]->getType()->isUnConditionalJump())
               bubbleNeeded = true; 
            // or conditional branch that's predicted taken!
            if(fetchedBuffer[i]->getType()->isConditionalJump()) { 
               // this is a taken branch
               if(genRandomProbability()<=probTakenBranch)
                  // and it's not mispredicted
                  if(!fetchedBuffer[i]->isMispredictedJump())
                     bubbleNeeded = true;
            }
            // insert bubble if needed and don't generate any more tokens at this cycle! 
				//bubbleNeeded = false; 
            if(bubbleNeeded) { 
               // if fetch is already delayed, no need to insert bubble
               //if(nextAvailableFetch == currentCycle+1) 
                  //nextAvailableFetch++;
               if (Debug>=2) fprintf(stderr, "\t%s Token\n", fetchedBuffer[i]->getType()->getName());
               // don't generate any more tokens at this cycle
               break;
            }
            
            if (Debug>=2) fprintf(stderr, "\t%s Token\n", fetchedBuffer[i]->getType()->getName());
         }   
      }
   } 
   else { // stall fetch until buffer is empty
      if (Debug>=2) fprintf(stderr, "Fetch stalled at this cycle\n");
      fetchStallCycles++;
   } 

   return 0; 

}

/// @brief Put newly fetched instructions into queues
///
/// Allow queues to get new instructions in them if they
/// have room.
/// -- fetch logic - fetching will return a number of 
/// instructions based on average instructions per fetch
/// for the app. We will use those instructions over however
/// many cycles are needed, then fetch more, possibly stalling
///
int McOpteron::decodeInstructions()
{
   /// Waleed: there are 2 types of decoders (DirectPath and VectorPath)
   /// each capable of generating 3MOPs(macro-ops) per cycle 
   /// Both can not work in parallel
   /// The maximum through put is 3 MOPs(macro-ops) per cycle as follows:
   /// For DirectPath:
   /// 3 singlePath instructions per cycle
   /// or 1 double and 2 single path instructions
   /// if we have 2 doublePath instructions in the buffer, then 
   /// only one will be decoded per cycle giving a throughput of 2 MOPs per cycle
   /// For VectorPath:
   /// Upto 3 MOPs per cycle can be generated
   /// a vector path instruction exclusively uses all the decoders!!
   /// finally, the # of fetched bytes is 32 in K10 and 16 in K8
   /// this info was obtained from online documentations (agner manuals) and
   /// http://www.xbitlabs.com/articles/cpu/display/amd-k10_4.html

   /// Waleed:
   /// Here newly fetched instructions are already in the fetchedBuffer:
   /// First, we need to determine the types of tokens in the fetchedBuffer: which are single/double/vector paths?
   /// Based on that, we decode what we can during this cycle based on decoder info above and the rest are decoded
   /// on the next cycles; note that fetch must be stalled until the 32 bytes are all consumed!!!
   /// If there's a taken branch in this 32-byte buffer, then there's one cycle bubble before we fetch
   /// the new instructions!! The 32-byte buffer can only have contigous code (see agner manuals)

   if (Debug>=2) fprintf(stderr, "Begin Decode:\n");

   // if decodebuffer is not empty, then we still have not finished dispatching/issuing previously decoded instrs
   if(decodeBuffer[0]!= NULL) {
      if (Debug>=2) fprintf(stderr, "\tDecode stalled at this cycle\n");
      return 0; 
   }   

   // if fetchedBuffer is empty, then there's nothing to decode 
   if(fetchedBuffer[0]== NULL) {
      if (Debug>=2) fprintf(stderr, "\tDecoder Empty at this cycle\n");
      return 0; 
   }   
   
   // only upto to 3 instructions can be decoded per cycle
   // this will save the index to fetchedBuffer of tokens that can decode on this cycle 
   int *canDecode = new int[decodeWidth]; //
   // initialize 
   for(int i=0;i<decodeWidth; i++) canDecode[i] = -1; 
 
   // number of MOPs (macro-ops)
   int curMops = 0; 
   // maximum MOPs per cycle
   int maxMops = decodeWidth; // set it to decodeWidth for now (this is a problem of mixing concepts of MOP and whole x86)

   // counter
   int c = 0;    
   // loop thru the buffer to determine which tokens can go on this cycle
   for(int i=0; i<fetchBuffSize; i++) { 
      Token *t = fetchedBuffer[i];
      // if this buffer spot is empty, skip
      if(!t) continue; 
      int mops = t->getType()->decodeCost();
      bool isVector = (mops==3); 

      // this token can't be decoded at this cycle becuase the max is reached
      // we can only generate 3 MOPs per cycle
      if((mops+curMops)>maxMops){
         break; 
      } 
      // this token can't be decoded now because it's a VectorPath and we already have tokens ready for decode
      else if(curMops>0 && isVector) { 
         break;
      } 
      else if(isVector) { // if this is a Vector, it's the only one allowed in
         curMops +=mops;
         canDecode[c++]=i;
         break;
      }
      curMops +=mops;
      canDecode[c++]=i;
   }

   // now try to assign the generated tokens to queue
   int numDispatched = 0, numCanDispatch;
   numCanDispatch = c; 

   if (Debug>=2) fprintf(stderr, "\t%d tokens can be decoded in this cycle\n", numCanDispatch);
   for(int i=0; i<numCanDispatch; i++) { 
      if (reorderBuffer->isFull()) {
         reorderBuffer->incFullStall();
         if(Debug >1) fprintf(stderr, "\tReorder Buffer Full\n"); 
         break; // this is fatal, so leave loop early
      }
		// dispatch into ROB and decodeBuffer
      Token * newToken = fetchedBuffer[canDecode[i]];
      reorderBuffer->dispatch(newToken, currentCycle);
      newToken->setIssueCycle(currentCycle); // set issue cycle
      fetchedBuffer[canDecode[i]] = NULL; // clear fetchedBuffer spot
		//usedFetchBuffBytes -= generateInstrSize(); // free up space in fetchBuffer
		//usedFetchBuffBytes = (usedFetchBuffBytes<0)? 0:usedFetchBuffBytes; 
      decodeBuffer[i] = newToken; 
      numDispatched++;
   }
   if (Debug>=2) fprintf(stderr, "\t%d instructions decoded and dispatched into ROB\n", numDispatched);
	
   // now, check if we have decoded decodeWidth instructions 
   // if not, we need to generate empty tokens; this is needed to make sure one whole ROB line is 
   // dispatched and retired together  
   while(numDispatched > 0 && numDispatched < decodeWidth) { 
      // dispatch empty token to take space in the ROB
      reorderBuffer->dispatchEmpty(currentCycle);
      numDispatched++;
   }      
   //fprintf(stderr, "Numassigned at end of decode cycle is %d \n", numAssigned); 
	assert(numDispatched == decodeWidth || numDispatched == 0); 
   // finally, re-order/repack the FetchedBuffer; move all non-empty slots to the top of the buffer
   for(int i=0; i<fetchBuffSize; i++) { 
      bool empty = fetchedBuffer[i]==NULL;
      if(empty) { 
         // find a non-empty spot to replace 
         for(int q=i; q<fetchBuffSize; q++) {
            bool notEmpty = fetchedBuffer[q]!=NULL;
            if(notEmpty) { 
               //swap
               fetchedBuffer[i] = fetchedBuffer[q];
               fetchedBuffer[q] = NULL;
               break; // new
            }         
         }
      }
   }

   // update fetchBuffer index to point to the first empty spot
	fetchBufferIndex=-1; // this means full buffer
   for(int i=0; i<fetchBuffSize; i++) { 
      if(fetchedBuffer[i]==NULL) {
         fetchBufferIndex = i; 
         break; 
      }
   }		
   // print contents of buffer
   // print buffer
	if(Debug>=4) { 
      for(int i=0; i<fetchBuffSize; i++) { 
         if(fetchedBuffer[i]!=NULL) 
            fprintf(stderr, "FetchBuffer %d: Token %llu %s\n", i, 
              fetchedBuffer[i]->instructionNumber(), fetchedBuffer[i]->getType()->getName()); 
      }		
	}
   return 0;
}

/// @brief dispatch newly decoded instructions into queues
///
/// Allow queues to get new instructions in them if they
/// have room.
///
int McOpteron::dispatchInstructions()
{

   if (Debug>=2) fprintf(stderr, "Begin Dispatch:\n");
#if 0 	
   // if we have a leftover FAKE LEA token from previous cycle, assign it first
   InstructionQueue *iq = 0;
   if(lastToken) { 
      iq = instructionQueuesHead;
      bool assignedLastToken = false;
      for (; iq ; iq = iq->getNext()) {
         newToken = lastToken; 
         if (!iq->canHandleInstruction(lastToken))
            // wrong queue for this insn, so skip
            continue;
         if (fakeIBuffer->isFull()) {
            if(Debug >1) fprintf(stderr, "\tFake Reorder Buffer Full...\n"); 
            return 0; // this is fatal, so leave 
         }
         if (iq->isFull()) {
            // queue is full, so skip
            iq->incFullStall();
            //fprintf(stderr, "\tIQ is Full\n"); 
            continue;
         }
         // Finally, token is ready to send to queue, but last check
         // is if it has a memop then the load-store queue must not be full
         if ((!lastToken->isLoad() && !lastToken->isStore()) ||
            loadStoreUnit->add(lastToken, currentCycle)) {
            iq->assignInstruction(lastToken, currentCycle);
            if (lastToken->isFake()) // must be fake
               fakeIBuffer->dispatch(lastToken, currentCycle);
            lastToken = 0; 
            assignedLastToken = true;
            // once a token is assigned, change its issueCycle
            lastToken->setIssueCycle(currentCycle); 
            // that's it break
            break;
         }
      }
      if (Debug>=2 && assignedLastToken) fprintf(stderr, "\tAssigned Leftover Fake LEA Token\n");
      // if we can't assign the FAKE LEA token, return
      if(!assignedLastToken) 
         return 0; 	
   } 

#endif	

   // if decodeBuffer is empty, then there's nothing to issue to IQ's 
   if(decodeBuffer[0]== NULL) {
      if (Debug>=2) fprintf(stderr, "\tNothing to issue at this cycle: DecodeBuffer is Empty\n");
      return 0; 
   }   
   // upto decodeWidth instructions can be dispatched per cycle depending on what's in decodeBuffer
   int canDispatch = 0; 
   while(canDispatch < decodeWidth) { 
      if(decodeBuffer[canDispatch] == NULL) break; 
      canDispatch++;
   }

   // now try to assign the tokens to queue
   int numAssigned=0, numCanAssign=0;
   // the number of tokens that can be assigned to different iq's   
   numCanAssign = canDispatch; 

   if (Debug>=2) 
		fprintf(stderr, "\t%d tokens are in decodeBuffer and may be dispatched in this cycle\n", numCanAssign);

	
   // now, let's see how many of numCanAssign can optimally be dispatched together
   // NOTE: the following array sizes (3*6) are hardcoded --> they should correspond to numCanAssign & number of IQ's. 
	assert(decodeWidth<=3); // avoid any problems
   InstructionQueue *possibleQueues[3][6];  
#if 0   
	int neededLSQslots[3] = {0, 0, 0}; //how many LSQ slots needed by each
#endif	
   // initilize array
   for(int i=0; i<3; i++) { 
      for(int i2=0; i2<6; i2++)
         possibleQueues[i][i2] = NULL; 
   } 
   // For every isntruction in decodeBuff find all queues that it can be issued to   
   // i<3 below is needed to avoid any seg fault
   for(int i=0; i<numCanAssign && i<3; i++) { 
      InstructionQueue *iq = instructionQueuesHead;
      int q = 0; 
		//bool memOpsConsidered = false; 
      Token * newToken = decodeBuffer[i];
      if(Debug >=4) 
         fprintf(stderr, "\tFinding possible IQ's for Token %llu (%s)\n", 
          newToken->instructionNumber(), newToken->getType()->getName());

      for (; iq; iq = iq?iq->getNext():iq) {
         if(Debug >=4) cerr<<"\tLooking at IQ ("<<iq->getName()<<")"<<endl; 
            //fprintf(stderr, "\tLooking at IQ (%s)\n",iq->getName());
	 if (iq->isFull()) {
            if(Debug >=4) 
               cerr<<"\tIQ Full"<<endl;//fprintf(stderr, "\tIQ Full\n");
            continue;
         }
         if (!iq->canHandleInstruction(newToken, currentCycle)) { 
            if(Debug >=4) 
               cerr<<"\t Can not handle Instruction"<<endl;//fprintf(stderr, "\tCan not handle instruction\n");
            continue;
         }	
         if (iq->alreadyAssigned(currentCycle)) {
            if(Debug >=4) 
               cerr<<"\t IQ already assigned at this cycle"<<endl;//fprintf(stderr, "\tIQ already assigned at this cycle\n");
            continue;
         }				
//#if 0
//         // Finally, token can be assigned to iq, but need to see if it has memOps
//         if ((newToken->isLoad() || newToken->isStore()) && !memOpsConsidered) { 
//            if (newToken->isLoad())
//              neededLSQslots[i]++;
//            if (newToken->isStore())
//               neededLSQslots[i]++;
//			   memOpsConsidered = true; 
//			}
//#endif			
         if(Debug >=4 ) cerr<<"\tIQ:"<<iq->getName()<<" is a possible IQ for this token"<<endl;
            //fprintf(stderr, "\tIQ:%s is a possible IQ for this token\n", iq->getName());
         // finally add this possible queue to the list
         possibleQueues[i][q++] = iq; 
      }
      if(Debug >=4 && q==0) cerr<<"\tNo Possible IQ was found"<<endl; 
         //fprintf(stderr, "\tNo Possible IQ was found\n");
      if(Debug >=4 && q>0) cerr<<"\t"<<q<<" Possible IQ's were found"<<endl; 
         //fprintf(stderr, "\t%d Possible IQ's were found\n", q);
		
   } 
	// let's print possible queus
   if(Debug>=3) { 
      for(int i=0; i<numCanAssign; i++) { 
   //fprintf(stderr, "\tToken %llu (%s) can be assigned to: ", decodeBuffer[i]->instructionNumber(), decodeBuffer[i]->getType()->getName());
         cerr<<"\tToken "<<decodeBuffer[i]->instructionNumber()<<" ("<<decodeBuffer[i]->getType()->getName()<<") cane be assigned to: ";
         for(int q=0; q<6; q++) { 
            if(possibleQueues[i][q] == NULL) continue; 
            //fprintf(stderr, "%s,", possibleQueues[i][q]->getName());
            cerr<<possibleQueues[i][q]->getName()<<",";
         }
         cerr<<endl;//fprintf(stderr, "\n"); 
      }		
   }			
   // now find queues so we can issue the max number of instructions
   // Note: I'm hardcoding the number of instructions (3) in the decodeBuffer and 
   // and hardcoding the fact that only the FP IQ can accept upto 3 instructions per cycle
   // The following code must be changed if these assumptions are different. 
   bool assignable[3] = {false, false, false}; 
   InstructionQueue *target[3] = {NULL, NULL, NULL}; 
	InstructionQueue *finalTargets[3] = {NULL, NULL, NULL}; 
	int maxAssignable=0, numAssignable = 0; 
   for(int q0=0; q0<6; q0++) { 
		if(possibleQueues[0][q0] == NULL) continue; 
		assignable[0] = true; 
		target[0] = possibleQueues[0][q0];
		for(int q1=0; q1<6; q1++) { 
			if(possibleQueues[1][q1] != NULL && 
            (possibleQueues[1][q1] != target[0] ||
				possibleQueues[1][q1]->getAcceptRate()>=3)) {
				assignable[1] = true; 
				target[1] = possibleQueues[1][q1];
			}	
			for(int q2=0; q2<6; q2++) { 
				if(target[1] != NULL && 
               possibleQueues[2][q2] != NULL && 
               (possibleQueues[2][q2]->getAcceptRate()>=3 ||
					(possibleQueues[2][q2] != target[1] && 
               possibleQueues[2][q2] != target[0]))) {
					assignable[2] = true; 
					target[2] = possibleQueues[2][q2];
				}
				// how many can we assign at this iteration
				for(int a=0; a<3; a++) { 	
					if(assignable[a]) numAssignable++;
				}			
				// remember best case
				if(numAssignable > maxAssignable) { 
					maxAssignable = numAssignable; 
					for(int i=0; i<maxAssignable; i++)
						finalTargets[i] = target[i]; 
				}
				// reset 
				numAssignable = 0;
				// at end of every loop iteration, reset numbers
				target[2] = NULL;
				assignable[2] = false; 
			}
			// at end of every loop iteration, reset numbers
			target[1] = NULL;
			assignable[1] = false; 
		}
		// quit early condition; if we can assign all instructions in the decodeBuffer
		if(maxAssignable==numCanAssign)
			break; 
		// reset numbers
		target[0] = target[1] = target[2] = NULL; 
		assignable[0] = assignable[1] = assignable[2] = false;
 	}				
	
   if (Debug>=2) 
      fprintf(stderr, "\t%d Tokens can be dispatched in this cycle\n", maxAssignable);
	//for(int i=0; i<maxAssignable; i++) 
	//    fprintf(stderr, "%d queue is %s\n", i, finalTargets[i]->getName()); 
	

	
	// now issue the instructions to the IQ's found above
	for(int i=0; i<maxAssignable; i++) { 
		InstructionQueue *iq = finalTargets[i]; 
		Token * newToken = decodeBuffer[i];
		// sanity check
		if(newToken == NULL) continue; 
		// extra checking
		if (!iq->canHandleInstruction(newToken, currentCycle))
			continue;
		if (iq->isFull()) {
			// queue is full, so skip
			iq->incFullStall();
			//fprintf(stderr, "\tIQ is Full\n"); 
			continue;
		}
		if (iq->alreadyAssigned(currentCycle)) {
			// queue has already been assigned this cycle, so skip
			iq->incAlreadyAssignedStall();
			//fprintf(stderr, "\tIQ already assigned at this cycle\n"); 
			continue;
		}
		// Finally, token is ready to send to queue, but last check
		// is if it has a memop then the load-store queue must not be full
		if ((!newToken->isLoad() && !newToken->isStore()) ||
			  loadStoreUnit->add(newToken, currentCycle)) {

			// finally assign instruction to queue
			iq->assignInstruction(newToken, currentCycle);
			// clear decodeBuffer entry
			decodeBuffer[i] = NULL; 
			numAssigned++; 
		}
  }
  
  if (Debug>=2) fprintf(stderr, "Refilling instruction queues: %d tokens were dispatched at this cycle\n", numAssigned);
  
   // finally, re-order/repack the decodeBuffer in case not all instructions in it were issued;
   // move all non-empty slots to the top of the buffer
   for(int i=0; i<decodeWidth; i++) { 
      bool empty = decodeBuffer[i]==NULL;
      if(empty) { 
         // find a non-empty spot to replace 
         for(int q=i+1; q<decodeWidth; q++) {
            bool notEmpty = decodeBuffer[q]!=NULL;
            if(notEmpty) { 
               //swap
               decodeBuffer[i] = decodeBuffer[q];
               decodeBuffer[q] = NULL;
               break; // new
            }         
         }
      }
   }

   return 0;
}

/// @brief Generate an instruction token
///
/// 
Token* McOpteron::generateToken()
{
   Token *t;
   double p;
   unsigned int i;
   Dependency *dep;
	InstructionInfo *type;
   
   // If running off a trace, bypass this routine and get token from trace
   if (traceF)
      return getNextTraceToken();
      
   if (Debug>=3) fprintf(stderr, "Generating token at %llu\n", currentCycle);
   if(markovModel) { 
      type = markovModel->nextInstruction();	
   }
	else { 
      // sample instruction mix histogram
      p = genRandomProbability();
      // instruction lookup optimization: 2-level binary search start
      i = numInstructionClasses / 2;
      if (p > instructionClassProbabilities[i]) {
         i += numInstructionClasses / 4;
         if (p < instructionClassProbabilities[i])
            i = numInstructionClasses / 2;
      } else {
         i = numInstructionClasses / 4;
         if (p < instructionClassProbabilities[i])
            i = 0;
      }
      // now do linear search
      for (; i < numInstructionClasses; i++)
         if (p < instructionClassProbabilities[i])
            break;
      assert(i < numInstructionClasses);
      type = instructionClasses[i];
   }
   // create token
   t = new Token(type, ++totalInstructions, currentCycle, false);
   // set optional probability (not really used right now)
   p = genRandomProbability();
   t->setOptionalProb(p);
   // set mispredicted flag if appropriate
   if (Debug>=3 && t->getType()->isConditionalJump()) 
      fprintf(stderr, "TTToken (%s) is a conditional jump\n", 
              t->getType()->getName());
   if (t->getType()->isConditionalJump() && p <= probBranchMispredict) {
      if (Debug>=3) fprintf(stderr, "Mispredict!\n");
      t->setBranchMispredict();
   }
   // sample probability of address operand
   // TODO: allow both load and store on same insn
   // Waleed: just re-arranging the check for load/store; 
   // Waleed: i'm checking if there's a prob for a load/store first
   // Waleed: still, as mentioned above, the case for tokens with both load & store needs 
   // Waleed: to be addressed
   p = genRandomProbability();
   if (p <= (t->getType()->getLoadProb()+t->getType()->getStoreProb()) ) {
      if(p <= t->getType()->getLoadProb() ) {    
         if (Debug>=3) fprintf(stderr, "  has load  at %llu\n", fakeAddress);
         t->setMemoryLoadInfo(fakeAddress++, 8); // 8-byte fetch
      } 
      else {
         if (Debug>=3) fprintf(stderr, "  has store at %llu\n", fakeAddress);
         t->setMemoryStoreInfo(fakeAddress++, 8); // 8-byte store
      }
   }   
   // generate a dependency record (could be null)
   dep = new Dependency(t, NULL); //addNewDependency(t, NULL);
   // set a pointer in token
   t->setInDependency(dep); // might be null, but ok
	unsigned indep = 0; 
   if(dep) indep = dep->numInputs; 
   if (Debug>=3 && dep) fprintf(stderr, "  num indeps: %u\n", indep);

   // Now fix InstructionInfo record in case there are multiple and
   // they depend on data direction, size, etc.
   //t->fixupInstructionInfo();
   // MORE HERE TO DO?
   // done
   if (TraceTokens) 
      t->dumpTokenTrace(stderr);
   if (Debug>=2) fprintf(stderr, "  token %llu is %s type %u (%g %g %g) addr: %s\n",
           t->instructionNumber(), t->getType()->getName(), t->getType()->getCategory(),
           t->getType()->getOccurProb(),
           t->getType()->getLoadProb(), t->getType()->getStoreProb(), 
           t->needsAddressGeneration()?"T":"F");
	if(Debug>=2) fprintf(stderr, "  token %s num indeps: %u \n", 
                  t->getType()->getName(),
                  indep);			
  //fprintf(stderr, "%s\n", t->getType()->getName());			
  
   
	return t;
}

/// @brief Get an instruction token from a trace file
///
/// For now, the trace format is one instruction per line, each 
/// line formatted as "mnemonic opsize memflag usedist". The mnemonic
/// is the instruction name, opsize is the operand size (8/16/32/64/128),
/// memflag is 0/1/2/3 for none/load/store/both, and usedist is the number of
/// instructions ahead that the result of this one will be used 
/// (1 == next instruction).
///
Token* McOpteron::getNextTraceToken()
{
   Token *t;
   char line[256], mnemonic[24], op1[24], op2[24], op3[24];
   unsigned int opSize, memFlag, sourceOps;
   int n;
   InstructionInfo *it;
   double p;
   Dependency *dep;

   if (Debug>=2) fprintf(stderr, "Getting trace token at %llu\n", currentCycle);
   if (!fgets(line, sizeof(line), traceF)) {
      if (repeatTrace) {
         fseek(traceF,0,SEEK_SET);
         if (!fgets(line, sizeof(line), traceF)) 
            return 0;
      } else 
         return 0;
   }
   // read instruction
   op1[0] = op2[0] = op3[0] = '\0';
   if (sscanf(line, "%s %u %u %u %[^,]%*c %[^,]%*c %s",mnemonic, &opSize, &memFlag, &sourceOps,op1,op2,op3) != 7) { 	
      if (sscanf(line, "%s %u %u %u %[^,]%*c %s",mnemonic, &opSize, &memFlag, &sourceOps,op1,op2) != 6) {
         if (sscanf(line, "%s %u %u %u %s",mnemonic, &opSize, &memFlag, &sourceOps,op1) != 5) {          
            if (sscanf(line, "%s %u %u %u",mnemonic, &opSize, &memFlag, &sourceOps) != 4) {
              return 0;
            }
         }
      }
   }
   
// old code	
	//n = sscanf(line, "%s %u %u %u", mnemonic, &opSize, &memFlag, &sourceOps);
   //if (n != 4)
   //   return 0;
// end old code
   // set up operands for lookup
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
   //fprintf(stderr, "\nNow Looking for (%s) (%s) (%s) (%s) (%u)\n", mnemonic, op1, op2, op3, iOpSize);
   //Waleed: now fix up mnemonic
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
   char *ptr;
   if ((ptr=strstr(mnemonic,"_NEAR")))
      *ptr = '\0';
   if ((ptr=strstr(mnemonic,"_XMM")))
      *ptr = '\0';
				
   it = instructionClassesHead->findInstructionRecord(mnemonic, opSize, op1, op2, op3);
   if (!it)
      return 0;
   // if we got here, we read the line and parsed it ok, and we found the mnemonic
   // in the InstructionInfo records
   t = new Token(it, ++totalInstructions, currentCycle, false);
	// set source ops
   t->getType()->setSourceOps(sourceOps);
   unsigned int *distances= NULL;
   if(sourceOps>0)
      distances = new unsigned int[sourceOps];
   // now capture input register  dep distances
   for(unsigned int i=0; i<sourceOps; i++) { 
      if (!fgets(line, sizeof(line), traceF)) {
         if (repeatTrace) {
            fseek(traceF,0,SEEK_SET);
            if (!fgets(line, sizeof(line), traceF)) 
               return 0;
         } else 
            return 0;
      }
      n = sscanf(line, "%u", &distances[i]);
      if (n != 1)
         return 0;
   }   

   // set optional probability
   p = genRandomProbability();
   t->setOptionalProb(p);
   // set mispredicted flag if appropriate 
   // TODO: a trace can't have mispredicted jumps?
   if (t->getType()->isConditionalJump() && p <= probBranchMispredict) {
      t->setBranchMispredict();
   }
   // set memory access info
   if (memFlag & 1) {
      if (Debug>=3) fprintf(stderr, "  has load  at %llu\n", fakeAddress);
      t->setMemoryLoadInfo(fakeAddress++, 8); // 8-byte fetch
   } else if (memFlag & 2) {
      if (Debug>=3) fprintf(stderr, "  has store at %llu\n", fakeAddress);
      t->setMemoryStoreInfo(fakeAddress++, 8); // 8-byte store
   }
   // generate a dependency record (could be null)
   dep = new Dependency(t, distances); //addNewDependency(t, distances);
   // set a pointer in token
   t->setInDependency(dep); // might be null, but ok
	unsigned indep = 0; 
   if(dep) indep = dep->numInputs; 
   if (Debug>=3 && dep) fprintf(stderr, "  num indeps: %u\n", indep);

   // Now fix InstructionInfo record in case there are multiple and
   // they depend on data direction, size, etc.
   t->fixupInstructionInfo();
   // MORE HERE TO DO?
   // done
   if (Debug>=2) fprintf(stderr, "  token %llu is %s type %u (%g %g %g) addr: %s\n",
           t->instructionNumber(), t->getType()->getName(), t->getType()->getCategory(),
           t->getType()->getOccurProb(),
           t->getType()->getLoadProb(), t->getType()->getStoreProb(), 
           t->needsAddressGeneration()?"T":"F");


   //fprintf(stderr, "%s\n", t->getType()->getName());			

   return t;
}

}//End namespace McOpteron

//=========================================================
//
// Main program driver, if not compiled into a library
//
//=========================================================

#ifndef NOTSTANDALONE

#include <stdlib.h>

static const char* helpMessage = 
"\nUsage: mcopteron [options]\n\
Options:\n\
  --debug #        print lots of debugging information (# in 1-3)\n\
  --cycles #       set number of cycles to simulate (default: 100K?)\n\
  --converge       run until CPI converges (not default)\n\
  --deffile name   use 'name' as insn def file (default: opteron-insn.txt)\n\
  --dcycle #       start debugging output at cycle # (rather than 0)\n\
  --simix          print out static input instruction mix at beginning\n\
  --imix           print out simulation instruction mix at end\n\
  --mixfile name   use 'name' as insn mix input file (default: usedist_new.all)\n\
  --appdir name    use 'name' as application file directory (default: .)\n\
  --seed #         set random number seed\n\
  --trace name     use 'name' as input instruction trace file\n\
  --traceout       generate a token trace to stderr\n\
  --sepsize        keep instruction types separate on operand size\n\
  --newimix name   use 'name' as an i-mix-only file (new format)(default: instrMix.txt)\n\
  --isizefile name use 'name' as instruction size distribution file(default: instrSizeDistr.txt)\n\
  --fsizefile name use 'name' as fetch size distribution file(default: fetchSizeDistr.txt)\n\
  *note: --isizefile and --fsizefile are exclusive options; only one of them should be used\n\
  --transfile name use 'name' as instr transition probability file for Markov-based token generator\n\
                   (if this is not used, instr probabilities from instruction mix will be used)\n\
  --defaults       use default file names and options for --newimix, --mixfile, and --deffile\n\
  --repeattrace    use the input trace over and over\n\n";
  
void doHelp()
{
   fputs(helpMessage, stderr);
   exit(1);
}

/// @brief Standalone simulator driver
///
int main(int argc, char **argv)
{
   int i;
   int TT;			//Scoggin: Added with internalizing TraceTokens;
   bool TIN;			//Scoggin: Added with internalizing treatImmAsNone;
   unsigned long numSimCycles = 100000;
   unsigned long debugCycle = 0, c;
   unsigned int debug = 0;
   unsigned long seed = 100;
   bool untilConvergence = false;
   bool printIMix = false;
   bool printStaticIMix = false;
   string mixFile;
   string defFile;
   string traceFile;
   string appDirectory = ".";
   string newIMixFile;
   string instrSizeFile;
   string fetchSizeFile;
   string transFile;
   bool useNewIMix = false;
   bool repeatTrace = false;
   McOpteron::McOpteron *cpu;  //Scoggin added namespace McOpteron::
   McOpteron::Debug = 0;
   TT=0; 	//Scoggin: internalized TraceTokens = 0;
   TIN=false;	//Scoggin: internalized treatImmAsNone;
   for (i = 1; i < argc; i++) {
      if (!strcmp("--debug",argv[i])) {
         if (i == argc-1) doHelp();
         debug = atoi(argv[++i]);
      } else if (!strcmp("--converge",argv[i])) {
         untilConvergence = true;
      } else if (!strcmp("--cycles",argv[i])) {
         if (i == argc-1) doHelp();
         numSimCycles = atoi(argv[++i]);
      } else if (!strcmp("--dcycle",argv[i])) {
         if (i == argc-1) doHelp();
         debugCycle = atol(argv[++i]);
      } else if (!strcmp("--deffile",argv[i])) {
         if (i == argc-1) doHelp();
         defFile = argv[++i];
      } else if (!strcmp("--imix",argv[i])) {
         printIMix = true;
      } else if (!strcmp("--simix",argv[i])) {
         printStaticIMix = true;
      } else if (!strcmp("--mixfile",argv[i])) {
         if (i == argc-1) doHelp();
         mixFile = argv[++i];
      } else if (!strcmp("--appdir",argv[i])) {
         if (i == argc-1) doHelp();
         appDirectory = argv[++i];
      } else if (!strcmp("--seed",argv[i])) {
         if (i == argc-1) doHelp();
         seed = atol(argv[++i]);
      } else if (!strcmp("--trace",argv[i])) {
         if (i == argc-1) doHelp();
         traceFile = argv[++i];
      } else if (!strcmp("--repeattrace",argv[i])) {
         repeatTrace = true;
      } else if (!strcmp("--traceout",argv[i])) {
         TT=1;	//Scoggin: Internalized TraceTokens = 1;
      } else if (!strcmp("--sepsize",argv[i])) {
         McOpteron::InstructionInfo::separateSizeRecords = true;
      } else if (!strcmp("--newimix",argv[i])) {
         if (i == argc-1) doHelp();
         newIMixFile = argv[++i];
         useNewIMix = true;
		} else if (!strcmp("--isizefile",argv[i])) {
         if (i == argc-1) doHelp();
         instrSizeFile = argv[++i];
		} else if (!strcmp("--fsizefile",argv[i])) {
         if (i == argc-1) doHelp();
         fetchSizeFile = argv[++i];
		} else if (!strcmp("--transfile",argv[i])) {
         if (i == argc-1) doHelp();
         transFile = argv[++i];
		} else if (!strcmp("--defaults",argv[i])) {
          // use default file names
          mixFile = "usedist_new.all";
          defFile = "agner-insn.txt";
          newIMixFile = "instrMix.txt";
          //instrSizeFile = "instrSizeDistr.txt";
          //fetchSizeFile = "fetchSizeDistr.txt";
          //transFile = "transition_prob.txt"; 
		}else if (!strcmp("--immasnone",argv[i])) {
         TIN=true; 	//Scoggin: Internalized McOpteron::treatImmAsNone = true;
      } else  {
         doHelp();
         return 0;
      }
   }
   // quick check
   if((instrSizeFile.size() && fetchSizeFile.size()) || (!instrSizeFile.size() && !fetchSizeFile.size())) {
      fprintf(stderr, "You must specify either instrSizeFile or fetchSizeFile\n");
      doHelp();
      return 0; 
   }
   if (debugCycle == 0)
      McOpteron::Debug = debug;
   McOpteron::seedRandom(seed);
   cpu = new McOpteron::McOpteron(); 	//Scoggin: Added namespace McOpteron::
   cpu->TraceTokens=TT;			//Scoggin: Internalized TraceTokens
   cpu->treatImmAsNone=TIN;		//Scoggin: Internalized treatImmAsNone
   cpu->local_debug=McOpteron::Debug;		//Scoggin: Added to pass Debug around in library form
   cpu->init(appDirectory, defFile, mixFile, traceFile, repeatTrace, newIMixFile, instrSizeFile, fetchSizeFile, transFile);
   if (printStaticIMix)
      cpu->printStaticIMix();
   if (untilConvergence) {
      fprintf(stderr, "Simulating till convergence\n");
      for (c = 0; true; c++) {
         if (c == debugCycle)
            {
            McOpteron::Debug = debug;
            cpu->local_debug=McOpteron::Debug;//Scoggin: Added to pass Debug around in library form
            }
         if (cpu->simCycle())
            break;
      }
   } else {
      fprintf(stderr, "Simulating %ld cycles\n", numSimCycles);
      for (c = 0; c < numSimCycles; c++) {
         if (c == debugCycle)
            {
            McOpteron::Debug = debug;
            cpu->local_debug=McOpteron::Debug;//Scoggin: Added to pass Debug around in library form
            }
         if (cpu->simCycle())
            ; //break;
      }
   }
   fprintf(stderr, "Done simulating\n");
   cpu->finish(printIMix);
   delete cpu;
   fprintf(stderr, "Tokens created, deleted: %u %u\n", McOpteron::Token::totalTokensCreated,
           McOpteron::Token::totalTokensDeleted);
   return 0;
}

#endif
