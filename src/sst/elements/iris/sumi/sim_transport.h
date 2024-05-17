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

//#include <sstmac/common/stats/stat_spyplot_fwd.h>
//#include <sstmac/common/event_scheduler_fwd.h>
//#include <sstmac/software/launch/task_mapping.h>

#include <output.h>

#include <sst/core/eli/elementbuilder.h>

#include <mercury/operating_system/libraries/api.h>
#include <mercury/operating_system/libraries/service.h>
#include <mercury/operating_system/process/progress_queue.h>
#include <mercury/hardware/network/network_message_fwd.h>
#include <mercury/components/node_fwd.h>
#include <mercury/components/operating_system.h>
#include <mercury/common/errors.h>
#include <mercury/common/factory.h>
#include <mercury/common/util.h>

#include <iris/sumi/message_fwd.h>
#include <iris/sumi/collective.h>
#include <iris/sumi/comm_functions.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/collective.h>
#include <iris/sumi/comm_functions.h>
#include <iris/sumi/options.h>
#include <iris/sumi/communicator_fwd.h>
#include <iris/sumi/null_buffer.h>

#include <unordered_map>
#include <queue>

namespace SST::Iris::sumi {

class QoSAnalysis {

 public:
  SST_ELI_DECLARE_BASE(QoSAnalysis)
  SST_ELI_DECLARE_DEFAULT_INFO()
  SST_ELI_DECLARE_CTOR(SST::Params&)

  QoSAnalysis(SST::Params& /*params*/){}

  virtual ~QoSAnalysis(){}

  virtual int selectQoS(Message* m) = 0;

  virtual void logDelay(SST::Hg::TimeDelta delay, Message* m) = 0;

};

class SimTransport : public Transport, public SST::Hg::API {

