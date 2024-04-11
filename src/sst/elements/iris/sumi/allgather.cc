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

#include <iris/sumi/allgather.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
#include <output.h>
#include <cstring>

#define divide_by_2_round_up(x) \
  ((x/2) + (x%2))

#define divide_by_2_round_down(x) \
  (x/2)

//using namespace sprockit::dbg;


namespace SST::Iris::sumi {

void
BruckTree::computeTree(int nproc, int &log2nproc, int &midpoint,
                        int &num_rounds, int &nprocs_extra_round)
{
  int virtual_nproc;
  RecursiveDoubling::computeTree(nproc, log2nproc, midpoint, virtual_nproc);
  nprocs_extra_round = 0;
  num_rounds = log2nproc;
  if (nproc != virtual_nproc){
    --num_rounds;
    //we will have to do an extra exchange in the last round
    nprocs_extra_round = nproc - midpoint;
  }
}

void
BruckActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;
  bool in_place = dst == src;
  int block_size = nelems_ * type_size_;
  if (in_place){
    if (dom_me_ != 0){
      int inPlaceOffset = dom_me_* block_size;
      void* inPlaceSrc = ((char*)src + inPlaceOffset);
      my_api_->memcopy(dst, inPlaceSrc, block_size);
    }
  } else {
    //put everything into the dst buffer to begin
    my_api_->memcopy(dst, src, block_size);
  }
  result_buffer_ = dst;
  send_buffer_ = result_buffer_;
  recv_buffer_ = result_buffer_;
}

void
BruckActor::finalizeBuffers()
{
}

void
BruckActor::initDag()
{
  int log2nproc, midpoint, nprocs_extra_round, num_rounds;
  BruckTree::computeTree(dom_nproc_, log2nproc, midpoint, num_rounds, nprocs_extra_round);

  output.output("Bruck %s: configured for %d rounds with an extra round exchanging %d proc segments on tag=%d ",
    rankStr().c_str(), log2nproc, nprocs_extra_round, tag_);

  //in the last round, we send half of total data to nearest neighbor
  //in the penultimate round, we send 1/4 data to neighbor at distance=2
  //and so on...


  //for the allgather we do not worry about packed versus unpacked here
  //the allgather should ALWAYS run on packed data
  //it will be WAY more efficient to operate the entire collective on packed data first
  //and then unpack at the very end

  int partner_gap = 1;
  int round_nelems = nelems_;
  int nproc = dom_nproc_;
  Action *prev_send = 0, *prev_recv = 0;
  for (int i=0; i < num_rounds; ++i){
    int send_partner = (dom_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dom_me_ + partner_gap) % nproc;
    Action* send_ac = new SendAction(i, send_partner, SendAction::in_place);
    Action* recv_ac = new RecvAction(i, recv_partner, RecvAction::in_place);
    send_ac->offset = 0;
    recv_ac->offset = round_nelems;
    send_ac->nelems = round_nelems;
    recv_ac->nelems = round_nelems;

    addDependency(prev_send, send_ac);
    addDependency(prev_recv, send_ac);
    addDependency(prev_send, recv_ac);
    addDependency(prev_recv, recv_ac);

    partner_gap *= 2;
    round_nelems *= 2;
    prev_send = send_ac;
    prev_recv = recv_ac;
  }

  if (nprocs_extra_round){
    int nelems_extra_round = nprocs_extra_round * nelems_;
    int send_partner = (dom_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dom_me_ + partner_gap) % nproc;
    Action* send_ac = new SendAction(num_rounds+1,send_partner, SendAction::in_place);
    Action* recv_ac = new RecvAction(num_rounds+1,recv_partner, RecvAction::in_place);
    send_ac->offset = 0;
    recv_ac->offset = round_nelems;
    send_ac->nelems = nelems_extra_round;
    recv_ac->nelems = nelems_extra_round;

    addDependency(prev_send, send_ac);
    addDependency(prev_recv, send_ac);
    addDependency(prev_send, recv_ac);
    addDependency(prev_recv, recv_ac);
  }
}

void
BruckActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
BruckActor::finalize()
{
  // rank 0 need not reorder
  // or no buffers
  if (dom_me_ == 0 || result_buffer_ == 0){
    return;
  }

  //we need to reorder things a bit
  //first, copy everything out
  int total_nelems = nelems_* dom_nproc_;
  int total_size = total_nelems * type_size_;
  char* tmp = new char[total_size];
  my_api_->memcopy(tmp, result_buffer_, total_size);


  int my_offset = nelems_ * dom_me_;

  int copy_size = (total_nelems - my_offset) * type_size_;
  int copy_offset = my_offset * type_size_;

  void* src = tmp;
  void* dst = ((char*)result_buffer_) + copy_offset;
  my_api_->memcopy(dst, src, copy_size);

  copy_size = my_offset * type_size_;
  copy_offset = (total_nelems - my_offset) * type_size_;
  src = tmp + copy_offset;
  dst = result_buffer_;
  my_api_->memcopy(dst, src, copy_size);

  delete[] tmp;
}

void
RingAllgatherActor::initBuffers()
{
  recv_buffer_ = result_buffer_;
}

void
RingAllgatherActor::finalizeBuffers()
{
}

void
RingAllgatherActor::bufferAction(void *dst_buffer, void *msg_buffer, Action *ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
RingAllgatherActor::initDag()
{
  int send_partner = (dom_me_ + 1) % dom_nproc_;
  int recv_partner = (dom_me_ + dom_nproc_ - 1) % dom_nproc_;
  Action *prev_send = nullptr, *prev_recv = nullptr;
  int last_recved = dom_me_;
  for (int i=1; i < dom_nproc_; ++i){
    //the chunk rotates in a ring
    /**
     * 0->1->2->3->0
     * Rank 1 would exchange the following "segments"
     * receive 0, send 1
     * receive 3, send 0
     * receive 2, send 3
     */
    int recv_chunk = (last_recved - 1 + dom_nproc_) % dom_nproc_;
    int send_chunk  = last_recved;

    Action* send_ac = new SendAction(i, send_partner, SendAction::in_place);
    send_ac->offset = send_chunk * nelems_;
    send_ac->nelems = nelems_;
    Action* recv_ac = new RecvAction(i, recv_partner, RecvAction::in_place);
    recv_ac->offset = recv_chunk * nelems_;
    recv_ac->nelems = nelems_;

    addDependency(prev_send, send_ac);
    addDependency(prev_send, recv_ac);
    addDependency(prev_recv, send_ac);
    addDependency(prev_recv, recv_ac);

    last_recved = recv_chunk;
    prev_send = send_ac;
    prev_recv = recv_ac;
  }
}



}
