// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MACRO_FAKEEVENTMANAGER_H
#define _MACRO_FAKEEVENTMANAGER_H

#include <sstmac/common/errors.h>
#include <sstmac/common/eventmanager.h>

#include <sst/core/sst_types.h>
//#include        <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

class eventmanager_interface
{
public:
  virtual SST::Link*
  get_self_event_link() = 0;

  virtual const sstmac::timestamp&
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

class ScheduleEvent2 : public SST::Event
{
public:
	sstmac::event::ptr event_;
	
	ScheduleEvent2() :
	SST::Event()
	{
	}
	
	virtual void
	print(const std::string& header) const
	{
		std::cout << header << "macro_processor::ScheduleEvent2 " << " with event "
        << event_->to_string() << std::endl;
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
	typedef boost::intrusive_ptr<fakeeventmanager> ptr;

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
  register_finish(boost::intrusive_ptr<sstmac::ptr_type>){
	
  }

	virtual void
	cancel_all_messages(const sstmac::eventmodulePtr& mod){
		
	}
	
	
  virtual void
  run()
  {

  }

  virtual void
  step()
  {

  }
	
	void
	update(const sstmac::timestamp &t){
		set_now(t);
	}
	
	virtual void
	schedule(const sstmac::timestamp& start_time,
             const sstmac::event::ptr& event){
		
		 ScheduleEvent2 *evt = new ScheduleEvent2();
		evt->event_ = event;
		
		if (!parent_)
		{
			//sst_throw(sstmac::ssterror, "fakeeventmanager::schedule - parent not set");
			throw sstmac::ssterror("fakeeventmanager::schedule - parent not set");
		}
		
		SST::SimTime_t delay = start_time.psec() - parent_->now().psec();
		
		if (start_time.psec() < parent_->now().psec())
		{
			std::cout << "now: " << parent_->now() << "\n";
			std::cout << "start time: " << start_time << "\n";
			
			//exit(1);
			
			// sst_throw(sstmac::sst_error, "doh");
			//  sstmac::sst_throw(sstmac::sst_error,
			//    "fakeeventmanager: now=%s, tried to schedule to %s. doh!", parent_->now().pse(), start_time.psec());
			throw sstmac::ssterror("doh");
			// delay = 0;
		}
		
		//  std::cout << "fakeeventmanager: scheudling message with delay " << delay << "\n";
		
		
		
		parent_->get_self_event_link()->send(delay, evt); 
		
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
		  //sst_throw(sstmac::ssterror, "fakeeventmanager::schedule - parent not set");
		  throw sstmac::ssterror("fakeeventmanager::schedule - parent not set");
      }

	   SST::SimTime_t delay = start_time.psec() - parent_->now().psec();
	  
    if (start_time.psec() < parent_->now().psec())
      {
		  std::cout << "now: " << parent_->now() << "\n";
		  std::cout << "start time: " << start_time << "\n";
		  
		  //exit(1);
		  
		 // sst_throw(sstmac::sst_error, "doh");
		//  sstmac::sst_throw(sstmac::sst_error,
        //    "fakeeventmanager: now=%s, tried to schedule to %s. doh!", parent_->now().pse(), start_time.psec());
		  throw sstmac::ssterror("doh");
		 // delay = 0;
      }
	  
	//  std::cout << "fakeeventmanager: scheudling message with delay " << delay << "\n";

   

    parent_->get_self_event_link()->send(delay, evt); 

  }
};

#endif
