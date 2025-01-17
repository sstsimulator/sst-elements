// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iris/sumi/gather.h>
#include <iris/sumi/communicator.h>
#include <iris/sumi/transport.h>

namespace SST::Iris::sumi {

void
BtreeGatherActor::initTree()
{
  log2nproc_ = 0;
  midpoint_ = 1;
  int nproc = comm_->nproc();
  while (midpoint_ < nproc){
    midpoint_ *= 2;
    log2nproc_++;
  }
  //unrull one - we went too far
  midpoint_ /= 2;
}

void
BtreeGatherActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;

  int me = comm_->myCommRank();

  if (me == root_){
    result_buffer_ = dst;
    recv_buffer_ = result_buffer_;
    send_buffer_ = result_buffer_;
  } else {
    int max_recv_buf_size = midpoint_*nelems_*type_size_;
    recv_buffer_ = my_api_->allocateWorkspace(max_recv_buf_size, src);
    send_buffer_ = recv_buffer_;
    result_buffer_ = recv_buffer_;
  }

  my_api_->memcopy(recv_buffer_, src, nelems_*type_size_);
}

void
BtreeGatherActor::finalizeBuffers()
{
  if (!result_buffer_)
    return;

  int me = comm_->myCommRank();
  if (me != root_){
    int max_recv_buf_size = midpoint_*nelems_*type_size_;
    my_api_->freeWorkspace(recv_buffer_,max_recv_buf_size);
  }
}

void
BtreeGatherActor::startShuffle(Action *ac)
{
  if (result_buffer_){
    //only ever arises in weird midpoint scenarios
    int copy_size = ac->nelems * type_size_;
    int copy_offset = ac->offset * type_size_;
    char* dst = ((char*)result_buffer_) + copy_offset;
    char* src = ((char*)result_buffer_);
    ::memcpy(dst, src, copy_size);
  }
}

void
BtreeGatherActor::bufferAction(void *dst_buffer, void *msg_buffer, Action *ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
BtreeGatherActor::initDag()
{
  int me = comm_->myCommRank();
  int nproc = comm_->nproc();
  int round = 0;

  int maxGap = midpoint_;
  if (root_ != 0){
    //special case to handle the last gather round
    maxGap = midpoint_ / 2;
  }

  //as with the allgather, it makes no sense to run the gather on unpacked data
  //everyone should immediately pack all their buffers and then run the collective
  //directly on already packed data

  Action* prev = 0;

  int partnerGap = 1;
  int stride = 2;
  while (1){
    if (partnerGap > maxGap) break;

    //just keep going until you stop
    if (me % stride == 0){
      //I am a recver
      int partner = me + partnerGap;
      if (partner < nproc){
        Action* recv = new RecvAction(round, partner, RecvAction::in_place);
        int recvChunkStart = me + partnerGap;
        int recvChunkStop = std::min(recvChunkStart+partnerGap, nproc);
        int recvChunkSize = recvChunkStop - recvChunkStart;
        recv->nelems = nelems_ * recvChunkSize;
        recv->offset = partnerGap * nelems_;  //I receive into top half of my buffer
        addDependency(prev, recv);
        prev = recv;
      }
    } else {
      //I am a sender
      int partner = me - partnerGap;
      Action* send = new SendAction(round, partner, SendAction::in_place);
      int sendChunkStart = me;
      int sendChunkStop = std::min(sendChunkStart+partnerGap,nproc);
      int sendChunkSize = sendChunkStop - sendChunkStart;
      send->nelems = nelems_*sendChunkSize;
      send->offset = 0; //I send my whole buffer
      addDependency(prev, send);
      prev = send;
      break; //I am done, yo
    }
    ++round;
    partnerGap *= 2;
    stride *= 2;
  }

  round = log2nproc_;
  if (root_ != 0 && root_ == midpoint_ && me == root_){
    //I have to shuffle my data
    Action* shuffle = new ShuffleAction(round, me);
    shuffle->offset = midpoint_ * nelems_;
    shuffle->nelems = (nproc - midpoint_) * nelems_;
    addDependency(prev, shuffle);
    prev = shuffle;
  }


  if (root_ != 0){
    //the root must receive from 0 and midpoint
    if (me == root_){
      int size_1st_half = midpoint_;
      int size_2nd_half = nproc - midpoint_;
      //recv 1st half from 0
      Action* recv = new RecvAction(round, 0, RecvAction::in_place);
      recv->offset = 0;
      recv->nelems = nelems_ * size_1st_half;
      addDependency(prev, recv);
      //recv 2nd half from midpoint - unless I am the midpoint
      if (midpoint_ != root_){
        recv = new RecvAction(round, midpoint_, RecvAction::in_place);
        recv->offset = midpoint_*nelems_;
        recv->nelems = nelems_ * size_2nd_half;
        addDependency(prev, recv);
      }
    }
    //0 must send the first half to the root
    if (me == 0){
      Action* send = new SendAction(round, root_, SendAction::in_place);
      send->offset = 0; //send whole thing
      send->nelems = nelems_*midpoint_;
      addDependency(prev,send);
    }
    //midpoint must send the second half to the root
    //unless it is the root
    if (me == midpoint_ && midpoint_ != root_){
      Action* send = new SendAction(round, root_, SendAction::in_place);
      int size = nproc - midpoint_;
      send->offset = 0;
      send->nelems = nelems_ * size;
      addDependency(prev,send);
    }
  }
}


}
