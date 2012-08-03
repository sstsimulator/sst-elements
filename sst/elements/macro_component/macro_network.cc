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

using namespace SST;
using namespace std;

using namespace sstmac::hw;
using namespace sstmac::sw;
using namespace sstmac;

macro_network::macro_network(ComponentId_t id, Params_t& params) :
    Component(id)
{

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
  registerExit();

 
  self_proc_link_ = configureSelfLink(
       "self_proc_link",
       new Event::Handler<macro_network>(this,
           &macro_network::handle_proc_event));


  Event::Handler<macro_network> *h = new Event::Handler<macro_network>(this,
      &macro_network::handleEvent);

	//sstmac::eventhandler::ptr sent = message_sent_handler::construct(this);
	sstmac::eventhandler::ptr recv = message_recv_handler::construct(this);
	
  for (int i = 0; i < num_ports_; i++)
    {
      stringstream ss;
      ss << "port" << i;
      ports_[i] = configureLink(ss.str(), h);

      assert(ports_[i]);
		
		intercon_->set_endpoint(macro_address::construct(i), recv);

    }

  registerTimeBase("1 ps", true);
	
}

void
macro_network::handle_proc_event(Event *evt)
{

  ScheduleEvent *event = dynamic_cast<ScheduleEvent*>(evt);
  event->handler_->handle(event->msg_);
  delete event;
}

// incoming events are scanned and deleted
void
macro_network::handleEvent(Event *ev)
{
  sstMessageEvent *event = dynamic_cast<sstMessageEvent*>(ev);
  if (event)
    {
      intercon_->send(now(), event->data_);

      delete event;
    }
  else
    {
      printf("Error! Bad Event Type!\n");
    }
}

timestamp
macro_network::now()
{
  SimTime_t current_time = getCurrentSimTime();
  return timestamp::exact_psec(current_time);
}

