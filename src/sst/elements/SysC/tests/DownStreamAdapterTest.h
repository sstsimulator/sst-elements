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

#ifndef TESTS_DOWNSTREAMADAPTERTEST_H
#define TESTS_DOWNSTREAMADAPTERTEST_H
#include <sst/core/sst_types.h>
#include "sst/elements/SysC/common/TLManouncer.h"

#include <sst/core/component.h>
//#include <sst/core/element.h>
//#include <sst/core/params.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include "sst/elements/SysC/TLM/memEventAdapterFunctions.h"
#include "sst/elements/SysC/TLM/adapter.h"
class TLMAnnouncer;
namespace SST{
class Params;
namespace SysC{

class TLMAnnouncerWrapper: public DownStreamAdapter<SST::MemHierarchy::MemEvent> {
 protected:
  typedef TLMAnnouncerWrapper          ThisType_t;
  typedef ThisType_t SC_CURRENT_USER_MODULE;
  typedef SST::MemHierarchy::MemEvent      Event_t;
  typedef DownStreamAdapter<Event_t>  BaseType_t;
  void setup(){BaseType_t::setup();}
  void finish(){anouncer->printData();BaseType_t::finish();}
  bool Status(){return BaseType_t::Status();}
  void init(uint8_t _ip){BaseType_t::init(_ip);}
 public:
  TLMAnnouncerWrapper(SST::ComponentId_t _id,
                     SST::Params& _params)
      : BaseType_t(_id,_params)
  {
    anouncer=new TLMAnnouncer("TLMAnnouncer");
    sysc_adapter->getSocket().bind(anouncer->socket);
  }
 private:
  TLMAnnouncer* anouncer;
};

}//namespace SysC
}//namespace SST
#endif //TESTS_DOWNSTREAMADAPTERTEST_H
