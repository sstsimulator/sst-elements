/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

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
