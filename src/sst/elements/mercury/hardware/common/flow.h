// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

#include <sst/core/event.h>
#include <mercury/common/node_address.h>
#include <mercury/common/request.h>

namespace SST {
namespace Hg {

class Flow : public Request
{
 public:
  /**
   * Virtual function to return size. Child classes should impement this
   * if they want any size tracked / modeled.
   * @return Zero size, meant to be implemented by children.
   */
  uint64_t byteLength() const {
    return byte_length_;
  }

  ~Flow() override{}

  void setFlowId(uint64_t id) {
    flow_id_ = id;
  }

  uint64_t flowId() const {
    return flow_id_;
  }

  std::string libname() const {
    return libname_;
  }

  void setFlowSize(uint64_t sz) {
    byte_length_ = sz;
  }

  void
  serialize_order(Core::Serialization::serializer& ser) override
  {
    Event::serialize_order(ser);
    ser & flow_id_;
    ser & byte_length_;
    ser & libname_;
  }

 protected:
  Flow(uint64_t id, uint64_t size, const std::string& libname = "") :
    flow_id_(id), byte_length_(size), libname_(libname)
  {
  }

  uint64_t flow_id_;
  uint64_t byte_length_;
  std::string libname_;

};

} // end namespace Hg
} // end namespace SST
