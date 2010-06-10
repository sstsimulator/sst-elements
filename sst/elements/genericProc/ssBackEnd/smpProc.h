// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007,2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SMPPROC_H
#define SMPPROC_H

#include "ssb.h"
#include "ssb_memory.h"
#include <map>

using namespace std;

class mainProc;

//: Conventinoal SMP processor
class smpProc {
  typedef enum {INVALID, SHARED, EXCLUSIVE} blkTag;
  typedef map<simAddress, blkTag> tagMap;
  tagMap blkTags;
  void writeBackBlock(simAddress);
  unsigned long long invalidates[2];
  unsigned long long busMemAccess;
  unsigned long long writeBacks;
  mainProc *myMainProc;
  simAddress getBAddr(simAddress a);
public:
  smpProc(mainProc *mp);
  //virtual unsigned getFEBDelay() {return 8;}
  virtual void noteWrite(const simAddress a);
  virtual void busReadMiss(simAddress);
  virtual void busWriteMiss(simAddress);
  virtual void busWriteHit(simAddress);
  virtual void finish();

  virtual void handleCoher(const simAddress, const enum mem_cmd cmd);
};

#endif
