// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ZODIAC_TRACE_READER_H
#define _ZODIAC_TRACE_READER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/elements/hermes/msgapi.h>

using namespace SST::Hermes;
using namespace SST::Hermes::MP;

namespace SST {
namespace Zodiac {

class ZodiacTraceReader : public SST::Component {
public:

  ZodiacTraceReader(SST::ComponentId_t id, SST::Params& params);
  void setup() { }
  void finish() { }

private:
  ZodiacTraceReader();  // for serialization only
  ZodiacTraceReader(const ZodiacTraceReader&); // do not implement
  void operator=(const ZodiacTraceReader&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );

  MP::Interface* msgapi;

};

}
}

#endif /* _ZODIAC_TRACE_READER_H */
