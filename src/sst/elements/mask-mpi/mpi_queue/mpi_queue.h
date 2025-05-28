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

#include <mpi_message.h>
#include <mpi_request.h>
#include <mpi_comm/mpi_comm.h>
#include <mpi_types/mpi_type.h>

#include <mercury/operating_system/process/progress_queue.h>
//#include <mercury/common/event_scheduler_fwd.h>

#include <sst/core/factory.h>

//#include <mercury/common/event_manager_fwd.h>

#include <mpi_api_fwd.h>
#include <mpi_status_fwd.h>
#include <mpi_protocol/mpi_protocol_fwd.h>

#include <mpi_queue/mpi_queue_recv_request_fwd.h>
#include <mpi_queue/mpi_queue_probe_request.h>

#include <sst/core/params.h>

#include <queue>
#include <mercury/common/timestamp.h>

#pragma once

namespace SST::MASKMPI {

class MpiQueue
{

  /** temporary fix until I figure out a better way to do this */
  friend class MpiProtocol;
  friend class Eager0;
  friend class Eager1;
  friend class RendezvousGet;
  friend class DirectPut;
  friend class MpiQueueRecvRequest;

  using progress_queue = SST::Hg::MultiProgressQueue<SST::Iris::sumi::Message>;

 public:
  MpiQueue(SST::Params& params, int TaskId,
            MpiApi* api, Iris::sumi::CollectiveEngine* engine);

  ~MpiQueue() throw ();

  void init();

  static void deleteStatics();

  void send(MpiRequest* key, int count, MPI_Datatype type,
       int dest, int tag, MpiComm* comm,
       void* buffer);

  void recv(MpiRequest* key, int count, MPI_Datatype type,
       int source, int tag, MpiComm* comm,
       void* buffer = 0);

  void probe(MpiRequest* key, MpiComm* comm,
        int source, int tag);

  bool iprobe(MpiComm* comm, int source, int tag, MPI_Status* stat);

  MpiApi* api() const {
    return api_;
  }

  void memcopy(uint64_t bytes);

  SST::Hg::Timestamp now() const;

  void finalizeRecv(MpiMessage* msg,
                MpiQueueRecvRequest* req);

  SST::Hg::Timestamp progressLoop(MpiRequest* req);

  void nonblockingProgress();

  void startProgressLoop(const std::vector<MpiRequest*>& req);

  void startProgressLoop(const std::vector<MpiRequest*>& req,
                      SST::Hg::TimeDelta timeout);

  void finishProgressLoop(const std::vector<MpiRequest*>& req);

  void forwardProgress(double timeout);

  void bufferUnexpected(MpiMessage* msg);

  void postRdma(MpiMessage* msg,
    bool needs_send_ack,
    bool needs_recv_ack);

  void incomingNewMessage(MpiMessage* Message);

  int pt2ptCqId() const{
    return pt2pt_cq_;
  }

  int collCqId() const {
    return coll_cq_;
  }

 private:
  struct sortbyseqnum {
    bool operator()(MpiMessage* a, MpiMessage*b) const;
  };

  typedef std::set<MpiMessage*, sortbyseqnum> hold_list_t;

 private:
  /**
   * @brief incoming_pt2pt_message Message might be held up due to sequencing constraints
   * @param msg
   */
  void incomingPt2ptMessage(Iris::sumi::Message* msg);

  /**
   * @brief handle_pt2pt_message Message is guaranteed to satisfy sequencing constraints
   * @param msg
   */
  void handlePt2ptMessage(MpiMessage* msg);

  void incomingCollectiveMessage(Iris::sumi::Message* Message);

  void incomingMessage(Iris::sumi::Message* Message);

  void notifyProbes(MpiMessage* Message);

  MpiMessage* findMatchingRecv(MpiQueueRecvRequest* req);
  MpiQueueRecvRequest* findMatchingRecv(MpiMessage* msg);

  void clearPending();

  bool atLeastOneComplete(const std::vector<MpiRequest*>& req);

 private:
  /// The sequence number for our next outbound transmission.
  std::unordered_map<TaskId, int> next_outbound_;

  /// The sequence number expected for our next inbound transmission.
  std::unordered_map<TaskId, int> next_inbound_;

  /// Hold messages that arrived out of order.
  std::unordered_map<TaskId, hold_list_t> held_;

  /// Inbound messages waiting for a matching receive request.
  std::list<MpiMessage*> need_recv_match_;
  std::list<MpiQueueRecvRequest*> need_send_match_;

  std::vector<MpiProtocol*> protocols_;

  /// Probe requests watching
  std::list<SST::MASKMPI::mpi_queue_probe_request*> probelist_;

  progress_queue queue_;

  TaskId taskid_;

  AppId appid_;

  MpiApi* api_;

  int max_vshort_msg_size_;
  int max_eager_msg_size_;
  bool use_put_window_;

  int pt2pt_cq_;
  int coll_cq_;

};

}

//#define mpi_queue_action_debug(rank, ...) \
//  mpi_debug(rank, sprockit::dbg::mpi_queue, \
//   " [queue] %s", sprockit::sprintf(__VA_ARGS__).c_str())

////for local use in mpi queue object
//#define mpi_queue_debug(...) \
//  mpi_queue_action_debug(int(taskid_), __VA_ARGS__)

//#define mpi_queue_protocol_debug(...) \
//  mpi_queue_action_debug(mpi_->rank(), __VA_ARGS__)


//#endif
