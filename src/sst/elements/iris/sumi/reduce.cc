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

#include <iris/sumi/reduce.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
//#include <sprockit/output.h>
#include <mercury/common/stl_string.h>
#include <cstring>

#define divide_by_2_round_up(x) \
  ((x/2) + (x%2))

#define divide_by_2_round_down(x) \
  (x/2)

//using namespace sprockit::dbg;

namespace SST::Iris::sumi {


void
WilkeReduceActor::finalizeBuffers()
{
  //nothing to do
  if (!result_buffer_) return;

  //if we need to do operations, then we need a temp buffer for doing sends
  int size = nelems_ * type_size_;

  if (dom_me_ != root_){
    my_api_->freeWorkspace(result_buffer_, size);
  }
  my_api_->freeWorkspace(recv_buffer_, size);
}

void
WilkeReduceActor::initBuffers()
{
  void* src = send_buffer_;

  //if we need to do operations, then we need a temp buffer for doing sends
  int size = nelems_ * type_size_;

  if (dom_me_ != root_){
    //I need a result buffer - was not given anything
    result_buffer_ = my_api_->allocateWorkspace(size, src);
  }

  //but! we need a temporary recv buffer
  recv_buffer_ = my_api_->allocateWorkspace(size, src);
  send_buffer_ = result_buffer_;

  //we will now only work with the dst buffer
  my_api_->memcopy(send_buffer_, src, size);
}

void
WilkeReduceActor::initDag()
{
  slicer_->fxn = fxn_;

  int log2nproc, midpoint, virtual_nproc;
  RecursiveDoubling::computeTree(dom_nproc_, log2nproc, midpoint, virtual_nproc);

  VirtualRankMap rank_map(dom_nproc_, virtual_nproc);
  int my_roles[2];
  int num_roles = rank_map.realToVirtual(dom_me_, my_roles);

  int num_doubling_rounds = log2nproc;

  output.output("Rank %s configured reduce for tag=%d for nproc=%d(%d) virtualized to n=%d over %d rounds",
    rankStr().c_str(), tag_, dom_nproc_, my_api_->nproc(), virtual_nproc, log2nproc);

  //I either receive directly into the final buffer
  //Or I have to receive a bunch of packed stuff into a temp buffer
  RecvAction::buf_type_t fan_in_recv_type = slicer_->contiguous() ?
        RecvAction::in_place : RecvAction::packed_temp_buf;

  //on the fan-in, I'll be received packed data
  //there's no need to unpack during the fan-in
  //only need an unpack at the very end
  SendAction::buf_type_t fan_in_send_type = slicer_->contiguous() ?
        SendAction::in_place : SendAction::prev_recv;

  Action* final_join = new Action(Action::join, 0, 0);

  for (int role=0; role < num_roles; ++role){
    Action* null = nullptr;
    std::vector<Action*> send_rounds(num_doubling_rounds, null);
    std::vector<Action*> recv_rounds(num_doubling_rounds, null);

    int my_buffer_offset = 0;

    int partner_gap = 1;
    int round_nelems = nelems_;

    Action *prev_send = nullptr, *prev_recv = nullptr;

    int virtual_me = my_roles[role];
    bool i_am_even = (virtual_me % 2) == 0;
    int round_offset = 2*num_doubling_rounds;
    output.output("Rank %d configuring reduce for virtual role=%d tag=%d for nproc=%d(%d) virtualized to n=%d over %d rounds ",
      my_api_->rank(), virtual_me, tag_, dom_nproc_, my_api_->nproc(), virtual_nproc, log2nproc);
    for (int i=0; i < num_doubling_rounds; ++i){
      //again, see comment above about weirndess of round numberings
      int rnd = (i == 0 || i_am_even) ? i : i + round_offset;
      bool i_am_low = isLowerPartner(virtual_me, partner_gap);
      int virtual_partner, send_nelems, recv_nelems, send_offset, recv_offset;
      if (i_am_low){
        virtual_partner = virtual_me + partner_gap;
        send_nelems = divide_by_2_round_down(round_nelems);
        send_offset = my_buffer_offset + round_nelems - send_nelems;
        recv_offset = my_buffer_offset;
      } else {
        virtual_partner = virtual_me - partner_gap;
        send_nelems = divide_by_2_round_up(round_nelems);
        send_offset = my_buffer_offset;
        recv_offset = my_buffer_offset + send_nelems;
      }

      output.output("Rank %d:%d testing partner=%d tag=%d for round=%d,%d",
        my_api_->rank(), virtual_me, virtual_partner, tag_, i, rnd);

      recv_nelems = round_nelems - send_nelems;

      if (!isSharedRole(virtual_partner, num_roles, my_roles)){
        int partner = rank_map.virtualToReal(virtual_partner);
        //this is not colocated with me - real send/recv
        Action* send_ac = new SendAction(rnd, partner, SendAction::in_place);
        send_ac->offset = send_offset;
        send_ac->nelems = send_nelems;

        Action* recv_ac = new RecvAction(rnd, partner, RecvAction::reduce);
        recv_ac->offset = recv_offset;
        recv_ac->nelems = round_nelems - send_nelems;


        addDependency(prev_send, send_ac);
        addDependency(prev_send, recv_ac);
        addDependency(prev_recv, send_ac);
        addDependency(prev_recv, recv_ac);

        send_rounds[i] = send_ac;
        recv_rounds[i] = recv_ac;

        prev_send = send_ac;
        prev_recv = recv_ac;
       //end if not real send/recv
      } else {
        output.output("Rank %d:%d skipping partner=%d on round %d with send=(%d,%d) recv=(%d,%d)",
          my_api_->rank(), virtual_me, virtual_partner, i, send_offset, send_offset + send_nelems,
          recv_offset, recv_offset + recv_nelems);
      }

      //whatever I recv becomes the subarray for the next iteration
      my_buffer_offset = recv_offset;
      partner_gap *= 2;
      round_nelems = recv_nelems;
    } //end loop over fan-out reduce rounds

    //the last gather round is skipped
    int num_halving_rounds = num_doubling_rounds;
    if (root_ != 0){
      num_halving_rounds -= 1;
    }

    int max_active_rank = virtual_nproc;
    for (int i=0; i < num_halving_rounds; ++i){
      //I am done - leave the reduce
      if (virtual_me >= max_active_rank) break;

      int mirror_round = num_doubling_rounds - i - 1;
      int my_round = num_doubling_rounds + i;
      if (mirror_round == 0 || i_am_even){
        //my round is correct
      } else {
        my_round += round_offset;
      }
      Action* mirror_send = send_rounds[mirror_round];
      Action* mirror_recv = recv_rounds[mirror_round];
      if (mirror_send && mirror_recv){
        //if it's a real action, not colocated
        //for the first fan-in send, always send in-place from the result buf
        //for the rest of the fan-in sends, changes for non-contiguous types
        //the fan-in should operate entirely on packed data
        Action* send_ac = new SendAction(my_round, mirror_recv->partner,
                               i == 0 ? SendAction::in_place : fan_in_send_type);
        send_ac->nelems = mirror_recv->nelems;
        send_ac->offset = mirror_recv->offset;
        addDependency(prev_send, send_ac);
        addDependency(prev_recv, send_ac);

        Action* recv_ac = new RecvAction(my_round, mirror_send->partner,
                                          fan_in_recv_type);
        recv_ac->nelems = mirror_send->nelems;
        recv_ac->offset = mirror_send->offset;
        addDependency(prev_send, recv_ac);
        addDependency(prev_recv, recv_ac);

        prev_send = send_ac;
        prev_recv = recv_ac;
      } //end if real send/recv
      max_active_rank /= 2;
    } //end loop over fan-in recv rounds

    addDependency(prev_send, final_join);
    addDependency(prev_recv, final_join);
  }

  if (root_ != 0){
    int round = 4*num_doubling_rounds - 1;
    int nelems_split = divide_by_2_round_up(nelems_);
    //rank 0 and midpoint must send to root
    if (dom_me_ == 0){
      Action* send_ac = new SendAction(round, root_, fan_in_send_type);
      send_ac->nelems = nelems_split;
      send_ac->offset = 0;
      addDependency(final_join, send_ac);
    }

    if (dom_me_ == 1 && root_ != dom_me_){
      Action* send_ac = new SendAction(round, root_, fan_in_send_type);
      send_ac->nelems = nelems_ - nelems_split;
      send_ac->offset = nelems_split;
      addDependency(final_join, send_ac);
    }

    if (dom_me_ == root_){
      Action* recv_ac = new RecvAction(round, 0, fan_in_recv_type);
      recv_ac->nelems = nelems_split;
      recv_ac->offset = 0;
      addDependency(final_join, recv_ac);
    }

    if (dom_me_ == root_ && root_ != 1){
      Action* recv_ac = new RecvAction(round, 1, fan_in_recv_type);
      recv_ac->nelems = nelems_ - nelems_split;
      recv_ac->offset = nelems_split;
      addDependency(final_join, recv_ac);
    }
  }

  num_reducing_rounds_ = num_doubling_rounds;
  num_total_rounds_ = num_doubling_rounds * 2;
}

bool
WilkeReduceActor::isLowerPartner(int virtual_me, int partner_gap)
{
  int my_role = (virtual_me / partner_gap) % 2;
  return my_role == 0;
}

void
WilkeReduceActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  int rnd = ac->round % num_total_rounds_;
  if (rnd < num_reducing_rounds_){
    (fxn_)(dst_buffer, msg_buffer, ac->nelems);
  } else {
    my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
  }
}

}
