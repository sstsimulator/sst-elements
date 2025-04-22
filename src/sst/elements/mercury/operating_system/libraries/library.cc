// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/params.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/libraries/library.h>
#include <mercury/hardware/common/flow.h>
#include <mercury/common/thread_lock.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;
extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

static thread_lock the_api_lock;

void
apiLock() {
  the_api_lock.lock();
}

void
apiUnlock() {
  the_api_lock.unlock();
}

Library::~Library()
{
}

SST::Hg::SoftwareId
Library::sid() const {
  return api_parent_app_->sid();
}

NodeId
Library::addr() const {
  return api_parent_app_->os()->addr();
}

Thread*
Library::activeThread()
{
  return api_parent_app_->os()->activeThread();
}

void
Library::startLibraryCall()
{
  activeThread()->startLibraryCall();
}
void
Library::endLibraryCall()
{
  activeThread()->endLibraryCall();
}

Timestamp
Library::now() const 
{
  return api_parent_app_->os()->now();
}

void
Library::schedule(Timestamp t, ExecutionEvent* ev)
{
  api_parent_app_->os()->sendExecutionEvent(t, ev);
}

void
Library::scheduleDelay(TimeDelta t, ExecutionEvent* ev)
{
  api_parent_app_->os()->sendDelayedExecutionEvent(t, ev);
}

Library::Library(SST::Params & params, App *parent) :
  api_parent_app_(parent)
{ }

} // end namespace Hg
} // end namespace SST
