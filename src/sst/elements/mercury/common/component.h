/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#pragma once

#include <sst_element_config.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <common/events.h>
#include <common/timestamp.h>
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
//    ++selfid;
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
