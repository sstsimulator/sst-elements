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


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "MemoryModel.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Memory model constructor
///
/// This routine just zero's out fields. Use the init methods to 
/// populate the object with real data 
///
MemoryModel::MemoryModel()
{
   memQHead = memQTail = 0;
   numLoadsInQ = numStoresInQ = 0;
   cdfSTBHit = cdfL1Hit = cdfL2Hit = cdfL3Hit = cdfTLB1Hit = cdfTLB2Hit = 0.0;
   cdfICHit = cdfIL2Hit = cdfIL3Hit = cdfITLB1Hit = cdfITLB2Hit = 0.0;
   numL1Hits = numL2Hits = numL3Hits = 0;
   numMemoryHits = numTLB1Misses = numTLB2Misses = 0;
   numICHits = numIL2Hits = numIL3Hits = 0;
   numIMemoryHits = numITLB1Misses = numITLB2Misses = 0;
   numSTBHits = numStores = numLoads = 0;
   lastLoad = lastStore = 0;
}


/// @brief Memory model destructor
///
/// This deletes any outstanding memory ops in the queue
///
MemoryModel::~MemoryModel()
{
   MemoryOp *p;
   while (memQHead) {
      p = memQHead->next;
      delete memQHead;
      memQHead = p;
   }
   fprintf(stdout, "MM: loads: %llu  from STB: %llu  L1: %llu  L2: %llu  L3: %llu  Mem: %llu\n",
           numLoads, numSTBHits, numL1Hits, numL2Hits, numL3Hits, numMemoryHits);
   fprintf(stdout, "MM: stores: %llu \n", numStores);
   fprintf(stdout, "MM: iloads from IC: %llu  L2: %llu  L3: %llu  Mem: %llu\n",
           numICHits, numIL2Hits, numIL3Hits, numIMemoryHits);
   fprintf(stdout, "MM: DTLB1,2 misses %llu %llu   ITLB1,2 misses: %llu %llu\n",
           numTLB1Misses, numTLB2Misses, numITLB1Misses, numITLB2Misses);
}

/// @brief Initialize memory hierarchy latencies
///
/// @param latTLB is the TLB latency in cycles
/// @param latL1 is the L1 cache latency in cycles
/// @param latL2 is the L2 cache latency in cycles
/// @param latMem is the memory access latency in cycles
///
void MemoryModel::initLatencies(unsigned int latTLB1, unsigned int latTLB2,
       unsigned int latL1, unsigned int latL2, unsigned int latL3, 
       unsigned int latMem)
{
   latencyTLB1 = latTLB1;
   latencyTLB2 = latTLB2;
   latencyL1 = latL1;
   latencyL2 = latL2;
   latencyL3 = latL3;
   latencyMem = latMem;
   if (Debug) fprintf(stderr, "Latencies: TLB1,2 %u %u L1 %u L2 %u Mem %u\n",
                      latencyTLB1, latencyTLB2, latencyL1, latencyL2, latencyMem);
}

