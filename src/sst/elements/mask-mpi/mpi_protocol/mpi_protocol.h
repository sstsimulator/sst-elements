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

#include <string>
#include <map>
#include <mpi_queue/mpi_queue_fwd.h>
#include <mpi_api_fwd.h>
#include <mpi_queue/mpi_queue_recv_request_fwd.h>
#include <mpi_request_fwd.h>
#include <mpi_message.h>
#include <sst/core/params.h>
#include <mercury/common/timestamp.h>

#pragma once

namespace SST::MASKMPI {

/**
 * @brief The mpi_protocol class
 */
class MpiProtocol : public SST::Hg::printable {

 public:
  enum PROTOCOL_ID {
    EAGER0=0,
    EAGER1=1,
    RENDEZVOUS_GET=2,
    DIRECT_PUT=3,
    NUM_PROTOCOLS
  };

 public:
  virtual void start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* typeobj,
                     int tag, MPI_Comm comm, int seq_id, MpiRequest* req) = 0;

  virtual void incoming(MpiMessage* msg) = 0;

  virtual void incoming(MpiMessage *msg, MpiQueueRecvRequest* req) = 0;

  virtual ~MpiProtocol(){}

 protected:
  MpiProtocol(SST::Params& params, MpiQueue* queue);

  MpiQueue* queue_;

  MpiApi* mpi_;

  void* fillSendBuffer(int count, void *buffer, MpiType *typeobj);

  void logRecvDelay(int stage, SST::Hg::TimeDelta timeSinceQuiesce,
                    MpiMessage* msg, MpiQueueRecvRequest* req);
};

/**
 * @brief The eager0 class
 * Basic eager protocol. Sender copies into a temp buf before sending.
 * MPI_Send completes immediately. Assumes dest has allocated "mailbox"
 * temp buffers to receive unexpected messages.
 */
class Eager0 final : public MpiProtocol
{
 public:
  Eager0(SST::Params& params, MpiQueue* queue);

  ~Eager0(){}

  std::string toString() const override {
    return "eager0";
  }

  void start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* typeobj,
             int tag, MPI_Comm comm, int seq_id, MpiRequest* req) override;

  void incoming(MpiMessage *msg) override;

  void incoming(MpiMessage *msg, MpiQueueRecvRequest* req) override;

 private:
  std::map<uint64_t,void*> send_flows_;
  int qos_;

};

/**
 * @brief The eager1 class
 * The eager1 protocol uses RDMA to complete the operation, but
 * copies into buffers to allow send/recv to complete faster.
 * On MPI_Send, the source copies into a temp buffer.
 * It then sends an RDMA header to the dest.
 * Whenever the header arrives - even if before MPI_Recv is posted,
 * the dest allocates and temp buffer and then posts an RDMA get
 * from temp buf into temp buf.  On MPI_Recv (or completion of transfer),
 * the final result is copied from temp into recv buf and temp bufs are freed.
 */
class Eager1 final : public MpiProtocol
{
 public:
  Eager1(SST::Params& params, MpiQueue* queue);

  virtual ~Eager1(){}

  std::string toString() const override {
    return "eager1";
  }

  void start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* typeobj,
             int tag, MPI_Comm comm, int seq_id, MpiRequest* req) override;

  void incoming(MpiMessage *msg) override;

  void incoming(MpiMessage *msg, MpiQueueRecvRequest* req) override;

 private:
  void incomingAck(MpiMessage* msg);
  void incomingHeader(MpiMessage* msg);
  void incomingPayload(MpiMessage* msg);

  int header_qos_;
  int rdma_get_qos_;
  int ack_qos_;

};

class RendezvousProtocol : public MpiProtocol
{
 public:
  RendezvousProtocol(SST::Params& params, MpiQueue* queue);

  std::string toString() const override {
    return "rendezvous";
  }

  virtual ~RendezvousProtocol(){}

 protected:
  bool software_ack_;
  int header_qos_;
  int rdma_get_qos_;
  int ack_qos_;
  SST::Hg::Timestamp lastQuiesce_;
  SST::Hg::Timestamp minPartnerQuiesce_;

  SST::Hg::TimeDelta sync_byte_delay_catch_up_cutoff_;
  SST::Hg::TimeDelta quiesce_byte_delay_catch_up_cutoff_;
  int catch_up_qos_;

  void newOutstanding();
  void finishedOutstanding();

 private:
  int numOutstanding_;
};


/**
 * @brief The rendezvous_get class
 * Encapsulates a rendezvous protocol. On MPI_Send, the source sends an RDMA header
 * to the destination. On MPI_Recv, the destination then posts an RDMA get.
 * Hardware acks arrive at both dest/source signaling done.
 */
class RendezvousGet final : public RendezvousProtocol
{
 public:
  RendezvousGet(SST::Params& params, MpiQueue* queue) :
    RendezvousProtocol(params, queue)
  {
  }

  ~RendezvousGet();

  void start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* type,
             int tag, MPI_Comm comm, int seq_id, MpiRequest* req) override;

  void incoming(MpiMessage *msg) override;

  void incoming(MpiMessage *msg, MpiQueueRecvRequest* req) override;

  std::string toString() const override {
    return "rendezvous protocol rdma";
  }

 private:
  struct send {
    MpiRequest* req;
    void* original;
    void* temporary;
    send(MpiRequest* r, void*  /*o*/, void* t) :
      req(r), original(0), temporary(t){}
  };

  void incomingAck(MpiMessage* msg);
  void incomingHeader(MpiMessage* msg);
  void incomingPayload(MpiMessage* msg);
  void* configureSendBuffer(int count, void* buffer, MpiType* obj);

  std::map<uint64_t,MpiQueueRecvRequest*> recv_flows_;

  std::map<uint64_t,send> send_flows_;

};

/**
 * @brief The rendezvous_get class
 * Encapsulates a rendezvous protocol. On MPI_Send, the source sends an RDMA header
 * to the destination. On MPI_Recv, the destination then posts an RDMA get.
 * Hardware acks arrive at both dest/source signaling done.
 */
class DirectPut final : public MpiProtocol
{
 public:
  DirectPut(SST::Params& params, MpiQueue* queue) :
    MpiProtocol(params, queue)
  {
  }

  ~DirectPut();

  void start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* type,
             int tag, MPI_Comm comm, int seq_id, MpiRequest* req) override;

  void incoming(MpiMessage *msg) override;

  void incoming(MpiMessage *msg, MpiQueueRecvRequest* req) override;

  std::string toString() const override {
    return "direct rdma PUT";
  }

 private:
  void incomingAck(MpiMessage* msg);
  void incomingPayload(MpiMessage* msg);

  std::map<uint64_t,MpiRequest*> send_flows_;
};

}
