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

#pragma once

#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <sst/core/timeLord.h>
#include <mercury/common/events.h>
#include <mercury/common/timestamp.h>
#include <cstdint>

namespace SST {
namespace Hg {

static std::string _tick_spacing_string_("1ps");

template <class CoreBase>
class HgBase : public CoreBase
{
public:

HgBase(uint32_t id) :
    CoreBase(id)
  {
    static int selfid = 0;
    if (!time_converter_){
      time_converter_ = CoreBase::getTimeConverter(_tick_spacing_string_);
    }
    self_link_ = CoreBase::configureSelfLink("HgComponent" + std::to_string(self_id()), time_converter_,new SST::Event::Handler<HgBase>(this, &HgBase::handleExecutionEvent));
    ++selfid;

    RankInfo num_ranks = CoreBase::getNumRanks();
    nthread_ = num_ranks.thread;
    RankInfo rank = CoreBase::getRank();
    thread_id_ = rank.thread;
  }

 int self_id();

 int nthread() const {
   return nthread_;
 }

 int threadId() const {
   return thread_id_;
 }

 SST::SimTime_t getCurrentSimTime(SST::TimeConverter* tc) const {
   return CoreBase::getCurrentSimTime(tc);
 }

 void sendDelayedExecutionEvent(TimeDelta delay, ExecutionEvent* ev){
   self_link_->send(SST::SimTime_t(delay), time_converter_, ev);
 }

 void sendExecutionEventNow(ExecutionEvent* ev){
   self_link_->send(ev);
 }

 void sendExecutionEvent(Timestamp arrival, ExecutionEvent* ev){
   SST::SimTime_t delay = arrival.time.ticks() - getCurrentSimTime(time_converter_);
   self_link_->send(delay, time_converter_, ev);
 }

 Timestamp now() const {
   SST::SimTime_t nowTicks = getCurrentSimTime(time_converter_);
   return Timestamp(uint64_t(0), uint64_t(nowTicks));
 }

 void handleExecutionEvent(Event* ev){
   std::cerr << "event address: " << ev << std::endl;
   ExecutionEvent* sev = dynamic_cast<ExecutionEvent*>(ev);
   sev->execute();
   delete sev;
 }

 static SST::TimeConverter* timeConverter() {
   return time_converter_;
 }

protected:

 static SST::TimeConverter* time_converter_;

private:
 int thread_id_;
 int nthread_;
  SST::Link* self_link_;
};

template <typename CoreBase>
SST::TimeConverter* HgBase<CoreBase>::time_converter_ = nullptr;

class Component : public HgBase<SST::Component>
{
protected:
  Component(uint32_t id) :
    HgBase(id){ }
};

class SubComponent : public HgBase<SST::SubComponent>
{
protected:
  SubComponent(uint32_t id) :
    HgBase(id){ }
};

} // end of namespace Hg
} // end of namespace SST
