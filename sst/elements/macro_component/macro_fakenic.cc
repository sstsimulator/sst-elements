/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */
#include "sst_config.h"
#include "macro_processor.h"

#include "macro_fakenic.h"
#include "sstMessageEvent.h"

#include <sstmac/common/stats/stats_common.h>

#include <sstmac/common/errors.h>

// Pointer types.

//using namespace sstmac::hw;

//
// Hi.
//
macro_fakenic::macro_fakenic(const sstmac::hw::node::ptr &parent, macro_processor* op,
    const sstmac::hw::interconnect::ptr &interconn, long nodenum) :
    sstmac::hw::networkinterface(parent, interconn), core_parent_(op)

{
	if (!spy_)
	{
		spy_ = sstmac::stat_spyplot::construct("<stats> spyplot",
									   "network_spyplot.csv");
		register_stat(spy_);
		
		spy_bytes_ = sstmac::stat_spyplot::construct("<stats> spyplot", "network_spyplot_bytes.csv");
		register_stat(spy_bytes_);
	}
}

//
// Hello.
//
macro_fakenic::ptr
macro_fakenic::construct(const sstmac::hw::node::ptr &parent, macro_processor* op,
    const sstmac::hw::interconnect::ptr &interconn, long nodenum)
{
  return macro_fakenic::ptr(new macro_fakenic(parent, op, interconn, nodenum));
}


//
// Schedule a send operation to happen at the given time.
//
void
macro_fakenic::send(const sstmac::nodeaddress::ptr &recver,
    const sstmac::sst_message::ptr &payload, const sstmac::eventhandler::ptr &completion)
{
  SSTMAC_DEBUG << "macro_fakenic::send(" << recver << ", "
      << payload->to_string() << ", " << completion << ")\n";

  if (payload == 0)
    {
      throw sstmac::nullerror("macro_fakenic::send: null payload pointer.");
    }

  sstMessageEvent* msg = new sstMessageEvent();

  msg->bytes_ = payload->get_byte_length();
  msg->toaddr_ = payload->toaddr_;
  msg->fromaddr_ = payload->fromaddr_;
  msg->data_ = payload;

  SSTMAC_DEBUG << "FakeNic(" << core_parent_->myid_->to_string() << "): sending to "
      << payload->toaddr_->to_string() << "\n";

  core_parent_->outgate->Send(msg);

}

sstmac::hw::networkinterface::ptr
macro_fakenic::clone(const sstmac::hw::node::ptr &parent,
    const sstmac::hw::interconnect::ptr &interconn) const
{
  throw sstmac::unimplementederror("macro_fakenic::clone(...) should not be called");
}

void
macro_fakenic::handle(const sstmac::sst_message::ptr &msg)
{
  SSTMAC_DEBUG << "macro_fakenic::handle - received message \n";

  parent_->handle(msg);
}
