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

#include <iris/sumi/allgatherv.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
//#include <sprockit/output.h>
#include <cstring>

#define divide_by_2_round_up(x) \
  ((x/2) + (x%2))

#define divide_by_2_round_down(x) \
  (x/2)

//using namespace sprockit::dbg;

namespace SST::Iris::sumi {

void
BruckAllgathervActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;
  bool in_place = dst == src;
  //put everything into the dst buffer to begin
  if (in_place){
    if (dom_me_ != 0){
      int inPlaceOffset = my_offset_ * type_size_;
      void* inPlaceSrc = ((char*)src + inPlaceOffset);
      my_api_->memcopy(dst, inPlaceSrc, recv_counts_[dom_me_]*type_size_);
    }
  } else {
    my_api_->memcopy(dst, src, recv_counts_[dom_me_] * type_size_);
  }
  send_buffer_ = dst;
  recv_buffer_ = send_buffer_;
  result_buffer_ = send_buffer_;
}

void
BruckAllgathervActor::finalizeBuffers()
{
}

int
BruckAllgathervActor::nelems_to_recv(int partner, int partner_gap)
{
  int nelems_to_recv = 0;
  for (int p=0; p < partner_gap; ++p){
    int actual_partner = (partner + p) % dom_nproc_;
    nelems_to_recv += recv_counts_[actual_partner];
  }
  return nelems_to_recv;
}

void
BruckAllgathervActor::initDag()
{
  int virtual_nproc = dom_nproc_;

  /** let's go bruck algorithm for now */
  int nproc = 1;
  int log2nproc = 0;
  while (nproc < virtual_nproc)
  {
    ++log2nproc;
    nproc *= 2;
  }

  int num_extra_procs = 0;
  if (nproc > virtual_nproc){
    --log2nproc;
    //we will have to do an extra exchange in the last round
    num_extra_procs = virtual_nproc - nproc / 2;
  }

  int num_rounds = log2nproc;
  int nprocs_extra_round = num_extra_procs;

  output.output("Bruckv %s: configured for %d rounds with an extra round exchanging %d proc segments on tag=%d ",
    rankStr().c_str(), log2nproc, num_extra_procs, tag_);

  //in the last round, we send half of total data to nearest neighbor
  //in the penultimate round, we send 1/4 data to neighbor at distance=2
  //and so on...
  nproc = dom_nproc_;

  //as with the allgather, it makes absolutely no sense to run this collective on
  //unpacked data - everyone should immediately pack their data and then run the collective
  //on packed data instead


  int partner_gap = 1;
  Action *prev_send = nullptr, *prev_recv = nullptr;
  int nelems_recvd = recv_counts_[dom_me_];
  for (int i=0; i < num_rounds; ++i){
    int send_partner = (dom_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dom_me_ + partner_gap) % nproc;

    Action* send_ac = new SendAction(i, send_partner, SendAction::in_place);
    Action* recv_ac = new RecvAction(i, recv_partner, RecvAction::in_place);

    send_ac->offset = 0;
    recv_ac->offset = nelems_recvd;
    send_ac->nelems = nelems_recvd;
    recv_ac->nelems = nelems_to_recv(recv_partner, partner_gap);

    partner_gap *= 2;
    nelems_recvd += recv_ac->nelems;


    addDependency(prev_send, send_ac);
    addDependency(prev_recv, send_ac);
    addDependency(prev_send, recv_ac);
    addDependency(prev_recv, recv_ac);

    prev_send = send_ac;
    prev_recv = recv_ac;
  }

  if (nprocs_extra_round){
    int nelems_extra_round = total_nelems_ - nelems_recvd;
    int send_partner = (dom_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dom_me_ + partner_gap) % nproc;
    Action* send_ac = new SendAction(num_rounds,send_partner,SendAction::in_place);
    send_ac->offset = 0;
    //nelems_to_recv gives me the total number of elements the partner has
    //he needs the remainder to get up to total_nelems
    send_ac->nelems = total_nelems_ - nelems_to_recv(send_partner, partner_gap);
    Action* recv_ac = new RecvAction(num_rounds,recv_partner,RecvAction::in_place);
    recv_ac->offset = nelems_recvd;
    recv_ac->nelems = nelems_extra_round;

    addDependency(prev_send, send_ac);
    addDependency(prev_recv, send_ac);
    addDependency(prev_send, recv_ac);
    addDependency(prev_recv, recv_ac);
  }
}

void
BruckAllgathervActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
BruckAllgathervActor::finalize()
{
  // rank 0 need not reorder
  // or no buffers
  if (dom_me_ == 0 || result_buffer_ == 0){
    return;
  }

  //we need to reorder things a bit
  //first, copy everything out
  int total_size = total_nelems_ * type_size_;
  char* tmp = new char[total_size];
  my_api_->memcopy(tmp, result_buffer_, total_size);

  int copy_size = (total_nelems_ - my_offset_) * type_size_;
  int copy_offset = my_offset_ * type_size_;

  void* src = tmp;
  void* dst = ((char*)result_buffer_) + copy_offset;
  my_api_->memcopy(dst, src, copy_size);

  copy_size = my_offset_ * type_size_;
  copy_offset = (total_nelems_ - my_offset_) * type_size_;
  src = tmp + copy_offset;
  dst = result_buffer_;
  my_api_->memcopy(dst, src, copy_size);


  delete[] tmp;
}


}
