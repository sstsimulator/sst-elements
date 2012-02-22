// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MACRO_FAKEEVENTMANAGER_H
#define _MACRO_FAKEEVENTMANAGER_H


#include <sstmac/common/eventmanager.h>

#include <sst/core/sst_types.h>
#include        <sst/core/serialization/element.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

class eventmanager_interface
{
public:
  virtual SST::Link*
  get_self_event_link() = 0;

  virtual sstmac::timestamp
  now() = 0;

  virtual ~eventmanager_interface() { }
};

class ScheduleEvent : public SST::Event
{
public:
  sstmac::eventhandler::ptr handler_;
  sstmac::sst_message::ptr msg_;

  ScheduleEvent() :
      SST::Event()
  {
  }

  virtual void
  print(const std::string& header) const
  {
    std::cout << header << "macro_processor::ScheduleEvent " << " with message "
        << msg_->to_string() << std::endl;
  }

};

class fakeeventmanager : public sstmac::eventmanager
{
  eventmanager_interface *parent_;

  fakeeventmanager(eventmanager_interface *p) :
      parent_(p)
  {

  }

public:

  static boost::intrusive_ptr<fakeeventmanager>
  construct(eventmanager_interface *p)
  {
    boost::intrusive_ptr<fakeeventmanager> ret(new fakeeventmanager(p));
    return ret;
  }

  virtual void
  clear(const sstmac::timestamp &zero_time = sstmac::timestamp(0))
  {

  }

  virtual void
  run()
  {

  }

  virtual void
  step()
  {

  }

  virtual const sstmac::timestamp&
  now()
  {
    now_ = parent_->now();
    return now_;
  }

  /// Set off the given eventhandler at the given time.
  virtual void
  schedule(const sstmac::timestamp &start_time,
      const sstmac::eventhandler::ptr &handler,
      const sstmac::sst_message::ptr &msg)
  {
    ScheduleEvent *evt = new ScheduleEvent();
    evt->handler_ = handler;
    evt->msg_ = msg;

    if (!parent_)
      {
        throw sstmac::ssterror("fakeeventmanager::schedule - parent not set");
      }

    if (start_time.psec() < parent_->now().psec())
      {
        throw sstmac::valueerror(
            "fakeeventmanager: tried to schedule to the past. doh!");
      }

    SST::SimTime_t delay = start_time.psec() - parent_->now().psec();

    parent_->get_self_event_link()->Send(delay, evt);

  }
};

#endif
