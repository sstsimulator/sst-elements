// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SST_PROSPERO_H
#define _SST_PROSPERO_H

#include "sst/core/output.h"
#include "sst/core/component.h"
#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/event.h"
#include "sst/core/sst_types.h"
#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/interfaces/simpleMem.h"

#include "prosreader.h"
#include "prosmemmgr.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

using namespace SST;
using namespace SST::Interfaces;

namespace SST {
namespace Prospero {

class ProsperoComponent : public Component {
public:

  ProsperoComponent(ComponentId_t id, Params& params);
  ~ProsperoComponent();

  void setup() { }
  void finish();

private:
  ProsperoComponent();                         // Serialization only
  ProsperoComponent(const ProsperoComponent&); // Do not impl.
  void operator=(const ProsperoComponent&);    // Do not impl.

  void handleResponse( SimpleMem::Request* ev );
  bool tick( Cycle_t );
  void issueRequest(const ProsperoTraceEntry* entry);

  Output* output;
  ProsperoTraceReader* reader;
  ProsperoTraceEntry* currentEntry;
  ProsperoMemoryManager* memMgr;
  SimpleMem* cache_link;
  FILE* traceFile;
  bool traceEnded;
#ifdef HAVE_LIBZ
  gzFile traceFileZ;
#endif
  uint64_t pageSize;
  uint64_t cacheLineSize;
  uint32_t maxOutstanding;
  uint32_t currentOutstanding;
  uint32_t maxIssuePerCycle;

  uint64_t readsIssued;
  uint64_t writesIssued;
  uint64_t splitReadsIssued;
  uint64_t splitWritesIssued;
  uint64_t totalBytesRead;
  uint64_t totalBytesWritten;
  uint64_t cyclesWithIssue;
  uint64_t cyclesWithNoIssue;

};

}
}

#endif /* _SST_PROSPERO_H */
