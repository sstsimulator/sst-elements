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

#ifndef _ariel_H
#define _ariel_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include "arielcore.h"

using namespace std;

namespace SST {
namespace ArielComponent {

class Ariel : public SST::Component {
public:

  Ariel(SST::ComponentId_t id, SST::Params& params);

  void setup()  { }
  void finish();

  void handleEvent(SST::Event* event);
private:
  Ariel();  // for serialization only
  Ariel(const Ariel&); // do not implement
  void operator=(const Ariel&); // do not implement

  virtual bool tick( SST::Cycle_t );
  int create_pinchild(char* prog_binary, char** arg_list);

  uint64_t max_inst;
  char* named_pipe;
  int* pipe_id;
  std::string user_binary;
  Output* output;

  ArielCore** cores;
  uint32_t core_count;
  SST::Link** cache_link;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

}
}
#endif /* _ariel_H */
