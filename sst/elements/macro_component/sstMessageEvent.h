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

#ifndef _SSTMESSAGEEVENT_H
#define _SSTMESSAGEEVENT_H

#include <sst/core/event.h>

#include <boost/shared_ptr.hpp>
#include <sstmac/common/nodeaddress.h>
#include <sstmac/common/messages/sst_message.h>

class sstMessageEvent : public SST::Event
{
public:

  sstMessageEvent() :
      SST::Event()
  {
  }

  uint64_t bytes_;
  sstmac::nodeaddress::ptr toaddr_;
  sstmac::nodeaddress::ptr fromaddr_;

  sstmac::sst_message::ptr data_;

  virtual
  ~sstMessageEvent()
  {
  }

  virtual void
  print(const std::string& header) const
  {
    std::cout << header << "sstMessageEvent to be delivered at " << " at address " << toaddr_->to_string() << " from " << fromaddr_->to_string() << std::endl;
  }

};

#endif /* _SIMPLEEVENT_H */
