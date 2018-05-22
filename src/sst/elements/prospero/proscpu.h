// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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

#include "sst/core/elementinfo.h"
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
  void init(unsigned int phase);
  void finish();

  SST_ELI_REGISTER_COMPONENT(
        ProsperoComponent,
        "prospero",
        "prosperoCPU",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Prospero CPU Memory Trace Replay Engine",
        COMPONENT_CATEGORY_PROCESSOR
   )

   SST_ELI_DOCUMENT_PARAMS(
	{ "verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0" },
    	{ "cache_line_size", "Sets the length of the cache line in bytes, this should match the L1 cache", "64" },
    	{ "reader",  "The trace reader module to load", "prospero.ProsperoTextTraceReader" },
    	{ "pagesize", "Sets the page size for the Prospero simple virtual memory manager", "4096"},
    	{ "clock", "Sets the clock of the core", "2GHz"} ,
    	{ "max_outstanding", "Sets the maximum number of outstanding transactions that the memory system will allow", "16"},
    	{ "max_issue_per_cycle", "Sets the maximum number of new transactions that the system can issue per cycle", "2"},
   )

   SST_ELI_DOCUMENT_PORTS(
	{ "cache_link", "Link to the memHierarchy cache", { "memHierarchy.memEvent", "" } }
   )

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
