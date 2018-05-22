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

#ifndef COMMON_MEMORYWEDGE_H
#define COMMON_MEMORYWEDGE_H
#include <sst/core/sst_types.h>

#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <string>

namespace SST{
class Link;
class Params;
namespace MemHierarchy{
class MemEvent;
}//namespace MemHierarchy;

class MemoryWedge 
: public SST::Component
{
 private:
  typedef MemoryWedge                   ThisType;
  typedef Component                     BaseType;
 protected:
  typedef SST::Event                    Event_t;
  typedef SST::MemHierarchy::Addr       Address_t;
  typedef SST::Event::Handler<ThisType> Handler_t;
 public:
  MemoryWedge(SST::ComponentId_t _id, SST::Params& _params);
  void setup();//Children need to call MemoryWedge::setup
  void finish();//Children need to call MemoryWedge::finish
 private:
  void handleEventFromMemory(SST::Event* _ev);
  void handleEventFromUpstream(SST::Event* _ev);
 protected:
  virtual void load(Event_t *_event)=0;
  virtual void store(Event_t *_event)=0;
  virtual void readFromFile(std::string _filename)=0;
  virtual void writeToFile(std::string _filename)=0;
 protected:
  SST::Output out;
  Link* memory;
  Link* upstream;
 private:
  std::string load_filename;
  std::string write_filename;
};
#define WEDGE_LOADFILE "load_filename"
#define WEDGE_STOREFILE "write_filename"

#define WEDGE_PARAMS                                                          \
{WEDGE_LOADFILE,                                                              \
  "STR: filename to load starting memory from"                                \
},                                                                            \
{WEDGE_STOREFILE,                                                             \
  "STR: filename to store ending memory to"                                   \
}

}//namespace SST
#endif //COMMON_MEMORYWEDGE_H