/// @brief Initialize memory probabilities
///
/// Right now, these are assumed to be independent, but really should be a 
/// CDF.
/// @param pSTBHit is the probability a load will be satisfied from the
///        store buffer
/// @param pL1Miss is the probability a load is not satisfied in the L1 cache
///        given that it missed the store buffer
/// @param pTLB1Miss is the probability a load or store misses L1 DTLB
/// @param pTLB2Miss is the probability a load or store misses L2 DTLB
/// @param pICMiss is the probability an instruction fetch is not satisfied
///        in the I-cache
/// @param pITLB1Miss is the probability an instruction fetch misses L1 ITLB
/// @param pITLB2Miss is the probability an instruction fetch misses L2 ITLB
/// @param pL2Miss is the probability a load is not satisfied in the L2 cache
///        given that it missed the store buffer and the L1 cache
/// @param pL3Miss is the probability a load is not satisfied in the L2 cache
///        given that it missed the store buffer and the L1 cache
///
void MemoryModel::initProbabilities(double pSTBHit,  double pL1Miss, 
         double pTLB1Miss, double pTLB2Miss, double pICMiss, 
         double pITLB1Miss, double pITLB2Miss, double pL2Miss, double pL3Miss)
{
   // set up a cumulative distribution function to map a single 
   // probability to multiple "hit" levels; the formula for the
   // L1-L3 values is an algebraically simplified form
   cdfSTBHit = pSTBHit;
   cdfL1Hit = 1.0 + cdfSTBHit*pL1Miss - pL1Miss;
   cdfL2Hit = 1.0 + cdfL1Hit*pL2Miss - pL2Miss;
   cdfL3Hit = 1.0 + cdfL2Hit*pL3Miss - pL3Miss;
   cdfTLB1Hit = 1.0 - pTLB1Miss;
   cdfTLB2Hit = 1.0 + cdfTLB1Hit*pTLB2Miss - pTLB2Miss;
   cdfICHit = 1.0 - pICMiss;
   cdfIL2Hit = 1.0 + cdfICHit*pL2Miss - pL2Miss;
   cdfIL3Hit = 1.0 + cdfIL2Hit*pL3Miss - pL3Miss;
   cdfITLB1Hit = 1.0 - pITLB1Miss;
   cdfITLB2Hit = 1.0 + cdfITLB1Hit*pITLB2Miss - pITLB2Miss;
   if (Debug) fprintf(stderr, "Data hit CDF %%: STB %g L1 %g L2 %g L3 %g\n",
                      pSTBHit, cdfL1Hit, cdfL2Hit, cdfL3Hit);
   if (Debug) fprintf(stderr, "Inst hit CDF %%: IC %g L2 %g L3 %g\n",
                      cdfICHit, cdfIL2Hit, cdfIL3Hit);
   if (Debug) fprintf(stderr, "TLBs hit CDFs %%: DTLB1 %g DTLB2 %g ITLB1 %g ITLB2 %g\n",
                      cdfTLB1Hit, cdfTLB2Hit, cdfITLB1Hit, cdfITLB2Hit);
}

/// @brief Compute cost of serving a data load
///
/// This computes the cycle at which a load will be satisfied.
/// @param currentCycle is the current cycle count when load issued
/// @param address is the address this load is accessing (not used yet)
/// @param numbytes is the number of bytes being read (not used yet)
/// @param reason is an out-parameter giving the reason for the load delay
/// @return the cycle count at which the load will be satisfied
///
CycleCount MemoryModel::serveLoad(CycleCount currentCycle, Address address,
                                  unsigned int numBytes)
{
	CycleCount satisfiedCycle = currentCycle;
   double p;
	numLoads++;
   //purgeMemoryQ(currentCycle); // update mem Q (removes old loads/stores)

   // check if an existing load is not yet satisfied
   //if (lastLoad && lastLoad->satisfiedCycle > satisfiedCycle) {
   //   // stall until current load is done
   //   satisfiedCycle = lastLoad->satisfiedCycle + Cost::LoadAfterLoad;
   //}

   // All memops might suffer a TLB miss, so adjust if this happens
   // -- JEC: this should have a separate cpi accounting category
   p = genRandomProbability();
   if (p > cdfTLB1Hit) {
      numTLB1Misses++;
      satisfiedCycle += latencyTLB1;
   } else if (p > cdfTLB2Hit) {
      numTLB1Misses++;  // missed both!
      numTLB2Misses++;
      satisfiedCycle += latencyTLB2;
   }

   p = genRandomProbability();
   // Now step through memory hierarchy (including store buffer)
   if (p <= cdfSTBHit) { 
   	// Load is satisfied from store buffer
      numSTBHits++;
      satisfiedCycle += Cost::LoadFromSTB; 
   } else if (p <= cdfL1Hit) { 
   	// Load is satisfied in L1 cache
      numL1Hits++;
      satisfiedCycle += latencyL1;
   } else if (p <= cdfL2Hit) {
      // load satisfied in L2
      numL2Hits++;
      satisfiedCycle += latencyL2;
   } else if (p <= cdfL3Hit) {
      // load satisfied in L3
      numL3Hits++;
      satisfiedCycle += latencyL3;
   } else { 
      // load satisfied in Memory
      numMemoryHits++;
      satisfiedCycle += latencyMem;
   }
   // add this load to the mem Q
   //addToMemoryQ(satisfiedCycle, MEMLOAD);
   return satisfiedCycle;
}

