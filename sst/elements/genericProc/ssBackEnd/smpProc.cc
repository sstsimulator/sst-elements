// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2009, 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "smpProc.h"
#include "proc.h"
#include "ssb_cache.h"
#include "ssb_mainProc.h"
#include "ssb.h"

smpProc::smpProc(mainProc *mp) : myMainProc(mp) {
  invalidates[0] = invalidates[1] = 0;
  writeBacks = busMemAccess = 0;
}

simAddress smpProc::getBAddr(simAddress a) {
  simAddress baddr;
  if (myMainProc->cache_dl2) 
    baddr = cache_get_blkAddr(myMainProc->cache_dl2, a);
  else if (myMainProc->cache_dl1) 
    baddr = cache_get_blkAddr(myMainProc->cache_dl1, a);
  return baddr;
}

//: Process bus write hit message
//
// We implement the H&P algorithm, so a write hit is the same as a
// write miss.
void smpProc::busWriteHit(simAddress sa) {busWriteMiss(sa);}

void smpProc::writeBackBlock(simAddress sa) {
  if (sa) {
    writeBacks++;
    myMainProc->myProc->sendMemoryReq(STORE, sa, (instruction*)~0, 
				      myMainProc->mainProcID);
  }
}

void smpProc::busReadMiss(simAddress sa) {
  //printf("%d: saw read miss for %x\n", getMainProcID(), sa);
  if (sa) {
    tagMap::iterator it = blkTags.find(getBAddr(sa));
    if ((it != blkTags.end()) && (it->second == EXCLUSIVE)) {
      //printf("exclu->shared %x\n", sa);
      // we should have it exclusively
      // write back block
      writeBackBlock(sa);
      // change to shared
      it->second = SHARED;
    }
  }
}

// perform write invalidate
void smpProc::busWriteMiss(simAddress sa) {
  //printf("%d: erasing tag for %x\n", getMainProcID(), sa);
  if (sa) {   
    tagMap::iterator it = blkTags.find(getBAddr(sa));
    // do we have it?
    if (it != blkTags.end()) {
      bool needWriteBack = 0;
      if (it->second == EXCLUSIVE) {
	needWriteBack = 1; //only write it back if we are exclusive
      }
      // remove the tag
      blkTags.erase(it);
      // invalidate from cache(s)
      if (myMainProc->cache_dl1) {
	bool writtenBack = 0;
	if (cache_probe(myMainProc->cache_dl1, sa)) {
	  if (needWriteBack) {
	    writeBackBlock(sa);
	    writtenBack = 1;
	  }
	  invalidates[0]++;
	  //printf(" inv l1\n");
	  cache_invalidate_addr(myMainProc->cache_dl1, sa, 
				myMainProc->TimeStamp());
	}
	if (myMainProc->cache_dl2) {
	  if (cache_probe(myMainProc->cache_dl2, sa)) {
	    if (needWriteBack) {
	      if (writtenBack == 0) {writeBackBlock(sa);}
	    }
	    invalidates[1]++;
	    cache_invalidate_addr(myMainProc->cache_dl2, sa, 
				  myMainProc->TimeStamp());
	  }
	}
      }
    }
  }
}


void smpProc::finish() {
  if (myMainProc->myP->isCoherent()) {
    for (int i = 0; i < 2; ++i) 
      printf("%llu L%d Invalidations\n", invalidates[i], i+1);
    printf("%llu busMemAccess\n", busMemAccess);
    printf("%llu writebacks\n", writeBacks);
  }
}


void smpProc::noteWrite(simAddress a) {
  if (myMainProc->myP->isCoherent() && a) {
    //place write miss on bus
    md_addr_t baddr = getBAddr(a);
    blkTag &bTag = blkTags[baddr];
    if (bTag != EXCLUSIVE) {
      //printf("%d: note write %x\n", getMainProcID(), baddr);
      myMainProc->myP->postMessage(proc::WRITE_MISS, baddr, myMainProc);
      blkTags[baddr] = EXCLUSIVE; //change tag to Exclusive    
    }
  }
}

void smpProc::handleCoher(const simAddress baddr,
				  const enum mem_cmd cmd) {
  if (!myMainProc->myP->isCoherent())
    return;

  simAddress blkAddr = getBAddr(baddr);
  blkTag &bTag = blkTags[blkAddr];
  //printf("%d: smp cplx mem access baddr:%x(tag %d) cmd: %d\n", getMainProcID(), baddr, bTag, cmd);

  busMemAccess++;

  if (cmd == Read) { // read miss
    //place read miss on bus
    myMainProc->myP->postMessage(proc::READ_MISS, blkAddr, myMainProc);
    bTag = SHARED; //change tag to SHARED    
  } else { 
    // write backs hanlded here
    bTag = SHARED;
    // write misses, taken care of in noteWrite()
  }
}
