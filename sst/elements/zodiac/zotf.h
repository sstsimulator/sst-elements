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


#ifndef _ZODIAC_OTF_TRACE_READER_H
#define _ZODIAC_OTF_TRACE_READER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "stdio.h"
#include "stdlib.h"
#include <sst/elements/hermes/msgapi.h>

#include "otfreader.h"

using namespace std;
using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacOTFTraceReader : public SST::Component {
public:

  ZodiacOTFTraceReader(SST::ComponentId_t id, SST::Params& params);
  void setup() { }
  void finish() { 
  }

private:
  ~ZodiacOTFTraceReader();
  ZodiacOTFTraceReader();  // for serialization only
  ZodiacOTFTraceReader(const ZodiacOTFTraceReader&); // do not implement
  void operator=(const ZodiacOTFTraceReader&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );

  ////////////////////////////////////////////////////////

  MessageInterface* msgapi;
  OTFReader* reader;
  std::queue<ZodiacEvent*>* eventQ;

  ////////////////////////////////////////////////////////

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}

#endif /* _ZODIAC_TRACE_READER_H */