/// @brief Compute cost of serving an instruction fetch load
///
/// This computes the cycle at which an instruction fetch will be satisfied
/// @param currentCycle is the current cycle count when fetch issued
/// @param address is the address this fetch is accessing (not used yet)
/// @param numbytes is the number of bytes being read (not used yet)
/// @param reason is an out-parameter giving the reason for the load delay
/// @return the cycle count at which the load will be satisfied
///
CycleCount MemoryModel::serveILoad(CycleCount currentCycle, Address address,
                                   unsigned int numBytes)
{
	CycleCount satisfiedCycle = currentCycle;
   double p;
   
	numILoads++;
	
	// JEC: Instruction loads should check for conflicting loads
	//      from L2 on up, since they share resources
   //purgeMemoryQ(currentCycle); // update mem Q (removes old loads/stores)

   // All memops might suffer a TLB miss, so adjust if this happens
   p = genRandomProbability();
   if (p > cdfITLB1Hit) {
      numITLB1Misses++;
      satisfiedCycle += latencyTLB1;
   } else if (p > cdfITLB2Hit) {
      numITLB1Misses++;  // missed both!
      numITLB2Misses++;
      satisfiedCycle += latencyTLB2;
   }

   p = genRandomProbability();
   // Now step through memory hierarchy
   if (p <= cdfICHit) { 
   	// Load is satisfied in I cache
      numICHits++;
      //satisfiedCycle += 0;  // no cost to hit i-cache
   } else {
      // Will hit L2 or memory, so handle conflicts
      // check if an existing load is not yet satisfied
      //if (lastLoad && lastLoad->satisfiedCycle > satisfiedCycle) {
      //   // stall until current load is done
      //   satisfiedCycle = lastLoad->satisfiedCycle + Cost::LoadAfterLoad;
      //}
      if (p <= cdfIL2Hit) {
         // load satisfied in L2
         numIL2Hits++;
         satisfiedCycle += latencyL2;
      } else if (p <= cdfIL3Hit) {
         // load satisfied in L3
         numIL3Hits++;
         satisfiedCycle += latencyL3;
      } else {
         // load satisfied in Memory
         numIMemoryHits++;
         satisfiedCycle += latencyMem;
      }
      //addToMemoryQ(satisfiedCycle, MEMLOAD);
   }

   return satisfiedCycle;
}

/// @brief Compute cost of serving a data store
///
/// This computes the cycle at which a store will be satisfied and
/// how much the store instruction might need to stall.
/// @param currentCycle is the current cycle count when store issued
/// @param address is the address this store is accessing (not used yet)
/// @param numbytes is the number of bytes being written (not used yet)
/// @param reason is an out-parameter giving the reason for the store delay
/// @return the cycle count which the store needs to stall until (different
///         than when the store will be satisfied!)
///
CycleCount MemoryModel::serveStore(CycleCount currentCycle, Address address,
                         unsigned int numBytes)
{
   // how to compute a store's satisfied cycle? Maybe something better
   // would be to not compute it at all and have sim call a doStore()
   // method when a long instruction is going to give it time. Or if we
   // can come up with a probabilistic distribution, sample that.
   //CycleCount satisfiedCycle = currentCycle + Cost::AverageStoreLatency;
   CycleCount stallUntilCycle = currentCycle; // assume can finish now
   
   numStores++;

   //if (lastStore && lastStore->satisfiedCycle > satisfiedCycle) 
   //   satisfiedCycle = lastStore->satisfiedCycle + Cost::StoreAfterStore;

   if (numStoresInQ >= Config::StoreBufferSize) {
      // store buffer is full, must stall until an open slot
      // find first store in Q
      MemoryOp *firstStore = memQHead;
      while (firstStore && firstStore->op != MEMSTORE)
         firstStore = firstStore->next;
      if (!firstStore) assert(0);
      stallUntilCycle = firstStore->satisfiedCycle+1;
      //purgeMemoryQ(stallUntilCycle);
   }
   //addToMemoryQ(satisfiedCycle, MEMSTORE);
   return stallUntilCycle;
}

