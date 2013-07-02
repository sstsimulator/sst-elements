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

#ifndef _macro_network_H
#define _macro_network_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sstmac/hardware/network/interconnect/interconnect.h>
#include <sstmac/hardware/network/topology/topology.h>
#include <sstmac/common/messages/sst_message.h>

#include "macro_fakeeventmanager.h"
#include "sstMessageEvent.h"
#include "macro_address.h"

class macro_network : public SST::Component, public eventmanager_interface
{

/*  friend class message_recv_handler;

  class message_recv_handler : public sstmac::eventhandler
  {
    macro_network* parent_;
	  sstmac::nodeaddress::ptr addr_;
	  
	  message_recv_handler(macro_network* p, sstmac::nodeaddress::ptr a) :
        parent_(p), addr_(a)
    {
    }

  public:
    typedef boost::intrusive_ptr<message_recv_handler> ptr;

    virtual void
    handle(const sstmac::sst_message::ptr &msg)
    {

      sstMessageEvent *done = new sstMessageEvent();
      done->data_ = msg;
      done->toaddr_ = msg->toaddr();
      done->fromaddr_ = msg->fromaddr();
      done->bytes_ = msg->get_byte_length();

     // macro_address::ptr madd = boost::dynamic_pointer_cast<macro_address>(
     //     msg->toaddr_);
      int portnum = addr_->unique_id();

	//	std::cout << "sending message to port " << portnum << "\n";
      parent_->ports_[portnum]->send(done); 
    }

    virtual std::string
    to_string() const
    {
      return "macro_network::message_recv_handler";
    }

    static message_recv_handler::ptr
	  construct(macro_network* p, sstmac::nodeaddress::ptr a)
    {
      message_recv_handler::ptr ret(new message_recv_handler(p, a));
      return ret;
    }

  };*/
	
//	boost::unordered_map<long, message_recv_handler::ptr> recvhandlers_;

//  sstmac::eventhandler::ptr sent_handler_;
//  sstmac::eventhandler::ptr recv_handler_;
  fakeeventmanager::ptr fem_;

public:

  macro_network(SST::ComponentId_t id, SST::Component::Params_t& params);
  virtual
  ~macro_network()
  {
  }
  void setup()  
  {
  }
  void finish() 
  {
    static int n = 0;
    n++;
    if (n == 10)
      {
        printf("Several Simple Components Finished\n");
      }
    else if (n > 10)
      {
        ;
      }
    else
      {
        printf("Simple Component Finished\n");
      }
  }

  virtual const sstmac::timestamp&
  now();

  virtual SST::Link*
  get_self_event_link()
  {
    return self_proc_link_;
  }

private:
  // friend class boost::serialization::access;

  // macro_network() :
  //   Component(-1) {} // for serialization only
  // macro_network(const macro_network&); // do not implement
  // void
  // operator=(const macro_network&); // do not implement

  void
  handle_proc_event(SST::Event *ev);

  void
  handleEvent(SST::Event *ev);
  // virtual bool
  // clockTic(SST::Cycle_t);

  int num_ports_;

  //std::map<int, SST::Link*> ports_;

  sstmac::hw::interconnect::ptr intercon_;
  //sstmac::hw::topology::ptr topology_;

  SST::Link* self_proc_link_;
	
	//sstmac::eventhandler::ptr recv_;
	
	sstmac::timestamp now_;

};

#endif /* _macro_network_H */
