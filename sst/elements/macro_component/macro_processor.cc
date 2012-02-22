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

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>
#include "macro_parameters.h"

#include "sstMessageEvent.h"

#include "sst/core/element.h"

#include "macro_processor.h"

#include <sstmac/hardware/node/simplenode.h>
#include <sstmac/common/driver_util.h>
#include <sstmac/software/services/launch/instantlaunch.h>

#include "sstMessageEvent.h"
#include "macro_network.h"

#include "macro_fakenic.h"

#include "sst/core/simulation.h"
#include "sst/core/timelord.h"

#include <sstmac/common/basicstringtokenizer.h>

using namespace SST;
using namespace std;
using namespace sstmac;
using namespace sstmac::sw;
using namespace sstmac::hw;

debug macro_processor::dbg_;
bool macro_processor::debug_init_ = false;

macro_processor::macro_processor(ComponentId_t id, Params_t& params) :
    Component(id)
{

  if (!dbg_.is_real())
    {
      dbg_ = debug("<debug> processor | proc | node");
      logger::register_logger(&dbg_);
    }

  myid_ = macro_address::construct(id);

  std::cout << "constructing macro_processor " << id << " with address "
      << myid_->to_string() << "\n";

  outgate = configureLink("nic",
      new Event::Handler<macro_processor>(this, &macro_processor::handleEvent));
  std::cout << "outgate is " << outgate << "\n";

  if (!debug_init_)
    {
      if (params.find("logging_string") != params.end())
        {
          string dbg_input = params["logging_string"];

          std::deque<std::string> tok;

          std::string space = ",";
          pst::BasicStringTokenizer::tokenize(dbg_input, tok, space);

          string debug_str = "";

          std::deque<std::string>::iterator it, end = tok.end();
          bool first = true;
          for (it = tok.begin(); it != end; it++)
            {
              if ((*it) == "timestamps")
                {
                  if (first)
                    {
                      debug_str = *it;
                      first = false;
                    }
                  else
                    debug_str = debug_str + " | " + *it;
                }
              else if (first)
                {
                  debug_str = "<debug> " + *it;
                  first = false;
                }
              else
                debug_str = debug_str + " | <debug> " + *it;
            }

          std::cout << "debug string: " << debug_str << "\n";
          logger::set_user_param(debug_str);
          debug_init_ = true;

        }
    }

  fem_ = fakeeventmanager::construct(this);

  logger::timer_ = fem_;

  macro_parameters::ptr macroparams = macro_parameters::construct(params);


  self_proc_link_ = configureSelfLink(
      "self_proc_link",
      new Event::Handler<macro_processor>(this,
          &macro_processor::handle_proc_event));

  std::cout << "self_proc_link is " << self_proc_link_ << "\n";
	
	long nodenum = macroparams->get_long_param("node_num");

  node_ = sstmac::hw::simplenode::construct(macroparams);
  macro_fakenic::ptr nic = macro_fakenic::construct(node_, this,
      sstmac::hw::interconnect::ptr(), nodenum);
  node_->set_nic(nic);
  nic->set_eventmanager(fem_);

  registerTimeBase("1 ps", true);
	
	// tell the simulator not to end without us
	registerExit();
}

int
macro_processor::Setup()
{

  node_->initialize(myid_, fem_);

  return 0;
}

void
macro_processor::handle_proc_event(Event *evt)
{
  SSTMAC_DEBUG << "macro_procesor: handling proc event: \n";
  if (dbg_.is_active())
    {
      evt->print("macro_processor:");
    }
  ScheduleEvent *event = dynamic_cast<ScheduleEvent*>(evt);
  event->handler_->handle(event->msg_);
  delete event;
}

// incoming events are scanned and deleted
void
macro_processor::handleEvent(Event *evt)
{
  //printf("recv\n");
  SSTMAC_DEBUG << "macro_procesor: handling event: \n";
  if (dbg_.is_active())
    {
      evt->print("macro_processor:");
    }
  sstMessageEvent *event = dynamic_cast<sstMessageEvent*>(evt);
  if (event)
    {

      node_->handle(event->data_);

      delete event;
    }
  else
    {
      printf("Error! Bad Event Type!\n");
    }
}

timestamp
macro_processor::now()
{
  SimTime_t current_time = getCurrentSimTime();
  return timestamp::exact_psec(current_time);
}

