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

#include <sst_config.h>
//#include "DownStreamAdapterTest.h"
#include "sst/elements/SysC/common/TLManouncer.h"

#include <sst/core/component.h>
#include <sst/core/element.h>
#include <sst/core/params.h>

#include <sst/elements/memHierarchy/memEvent.h>



using namespace SST;
using namespace SST::SysC;

void TLMAnouncerWrapper::setup(){BaseType_t::setup();}
void TLMAnouncerWrapper::finish(){BaseType_t::finish();}
bool TLMAnouncerWrapper::Status(){return BaseType_t::Status();}
void TLMAnouncerWrapper::init(uint8_t _ip){BaseType_t::init(_ip);}
TLMAnouncerWrapper::TLMAnouncerWrapper(SST::ComponentId_t _id,
                                       SST::Params& _params)
: BaseType_t(_id,_params)
{
  anouncer=new TLMAnouncer("TLMAnouncer");
  sysc_adapter->getSocket().bind(anouncer->socket);
}

