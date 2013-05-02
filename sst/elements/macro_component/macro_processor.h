// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MACRO_PROCESSOR_H
#define _MACRO_PROCESSOR_H

#include "macro_address.h"
#include "macro_fakeeventmanager.h"

#include <sstmac/software/ami/environment.h>

#include <sstmac/common/debug.h>

#include <sstmac/common/sim_parameters.h>

#include <sstmac/common/messages/sst_message.h>
#include <sstmac/software/process/softwareid.h>

#include <sstmac/common/ptr_type.h>

#include <sstmac/hardware/node/node.h>

#include <sstmac/hardware/network/congestion/interconnect.h>

#include "sstMessageEvent.h"

//#include <sst/core/event.h>
#include <sst/core/sst_types.h>
//#include        <sst/core/serialization/element.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/element.h>

#include <queue>

class macro_processor : public SST::Component, public eventmanager_interface
{


public:

  sstmac::hw::node::ptr node_;
  friend class macro_fakenic;
  //sstcore_fakenic::ptr nic_;

  static sstmac::debug dbg_;
  static bool debug_init_;

  virtual const sstmac::timestamp&
  now();

protected:
	
	static bool nodeid_init_;
	sstmac::timestamp now_;

  SST::Link* outgate;
  SST::Link* self_proc_link_;
	
	static bool timeinit_;

  int coresPerProc;

	sstmac::nodeaddress::ptr myid_;

  fakeeventmanager::ptr fem_;

  bool
  isMasterCore()
  {
    return (myid_->unique_id() == 0);
  }
	
	friend class fake_interconnect;
	
	class fake_interconnect : public sstmac::hw::interconnect {
		
	protected:
		fake_interconnect(macro_processor* p) : sstmac::hw::interconnect(sstmac::hw::topology::ptr()),
		parent_(p){
			
		}
		
		macro_processor* parent_;
		
	public:
		typedef boost::intrusive_ptr<fake_interconnect> ptr;
		
		
		static ptr
		construct(macro_processor* p){
			return ptr(new fake_interconnect(p));
			
		}
		
		virtual std::string
		to_string() const {
			return "fake_interconnect";
			
		}
		
		virtual
		~fake_interconnect() {
			
		}
		
		
		/// The maximum number of nodes on this network.
		/// Return a negative (really <= 0) value to indicate an infinite capacity.
		virtual int64_t
		slots() const {
			std::cout << ";akjf;akjdf;kajd;fkjadsf\n";
			return 10;
		}
		
		/// Send the given message at the given time.
		/// Notify the sendhandler when the send is complete.
		/// Notify the recvhandler when the recv is complete.
		virtual void
		send(const sstmac::timestamp &start_time, const sstmac::sst_message::ptr &payload){	
			parent_->fem_->update(parent_->now());

			
			sstMessageEvent* msg = new sstMessageEvent();
			
			msg->bytes_ = payload->get_byte_length();
			msg->toaddr_ = payload->toaddr();
			msg->fromaddr_ = payload->fromaddr();
			msg->data_ = payload;
			
			sstmac::timestamp ts = parent_->now();
		
			double d = (start_time - ts).psec();
			SST::SimTime_t delay(d);
						
			parent_->outgate->send(delay, msg); 
			
						
		}
		
		
	};
	
	friend class fake_interconnect;

public:

  macro_processor(SST::ComponentId_t id, SST::Component::Params_t& params);
  virtual
  ~macro_processor()
  {
  }
  void setup();
  void finish()  
  {
	  std::cout << "---proc finished \n";
  }

  virtual SST::Link*
    get_self_event_link(){
    return self_proc_link_;
  }



private:
//  friend class boost::serialization::access;

// macro_processor() :
//   Component(-1) {} // for serialization only
// macro_processor(const macro_processor&); // do not implement
// void
// operator=(const macro_processor&); // do not implement

  void
  handleEvent(SST::Event *ev);

  void
  handle_proc_event(SST::Event *ev);

  //BOOST_SERIALIZATION_SPLIT_MEMBER()

};

#endif /* _SIMPLECOMPONENT_H */
