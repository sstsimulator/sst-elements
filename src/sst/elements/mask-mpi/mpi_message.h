/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

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

//#include <sstmac/common/sstmac_config.h>
#include <mpi_status.h>
#include <mpi_types/mpi_type.h>
#include <mpi_integers.h>
#include <mercury/common/timestamp.h>
#include <mercury/operating_system/process/task_id.h>
#include <mercury/operating_system/process/app_id.h>
#include <mercury/components/operating_system_fwd.h>
#include <iris/sumi/message.h>
#include <iris/sumi/collective_message.h>
#include <mercury/common/thread_safe_new.h>
#include <mercury/hardware/network/network_message.h>
#include <sst/core/serialization/serializer.h>

#pragma once

namespace SST::MASKMPI {

class MpiMessage final :
  public SST::Iris::sumi::ProtocolMessage,
  public SST::Hg::thread_safe_new<MpiMessage>
{
  ImplementSerializable(MpiMessage)

 public:
  template <class... Args>
  MpiMessage(int src_rank, int dst_rank,
              MPI_Datatype type, int tag, MPI_Comm commid, int seqnum,
              int count, int type_size, void* partner_buf, int protocol,
              Args&&... args) :
    Iris::sumi::ProtocolMessage(count, type_size, partner_buf, protocol,
                           std::forward<Args>(args)...),
    src_rank_(src_rank),
    dst_rank_(dst_rank),
    type_(type),
    tag_(tag),
    commid_(commid),
    seqnum_(seqnum),
    min_quiesce_(SST::Hg::Timestamp::max())
  {
  }

  std::string toString() const override;

  ~MpiMessage() throw () override;

  SST::MASKMPI::MpiMessage* clone_me() const {
    MpiMessage* cln = new MpiMessage(*this);
    return cln;
  }

  SST::Hg::NetworkMessage* cloneInjectionAck() const override {
    auto* msg = clone_me();
    msg->convertToAck();
    return msg;
  }

  void serialize_order(Core::Serialization::serializer& ser) override;

  MPI_Datatype type() const {
    return type_;
  }

  int tag() const {
    return tag_;
  }

  MPI_Comm comm() const {
    return commid_;
  }

  int seqnum() const {
    return seqnum_;
  }

  int srcRank() const {
    return src_rank_;
  }

  int dstRank() const {
    return dst_rank_;
  }

  void setMinQuiesce(SST::Hg::Timestamp ticks){
    min_quiesce_ = ticks;
  }

  SST::Hg::Timestamp minQuiesce() const {
    return min_quiesce_;
  }

  void buildStatus(MPI_Status* stat) const;

 private:
  MpiMessage(){} //for serialization

  int src_rank_;
  int dst_rank_;
  MPI_Datatype type_;
  int tag_;
  MPI_Comm commid_;
  int seqnum_;
  SST::Hg::Timestamp min_quiesce_;

};

}
