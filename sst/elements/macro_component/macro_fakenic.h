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

#ifndef _MACRO_FAKENIC_H_INCLUDED
#define _MACRO_FAKENIC_H_INCLUDED


#include <sstmac/hardware/nic/networkinterface.h>

class macro_processor;

/**
 * A private nested type to handle sending data.
 */
class macro_fakenic : public sstmac::hw::networkinterface
{

protected:
  /// Hi.
  macro_fakenic(const sstmac::hw::node::ptr &parent, macro_processor* op,
      const sstmac::hw::interconnect::ptr &interconn, long nodenum);

  macro_processor* core_parent_;

public:
  typedef boost::intrusive_ptr<macro_fakenic> ptr;

  /// Hello.
  static ptr
  construct(const sstmac::hw::node::ptr &parent, macro_processor* op,
      const sstmac::hw::interconnect::ptr &interconn, long nodenum);

  /// Goodbye.
  virtual
  ~macro_fakenic() throw () { }

  /// Schedule a send operation to happen at the given time.
  /// \param start_time is the time when the send is started.
  /// \param ppidarg is the parallel process id of the source and target.
  /// \param recver is the global index of the receiver node.
  /// \param payload is the data sent over the network
  /// \param completion is the eventhandler signaled when the send is complete
  virtual void
  send(const sstmac::nodeaddress::ptr &recver,
      const sstmac::sst_message::ptr &payload,
      const eventhandler::ptr &completion);

  virtual networkinterface::ptr
  clone(const sstmac::hw::node::ptr &parent,
      const sstmac::hw::interconnect::ptr &interconn) const;

  virtual void
  handle(const sstmac::sst_message::ptr &msg);


};

#endif
