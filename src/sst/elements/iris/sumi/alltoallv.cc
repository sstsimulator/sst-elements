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

#include <iris/sumi/alltoallv.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
//#include <sprockit/output.h>
#include <cstring>

#define divide_by_2_round_up(x) ((x/2) + (x%2))

#define divide_by_2_round_down(x) (x/2)

//using namespace sprockit::dbg;

#define SEND_SHUFFLE 0
#define RECV_SHUFFLE 1

#define MAX_UNROLL =

namespace SST::Iris::sumi {

void
DirectAlltoallvActor::initBuffers()
{
  total_send_size_ = 0;
  total_recv_size_ = 0;
  for (int i=0; i < dom_nproc_; ++i){
    total_send_size_ += send_counts_[i];
    total_recv_size_ += recv_counts_[i];
  }
  recv_buffer_ = result_buffer_;
}

void
DirectAlltoallvActor::finalizeBuffers()
{
}

void
DirectAlltoallvActor::addAction(
  const std::vector<Action*>& actions,
  int stride_direction,
  int num_initial,
  int stride)
{
  int partner = (dom_me_ + dom_nproc_ + stride*stride_direction) % dom_nproc_;
  Action* ac = actions[partner];
  if (stride < num_initial){
    DagCollectiveActor::addAction(ac);
  } else {
    int prev_partner = (partner + dom_nproc_ - num_initial*stride_direction) % dom_nproc_;
    Action* prev = actions[prev_partner];
    addDependency(prev, ac);
  }
}

void
DirectAlltoallvActor::initDag()
{
  std::vector<Action*> recvs(dom_nproc_);
  std::vector<Action*> sends(dom_nproc_);

  RecvAction::buf_type_t recv_ty = slicer_->contiguous() ?
        RecvAction::in_place : RecvAction::unpack_temp_buf;

  int send_offset = 0;
  int recv_offset = 0;
  int round = 0;
  for (int i=0; i < dom_nproc_; ++i){
    Action* recv = new RecvAction(round, i, recv_ty);
    Action* send = new SendAction(round, i, SendAction::in_place);
    send->offset = send_offset;
    send->nelems = send_counts_[i];
    recv->offset = recv_offset;
    recv->nelems = recv_counts_[i];

    sends[i] = send;
    recvs[i] = recv;
  }

  int num_initial = 3;
  for (int i=0; i < dom_nproc_; ++i){
    //move down for recvs
    addAction(recvs, -1, num_initial, i);
    //move down for sends
    addAction(sends, 1, num_initial, i);
  }

}

void
DirectAlltoallvActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
DirectAlltoallvActor::finalize()
{
}


}