/// @brief Add a load or store to the current memory op queue
///
/// @param whenSatisfied is the cycle count when this op will finish
/// @param type is either MEMLOAD or MEMSTORE
/// @return always 0
///
int MemoryModel::addToMemoryQ(CycleCount whenSatisfied, MemOpType type)
{
   MemoryOp *m;
   m = new MemoryOp();
   m->insnNum = 0;
   m->address = 0;
   m->numBytes = 0;
   m->issueCycle = 0;
   m->satisfiedCycle = whenSatisfied;
   m->op = type;
   m->next = 0;
   if (type == MEMSTORE) {
      lastStore = m;
      numStoresInQ++;
   } else {
      lastLoad = m;
      numLoadsInQ++;
   }
   if (!memQTail) {
      memQHead = memQTail = m;
   } else {
      memQTail->next = m;
      memQTail = m;
   }
   return 0;
}


/// @brief Purge the memory queue up to some certain cycle.
///
/// Remove "old" memory ops from the queue that are satisfied
/// up to the given cycle count.
/// @param upToCycle is the cycle count to purge up to
/// @return always 0.0
///
double MemoryModel::purgeMemoryQ(CycleCount upToCycle)
{
   MemoryOp *p;
   while (memQHead && memQHead->satisfiedCycle <= upToCycle) {
      p = memQHead;
      memQHead = memQHead->next;
      if (p->op == MEMLOAD)
         numLoadsInQ--;
      else
         numStoresInQ--;
      if (lastLoad == p) lastLoad = 0;
      if (lastStore == p) lastStore = 0;
      delete p;
   }
   if (!memQHead) memQTail = 0; // list is empty, clear tail ptr too
   
   if (numLoadsInQ + numStoresInQ > 10000) {
      fprintf(stderr, "Panic: too many ops in memq: %u %u\n",
              numLoadsInQ, numStoresInQ);
      exit(0);
   }
   return 0.0;
}

/// @brief return number of outstand ops in queue
///
/// @param memOp is either MEMLOAD or MEMSTORE
///
unsigned int MemoryModel::numberInMemoryQ(MemOpType memOp)
{
   if (memOp == MEMLOAD)
      return numLoadsInQ;
   else if (memOp == MEMSTORE)
      return numStoresInQ;
   return 0;
}

/// @brief Get data load operation statistics
///
void MemoryModel::getDataLoadStats(unsigned long long *numLoads,
                         unsigned long long *numSTBHits,
                         unsigned long long *numL1Hits,
                         unsigned long long *numL2Hits,
                         unsigned long long *numL3Hits,
                         unsigned long long *numMemoryHits,
                         unsigned long long *numTLB1Misses,
                         unsigned long long *numTLB2Misses)
{
   *numLoads = this->numLoads;
   *numSTBHits = this->numSTBHits;
   *numL1Hits = this->numL1Hits;
   *numL2Hits = this->numL2Hits;
   *numL3Hits = this->numL3Hits;
   *numMemoryHits = this->numMemoryHits;
   *numTLB1Misses = this->numTLB1Misses;
   *numTLB2Misses = this->numTLB2Misses;
}

/// @brief Get instruction load statistics
///
void MemoryModel::getInstLoadStats(unsigned long long *numILoads,
                         unsigned long long *numICHits,
                         unsigned long long *numIL2Hits,
                         unsigned long long *numIL3Hits,
                         unsigned long long *numIMemoryHits,
                         unsigned long long *numITLB1Misses,
                         unsigned long long *numITLB2Misses)
{
   *numILoads = this->numILoads;
   *numICHits = this->numICHits;
   *numIL2Hits = this->numIL2Hits;
   *numIL3Hits = this->numIL3Hits;
   *numIMemoryHits = this->numIMemoryHits;
   *numITLB1Misses = this->numITLB1Misses;
   *numITLB2Misses = this->numITLB2Misses;
}

/// @brief Get store operation statistics
///
void MemoryModel::getStoreStats(unsigned long long *numStores )
{
   *numStores = this->numStores;
}
}//end namespace McOpteron
