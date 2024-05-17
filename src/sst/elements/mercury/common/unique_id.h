// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

#include <sstream>
#include <stdint.h>
#include <mercury/common/serializable.h>

namespace SST {
namespace Hg {

struct UniqueEventId {
  uint32_t src_node;
  uint32_t msg_num;

  UniqueEventId(uint32_t src, uint32_t num) :
    src_node(src), msg_num(num) {
  }

  UniqueEventId() :
    src_node(-1), msg_num(0) {
  }

  operator uint64_t() const {
    //map onto single 64 byte number
    uint64_t lo = msg_num;
    uint64_t hi = (((uint64_t)src_node)<<32);
    return lo | hi;
  }

  static void unpack(uint64_t id, uint32_t& node, uint32_t& num){
    num = id; //low 32
    node = (id>>32); //high 32
  }

  std::string toString() const {
    std::stringstream sstr;
    sstr << "unique_id(" << src_node << "," << msg_num << ")";
    return sstr.str();
  }

  void setSrcNode(uint32_t src) {
    src_node = src;
  }

  void setSeed(uint32_t seed) {
    msg_num = seed;
  }

  UniqueEventId& operator++() {
    ++msg_num;
    return *this;
  }

  UniqueEventId operator++(int) {
    UniqueEventId other(*this);
    ++msg_num;
    return other;
  }
};

} // end namespace Hg
} // end namespace SST

START_SERIALIZATION_NAMESPACE
template <>
class serialize<SST::Hg::UniqueEventId>
{
 public:
  void
  operator()(SST::Hg::UniqueEventId& id, serializer& ser){
    ser.primitive(id);
  }
};
END_SERIALIZATION_NAMESPACE

namespace std {
template <>
struct hash<SST::Hg::UniqueEventId>
  : public std::hash<uint64_t>
{ };
}

