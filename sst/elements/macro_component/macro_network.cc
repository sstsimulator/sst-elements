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

#include <sstmac/common/driver_util.h>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "macro_network.h"
#include "macro_parameters.h"

#include "sst/core/simulation.h"
#include "sst/core/timeLord.h"

#include <sstmac/common/factories/network_factory.h>

#include <sstmac/backends/native/nodeid.h>

using namespace SST;
using namespace std;

using namespace sstmac::hw;
using namespace sstmac::sw;
using namespace sstmac;

macro_network::macro_network(ComponentId_t id, Params_t& params) :
    Component(id)
{
	
	nodeaddress::ptr dummy = sstmac::native::nodeid::construct(-1);


  macro_parameters::ptr macroparams = macro_parameters::construct(params);
	intercon_ = SSTNetworkFactory::get_network(macroparams->get_param("network_name"), macroparams);

  num_ports_ = intercon_->slots();

  fem_ = fakeeventmanager::construct(this);
  intercon_->set_eventmanager(fem_);

  if(num_ports_ < 0){
      throw sstmac::ssterror("macro_network: you can't specify -1 for the interconnect size");
  }

  std::cout << "network size: " << num_ports_ << "\n";

  // tell the simulator not to end without us
//  //registerExit();  // Renamed Per Issue 70 - ALevine
  //registerAsPrimaryComponent();
  //primaryComponentDoNotEndSim();

 
  self_proc_link_ = configureSelfLink(
       "self_proc_link",
       new Event::Handler<macro_network>(this,
           &macro_network::handle_proc_event));


  Event::Handler<macro_network> *h = new Event::Handler<macro_network>(this,
      &macro_network::handleEvent);

	//sstmac::eventhandler::ptr sent = message_sent_handler::construct(this);
	//recv_ = message_recv_handler::construct(this);
	
  for (int i = 0; i < num_ports_; i++)
    {
      stringstream ss;
      ss << "port" << i;
      ports_[i] = configureLink(ss.str(), h);

      assert(ports_[i]);
		
		//intercon_->set_endpoint(macro_address::construct(SST::ComponentId_t(i)), recv);
		//nodeaddress::ptr n = nodeaddress::convert_from_uniqueid(i);
		//std::cout << "interconnect setting endpoint for " << n->to_string() << "\n";
		//intercon_->set_endpoint(n, recv);

    }

  registerTimeBase("1 ps", true);
	
}

void
macro_network::handle_proc_event(Event *evt)
{
	//std::cout << "macro_network: handle_proc_event \n";
	fem_->update(now());
	ScheduleEvent *event = dynamic_cast<ScheduleEvent*>(evt);
	if(!event){
		ScheduleEvent2 *ev2 = dynamic_cast<ScheduleEvent2*>(evt);
		if(ev2){
			ev2->event_->execute();
		}
	}else{
		event->handler_->handle(event->msg_);
	}
	delete event;
}

// incoming events are scanned and deleted
void
macro_network::handleEvent(Event *ev)
{
	//	std::cout << "macro_network: handleEvent \n";
	fem_->update(now());
  sstMessageEvent *event = dynamic_cast<sstMessageEvent*>(ev);
	//std::cout << "macro_network::handleEvent - from " << event->fromaddr_->to_string() << " message going to " << event->toaddr_->to_string() << "\n";
	if(recvhandlers_.find(event->toaddr_->unique_id()) == recvhandlers_.end()){
		recvhandlers_[event->toaddr_->unique_id()] = message_recv_handler::construct(this, event->toaddr_);
		intercon_->set_endpoint(event->toaddr_, recvhandlers_[event->toaddr_->unique_id()]);
	}
	
	if(recvhandlers_.find(event->fromaddr_->unique_id()) == recvhandlers_.end()){
		recvhandlers_[event->fromaddr_->unique_id()] = message_recv_handler::construct(this, event->fromaddr_);
		intercon_->set_endpoint(event->fromaddr_, recvhandlers_[event->fromaddr_->unique_id()]);
	}
  if (event)
    {
      intercon_->send(now(), event->data_);

      delete event;
    }
  else
    {
      printf("Error! Bad Event Type!\n");
    }
	//recv_->handle(event->data_);
	//delete event;
}

const timestamp&
macro_network::now()
{
	SimTime_t current_time = getCurrentSimTime();
	now_ = timestamp::exact_psec(current_time);

	
	return now_;
}

