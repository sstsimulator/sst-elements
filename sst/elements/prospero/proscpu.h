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

#ifndef _SST_PROSPERO_H
#define _SST_PROSPERO_H

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/event.h"
#include "sst/core/sst_types.h"
#include "sst/core/component.h"
#include "sst/core/link.h"

namespace SST {
namespace Prospero {

class ProsperoComponent : public SST::Component {
public:

  ProsperoComponent(SST::ComponentId_t id, SST::Params& params);
  ~ProsperoComponent();

  void setup() { }
  void finish() { }

private:
  ProsperoComponent();                         // Serialization only
  ProsperoComponent(const ProsperoComponent&); // Do not impl.
  void operator=(const ProsperoComponent&);    // Do not impl.

  void handleResponse( SST::Event *ev );
  bool tick( SST::Cycle_t );

  ProsperoTraceReader* reader;
  ProsperoTraceEntry* currentEntry;
  SimpleMem* cache_link;
  FILE* traceFile;
#ifdef HAVE_LIBZ
  gzFile traceFileZ;
#endif
  uint64_t pageSize;
  uint64_t cacheLineSize;
  uint32_t maxOutstanding;
  uint32_t currentOutstanding;

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

#endif /* _SST_PROSPERO_H */