 public:
  SST_ELI_REGISTER_DERIVED(
    API,
    SimTransport,
    "hg",
    "SimTransport",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the SUMI transport API")

  using DefaultProgressQueue = SST::Hg::MultiProgressQueue<Message>;

  SimTransport(SST::Params& params, SST::Hg::App* parent, SST::Component* comp);

  SST::Hg::SoftwareId sid() const {
    return Transport::sid();
  }

  void init() override;

  void finish() override;

  ~SimTransport() override;

  Output output;

  SST::Hg::NodeId rankToNode(int rank) const override {

    // Use logical IDs and let merlin remap
    return SST::Hg::NodeId(rank);

    // other ways it might work some day
    //return rank_mapper_->rankToNode(rank);
    //return os_->rankToNode(rank);
  }

  /**
   * @brief smsg_send_response After receiving a short message m, use that message object to return a response
   * This function "reverses" the sender/recever
   * @param m
   * @param loc_buffer
   * @param local_cq
   * @param remote_cq
   * @return
   */
  void smsgSendResponse(Message* m, uint64_t sz, void* loc_buffer, int local_cq, int remote_cq, int qos) override;

  /**
   * @brief rdma_get_response After receiving a short message payload coordinating an RDMA get,
   * use that message object to send an rdma get
   * @param m
   * @param loc_buffer
   * @param remote_buffer
   * @param local_cq
   * @param remote_cq
   * @return
   */
  void rdmaGetRequestResponse(Message* m, uint64_t sz, void* loc_buffer, void* remote_buffer,
                                 int local_cq, int remote_cq, int qos) override;

  void rdmaGetResponse(Message* m, uint64_t sz, int local_cq, int remote_cq, int qos) override;

  /**
   * @brief rdma_put_response
   * @param m
   * @param loc_buffer
   * @param remote_buffer
   * @param local_cq
   * @param remote_cq
   * @return
   */
  void rdmaPutResponse(Message* m, uint64_t payload_bytes,
             void* loc_buffer, void* remote_buffer, int local_cq, int remote_cq, int qos) override;

  double wallTime() const override {
    return now().sec();
  }

  SST::Hg::Timestamp now() const override;

  void* allocateWorkspace(uint64_t size, void* parent) override;

  void freeWorkspace(void *buf, uint64_t size) override;

  void memcopy(void* dst, void* src, uint64_t bytes) override;

  void memcopyDelay(uint64_t bytes) override;

  int* nidlist() const override;

  void incomingEvent(SST::Event *ev);

  void compute(SST::Hg::TimeDelta t);

  void incomingMessage(Message* msg);

  void shutdownServer(int dest_rank, SST::Hg::NodeId dest_node, int dest_app);

  void pinRdma(uint64_t bytes);

  void configureNextPoll(bool& blocking, double& timeout){
    if (pragma_block_set_){
      blocking = true;
      pragma_block_set_ = false;
      timeout = pragma_timeout_;
    }
  }
  
  /**
   Check if a message has been received on a specific completion queue.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param blocking Whether to block until a message is received
   @param cq_id The specific completion queue to check
   @param timeout  An optional timeout - only valid with blocking
   @return    The next message to be received, null if no messages
  */
  Message* poll(bool blocking, int cq_id, double timeout = -1) override {
    configureNextPoll(blocking, timeout);
    return default_progress_queue_.find(cq_id, blocking, timeout);
  }

  /**
   Check all completion queues if a message has been received.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param blocking Whether to block until a message is received
   @param timeout  An optional timeout - only valid with blocking
   @return    The next message to be received, null if no messages
  */
  Message* poll(bool blocking, double timeout = -1) override {
    configureNextPoll(blocking, timeout);
    return default_progress_queue_.find_any(blocking, timeout);
  }

  void setPragmaBlocking(bool cond, double timeout = -1){
    pragma_block_set_ = cond;
    pragma_timeout_ = timeout;
  }

  /**
   Block until a message is received.
   Returns immediately if message already waiting.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param cq_id The specific completion queue to check
   @param timeout   Timeout in seconds
   @return          The next message to be received. Message is NULL on timeout
  */
  Message* blockingPoll(int cq_id, double timeout = -1) override {
    return poll(true, cq_id, timeout);
  }

  int allocateCqId() override {
    if (free_cq_ids_.empty()){
      int id = completion_queues_.size();
      completion_queues_.emplace_back();
      return id;
    } else {
      int id = free_cq_ids_.front();
      free_cq_ids_.pop();
      return id;
    }
  }

  void deallocateCq(int id) override {
    free_cq_ids_.push(id);
  }

  int allocateDefaultCq() override {
    int id = allocateCqId();
    allocateCq(id, std::bind(&DefaultProgressQueue::incoming,
                          &default_progress_queue_,
                          id, std::placeholders::_1));
    return id;
  }

  void allocateCq(int id, std::function<void(Message*)>&& f);

 private:      
  void send(Message* m) override;

  uint64_t allocateFlowId() override;

  std::vector<std::function<void(Message*)>> completion_queues_;

  std::function<void(Message*)> null_completion_notify_;

  SST::Hg::TimeDelta post_rdma_delay_;

  SST::Hg::TimeDelta post_header_delay_;
  SST::Hg::TimeDelta poll_delay_;

//  sstmac::StatSpyplot<int,uint64_t>* spy_bytes_;

  SST::Hg::TimeDelta rdma_pin_latency_;
  SST::Hg::TimeDelta rdma_page_delay_;
  int page_size_;
  bool pin_delay_;

 protected:
  void registerNullHandler(std::function<void(Message*)> f){
    null_completion_notify_ = f;
  }

  bool smp_optimize_;

  std::map<int,std::list<Message*>> held_;

  std::queue<int> free_cq_ids_;

  SST::Hg::App* parent_app_;

  DefaultProgressQueue default_progress_queue_;

  std::function<void(SST::Hg::NetworkMessage*)> nic_ioctl_;

  QoSAnalysis* qos_analysis_;

 private:
  bool pragma_block_set_;

  double pragma_timeout_;

  SST::Hg::OperatingSystem* os_;

  void drop(Message*){}
};


class TerminateException : public std::exception
{
};

} // end namespace SST::Iris::sumi
