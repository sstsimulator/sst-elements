/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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
