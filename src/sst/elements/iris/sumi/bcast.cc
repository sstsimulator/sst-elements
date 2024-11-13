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

#include <output.h>
#include <iris/sumi/bcast.h>
#include <iris/sumi/communicator.h>
#include <iris/sumi/transport.h>

namespace SST::Iris::sumi {

void
BinaryTreeBcastActor::bufferAction(void *dst_buffer, void *msg_buffer, Action *ac)
{
  ::memcpy(dst_buffer, msg_buffer, ac->nelems*type_size_);
}

void
BinaryTreeBcastActor::init_root(int  /*me*/, int roundNproc, int nproc)
{
  int partnerGap = roundNproc / 2;
  while (partnerGap > 0){
    if (partnerGap < nproc){ //might not be power of 2
      int partner = (root_ + partnerGap) % nproc; //everything offset by root
      Action* send = new SendAction(0, partner, SendAction::in_place);
      send->nelems = nelems_;
      send->offset = 0;
      addAction(send);
    }
    partnerGap /= 2;
  }
}

void
BinaryTreeBcastActor::init_internal(int offsetMe, int windowSize,
                                       int windowStop, Action* recv)
{

  //if contiguous - do everything through the result buf
  //if not contiguous, well, I'll have receive packed data
  //continue to send out that packed data
  //no need to unpack and re-pack
  SendAction::buf_type_t send_ty = slicer_->contiguous() ?
        SendAction::in_place : SendAction::prev_recv;

  int stride = windowSize;
  int nproc = comm_->nproc();
  while (stride > 0){
    int partner = offsetMe + stride;
    if (partner < windowStop){ //might not be power of 2
      partner = (partner + root_) % nproc; //everything offset by root
      Action* send = new SendAction(0, partner, send_ty);
      send->nelems = nelems_;
      send->offset = 0;
      addDependency(recv, send);
    }
    stride /= 2;
  }
}

void
BinaryTreeBcastActor::init_child(int offsetMe, int roundNproc, int nproc)
{
  int windowStart = 0;
  int windowSplit = roundNproc / 2;
  int windowSize = windowSplit;
  //figure out who I receive from
  while (windowSize > 0 && offsetMe != windowSplit){
    if (offsetMe > windowSplit){
      windowStart = windowSplit;
    }
    windowSize /= 2;
    windowSplit = windowStart + windowSize;
  }

  RecvAction::buf_type_t buf_ty = slicer_->contiguous() ?
        RecvAction::in_place : RecvAction::unpack_temp_buf;

  int parent = (windowStart + root_) % nproc; //everything offset by root
  Action* recv = new RecvAction(0, parent, buf_ty);
  recv->nelems = nelems_;
  recv->offset = 0;
  addAction(recv);

  int windowStop = std::min(nproc, (windowSplit + windowSize));
  output.output("Rank %s is in window %d->%d:%d in initing bcast",
    rankStr().c_str(), windowStart, windowSplit, windowSplit + windowSize);

  if (offsetMe % 2 == 0){
    init_internal(offsetMe, windowSize, windowStop, recv);
  } else {
    //pass, leaf node
  }
}

void
BinaryTreeBcastActor::finalizeBuffers()
{
}

void
BinaryTreeBcastActor::initDag()
{
  int roundNproc = 1;
  int nproc = comm_->nproc();
  int me = comm_->myCommRank();
  while (roundNproc < nproc){
    roundNproc *= 2;
  }

  if (me == root_){
    init_root(me, roundNproc, nproc);
  } else {
    int offsetMe = (me - root_ + nproc) % nproc;
    init_child(offsetMe, roundNproc, nproc);
  }
}

void
BinaryTreeBcastActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;

  if (dom_me_ == 0) send_buffer_ = src; //root
  else send_buffer_ = dst;

  recv_buffer_ = send_buffer_;
  result_buffer_ = send_buffer_;
}


}
