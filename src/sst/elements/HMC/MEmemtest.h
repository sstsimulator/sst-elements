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

#ifndef MEGENERATOR_H
#define MEGENERATOR_H
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>
/*#include <sst_config.h>

#include <sst/core/element.h>
#include <sst/core/params.h>

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
//#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"


#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;
*/
#define PAYLOAD_SIZE 32
namespace SST{
namespace SysC{
//////////////////////////////////////////////////////////////////////////////
class MEGenerator: public SST::Component{
 protected:
  typedef MEGenerator               ThisType_t;
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
  MEGenerator(SST::ComponentId_t _id,Params& _params);
 private:
  void handleEvent(SST::Event *_event);
  //read data from event
  void readData(Event_t* _ev,Address_t _address);
  //write data to event
  void writeData(Event_t* _ev,Address_t _address);
  bool tic(SimTime_t _cycle);
  void printData();
  
  Link_t *link;
  int data[PAYLOAD_SIZE];
  uint32_t cycles;
  uint32_t num_cycles;
};
}//namespace SysC
}//namespace SST
#endif //MEGENERATOR_H
