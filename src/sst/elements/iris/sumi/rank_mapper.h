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
*/

// NOTE: currently the merlin rank mapping capability is used
//   which allows the mercury environment to just use logical
//   node IDs. As such, this rank mapper is currently unused
//   but could be used in the future. -- JPK 8/22/2024

#pragma once

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "sst/core/serialization/serializable.h"

namespace SST::Iris::sumi {

using AppId = int;
using NodeId = uint32_t;

class RankMapping : public SST::Core::Serialization::serializable{
public:
  RankMapping(AppId aid) : aid_(aid) {}

  using ptr = std::shared_ptr<RankMapping>;

  NodeId rankToNode(int rank) const { return rank_to_node_indexing_[rank]; }

  const std::list<int> &nodeToRanks(int node) const {
    return node_to_rank_indexing_[node];
  }

  AppId aid() const { return aid_; }

  int numRanks() const { return rank_to_node_indexing_.size(); }

  int nproc() const { return rank_to_node_indexing_.size(); }

  const std::vector<NodeId> &rankToNode() const {
    return rank_to_node_indexing_;
  }

  std::vector<NodeId> &rankToNode() { return rank_to_node_indexing_; }

  const std::vector<std::list<int>> &nodeToRank() const {
    return node_to_rank_indexing_;
  }

  std::vector<std::list<int>> &nodeToRank() { return node_to_rank_indexing_; }

  static const RankMapping::ptr &globalMapping(AppId aid);

  static RankMapping::ptr globalMapping(const std::string &unique_name);

  static void addGlobalMapping(AppId aid, const std::string &unique_name,
                               const RankMapping::ptr &mapping);

  static void removeGlobalMapping(AppId aid, const std::string &name);

  /* Do not use.  For serialization only */
  RankMapping() {}

  void serialize_order(SST::Core::Serialization::serializer& ser) override
  {
  }

  ImplementSerializable(RankMapping);

private:
  AppId aid_;
  std::vector<NodeId> rank_to_node_indexing_;
  std::vector<std::list<int>> node_to_rank_indexing_;
  std::vector<int> core_affinities_;

  static std::vector<int> local_refcounts_;
  static std::vector<RankMapping::ptr> app_ids_launched_;
  static std::map<std::string, RankMapping::ptr> app_names_launched_;

  static void deleteStatics();
};

} // end namespace SST::Iris::sumi
