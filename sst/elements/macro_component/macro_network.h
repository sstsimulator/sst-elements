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

#ifndef _macro_network_H
#define _macro_network_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sstmac/hardware/network/congestion/interconnect.h>
#include <sstmac/hardware/network/topology/topology.h>
#include <sstmac/common/messages/sst_message.h>

#include "macro_fakeeventmanager.h"
#include "sstMessageEvent.h"
#include "macro_address.h"

class macro_network : public SST::Component, public eventmanager_interface
{

  friend class message_sent_handler;

  class message_sent_handler : public sstmac::eventhandler
  {
    macro_network* parent_;

    message_sent_handler(macro_network* p) :
        parent_(p)
    {
    }

  public:
    typedef boost::intrusive_ptr<message_sent_handler> ptr;

    virtual void
    handle(const sstmac::sst_message::ptr &msg)
    {
      //sstmac::sst_message::ptr sstm = msg->clone(sstmac::sst_message::SEND_ACK);
      sstMessageEvent *ack = new sstMessageEvent();
      ack->data_ = msg;
      ack->toaddr_ = msg->fromaddr_;
      ack->fromaddr_ = msg->toaddr_;
      ack->bytes_ = 1;

      macro_address::ptr madd = boost::dynamic_pointer_cast<macro_address>(
          msg->fromaddr_);
      if(!madd){
          std::cout << "address memory addr: " << msg->fromaddr_ << "\n";
          std::cout << "addr: " << msg->fromaddr_->to_string() << "\n";
          throw sstmac::ssterror("macro_network::message_sent_handler::handle - can't convert address to macro_address");
      }
      int portnum = madd->unique_id();

      if(parent_->ports_.find(portnum) == parent_->ports_.end()){
          std::cout << "portnum: " << portnum << "\n";
          throw sstmac::ssterror("macro_network::message_sent_handler::handle - port not found");
      }

      parent_->ports_[portnum]->Send(ack);
    }

    virtual std::string
    toString() const
    {
      return "macro_network::message_sent_handler";
    }

    static message_sent_handler::ptr
    construct(macro_network* p)
    {
      message_sent_handler::ptr ret(new message_sent_handler(p));
      return ret;
    }

  };

  friend class message_recv_handler;

  class message_recv_handler : public sstmac::eventhandler
  {
    macro_network* parent_;

    message_recv_handler(macro_network* p) :
        parent_(p)
    {
    }

  public:
    typedef boost::intrusive_ptr<message_recv_handler> ptr;

    virtual void
    handle(const sstmac::sst_message::ptr &msg)
    {

      sstMessageEvent *done = new sstMessageEvent();
      done->data_ = msg;
      done->toaddr_ = msg->toaddr_;
      done->fromaddr_ = msg->fromaddr_;
      done->bytes_ = msg->get_byte_length();

      macro_address::ptr madd = boost::dynamic_pointer_cast<macro_address>(
          msg->toaddr_);
      int portnum = madd->unique_id();

      parent_->ports_[portnum]->Send(done);
    }

    virtual std::string
    toString() const
    {
      return "macro_network::message_recv_handler";
    }

    static message_recv_handler::ptr
    construct(macro_network* p)
    {
      message_recv_handler::ptr ret(new message_recv_handler(p));
      return ret;
    }

  };

//  sstmac::eventhandler::ptr sent_handler_;
//  sstmac::eventhandler::ptr recv_handler_;
  fakeeventmanager::ptr fem_;

public:

  macro_network(SST::ComponentId_t id, SST::Component::Params_t& params);
  virtual
  ~macro_network()
  {
  }
  int
  Setup()
  {
    return 0;
  }
  int
  Finish()
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
    return 0;
  }

  virtual sstmac::timestamp
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

  std::map<int, SST::Link*> ports_;

  sstmac::hw::interconnect::ptr intercon_;
  sstmac::hw::topology::ptr topology_;

  SST::Link* self_proc_link_;

};

#endif /* _macro_network_H */
