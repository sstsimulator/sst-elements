// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _verona_H
#define _verona_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/memEvent.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>

using namespace std;
using namespace SST::Interfaces;
namespace SST {
namespace Verona {

class VeronaCPU : public SST::Component {
public:

  VeronaCPU(SST::ComponentId_t id, SST::Component::Params_t& params);

  int Setup()  { return 0; }
  int Finish() { return 0; }

  void handleEvent(SST::Event* event);

private:
  VeronaCPU();  // for serialization only
  VeronaCPU(const VeronaCPU&); // do not implement
  void operator=(const VeronaCPU&); // do not implement

  virtual bool tick( SST::Cycle_t );

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
#endif /* _verona_H */
