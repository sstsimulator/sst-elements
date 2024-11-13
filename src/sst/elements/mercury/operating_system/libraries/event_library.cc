// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mercury/components/operating_system.h>
#include <mercury/operating_system/libraries/event_library.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

EventLibrary::EventLibrary(const std::string& libname, SoftwareId sid, OperatingSystem* os) :
  os_(os),
  sid_(sid), 
  addr_(os->addr()),
  libname_(libname) 
{
  os_->registerEventLib(this);
}

EventLibrary::~EventLibrary()
{
  os_->unregisterEventLib(this);
}

void
EventLibrary::incomingEvent(Event*  /*ev*/)
{
  sst_hg_throw_printf(SST::Hg::UnimplementedError,
    "%s::incomingEvent: this library should only block, never receive incoming",
     toString().c_str());
}

void
EventLibrary::incomingRequest(Request*  /*ev*/)
{
  sst_hg_throw_printf(SST::Hg::UnimplementedError,
    "%s::incomingRequest: this library should only block, never receive incoming",
     toString().c_str());
}

} // end namespace Hg
} // end namespace SST
