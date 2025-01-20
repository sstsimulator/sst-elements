// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <sst/core/params.h>
#include <sst/core/event.h>
#include <mercury/common/component.h>
#include <mercury/common/event_link.h>

#define Connectable_type_invalid(ty) \
   spkt_throw_printf(sprockit::value_error, "invalid Connectable type %s", Connectable::str(ty))

#define connect_str_case(x) case x: return #x

namespace SST {
namespace Hg {

class Connectable {
 public:
  static const int any_port = -1;

  /**
   * @brief connectOutput
   * Invoked by interconnect setup routine to notify device
   * that all payloads sent out on given port should be sent to given payload handler
   * @param src_outport
   * @param dst_inport
   * @param payloadHandler
   */
  virtual void connectOutput(int src_outport, int dst_inport, EventLink::ptr&& payload_link) = 0;

  /**
   * @brief connectInput
   * Invoked by interconnect setup routine to notify device
   * that all credits sent back to a given port should be sent to given credit handler
   * @param src_outport
   * @param dst_inport
   * @param creditHandler Can be null, if no credits are ever sent
   */
  virtual void connectInput(int src_outport, int dst_inport, EventLink::ptr&& credit_link) = 0;

  /**
   * @brief creditHandler
   * @param port
   * @return Can be null, if no credits are ever to be received
   */
  virtual SST::Event::HandlerBase* creditHandler(int port) = 0;

  /**
   * @brief payloadHandler
   * @param port
   * @return A new handler for incoming payloads on the given port
   */
  virtual SST::Event::HandlerBase* payloadHandler(int port) = 0;

};

class ConnectableComponent :
  public Component,
  public Connectable
{
  public:
    void initOutputLink(int src_outport, int dst_inport);
    void initInputLink(int src_outport, int dst_inport);

  protected:
    ConnectableComponent(uint32_t cid, SST::Params& params);
};

class ConnectableSubcomponent :
  public SubComponent,
  public Connectable
{
 protected:
  ConnectableSubcomponent(uint32_t id, const std::string& selfname, SST::Component* parent)
    : SubComponent(id)
  {
  }

  void initOutputLink(int src_outport, int dst_inport);
  void initInputLink(int src_outport, int dst_inport);

};


} // end of namespace Hg
} // end of namespace SST
