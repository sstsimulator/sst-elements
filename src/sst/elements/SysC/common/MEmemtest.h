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

#ifndef MEMEMTEST_H
#define MEMEMTEST_H
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/elements/memHierarchy/memEvent.h>

namespace SST{
class Link;
namespace SysC{
//////////////////////////////////////////////////////////////////////////////
class MEMemTest: public SST::Component{
 protected:
  typedef MEMemTest               ThisType_t;
  typedef SST::Component                 BaseType_t;
  typedef uint64_t                  Address_t;
  typedef SST::MemHierarchy::MemEvent    Event_t;
  typedef SimTime_t                      Time_t;
  typedef SST::Link                 Link_t;
  typedef Event_t::dataVec          Vector_t;
  typedef SST::MemHierarchy::Command Command_t;
  typedef uint32_t                    Size_t;

 public:
  void setup();
  void finish();
  bool Status();
  MEMemTest(SST::ComponentId_t _id,Params& _params);
 private:
  void handleEvent(SST::Event *_event);
  //read data from event
  void readData(Event_t* _ev,Address_t _address);
  //write data to event
  void writeData(Event_t* _ev,Address_t _address);
  bool tic(SimTime_t _cycle);
  void printData();
  
  Link_t *link;
  uint8_t* data;
  uint8_t* buffer;
  uint8_t** old_buffers;
  uint32_t cycles;
  uint32_t num_cycles;
  uint32_t payload_size;
  uint32_t data_size;
  uint32_t max_index;
  Command_t last_cmd;
  Address_t last_address;
  Address_t *old_addresses;
  bool first_loop_done;
  uint8_t index;
};
}//namespace SysC
}//namespace SST
#endif //MEMEMTEST_H
