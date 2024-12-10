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

#include <iris/sumi/alltoall.h>
#include <iris/sumi/allgather.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
//#include <sprockit/output.h>
#include <cstring>

#define divide_by_2_round_up(x) \
  ((x/2) + (x%2))

#define divide_by_2_round_down(x) \
  (x/2)

//using namespace sprockit::dbg;

#define SEND_SHUFFLE 0
#define RECV_SHUFFLE 1

namespace SST::Iris::sumi {

void
BruckAlltoallActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;
  int log2nproc, num_rounds, nprocs_extra_round;
  BruckTree::computeTree(dom_nproc_, log2nproc, midpoint_, num_rounds, nprocs_extra_round);

  //put everything into the dst buffer to begin
  //but we have to shuffle for bruck algorithm
  int offset = dom_me_ * nelems_ * type_size_;
  int total_size = dom_nproc_ * nelems_ * type_size_;
  char* srcPtr = (char*) src;
  char* dstPtr = (char*) dst;
  int copySize = total_size - offset;
  my_api_->memcopy(dstPtr, srcPtr + offset, copySize);
  my_api_->memcopy(dstPtr + copySize, srcPtr, offset);

  int tmp_buffer_size = nelems_ * type_size_ * midpoint_;
  result_buffer_ = dst;
  send_buffer_ = my_api_->allocateWorkspace(tmp_buffer_size, src);
  recv_buffer_ = my_api_->allocateWorkspace(tmp_buffer_size, src);
}

void
BruckAlltoallActor::finalizeBuffers()
{
  int buffer_size = nelems_ * type_size_ * comm_->nproc();
  int tmp_buffer_size = nelems_ * type_size_ * midpoint_;
  my_api_->freeWorkspace(recv_buffer_, tmp_buffer_size);
  my_api_->freeWorkspace(send_buffer_, tmp_buffer_size);
}

void
BruckAlltoallActor::shuffle(Action *ac, void* tmpBuf, void* mainBuf, bool copyToTemp)
{
  int nproc = dom_nproc_;
  char* tmp_buffer = (char*) tmpBuf;
  char* main_buffer = (char*) mainBuf;
  int blocksPerCopy = ac->offset;
  int blockStride = blocksPerCopy*2;
  int tmpBlock = 0;
  for (int mainBlock=ac->offset; mainBlock < nproc; tmpBlock += blocksPerCopy, mainBlock += blockStride){
    int remainingBlocks = nproc - mainBlock;
    int numCopyBlocks = std::min(blocksPerCopy, remainingBlocks);
    int copySize = numCopyBlocks * nelems_ * type_size_;
    void* tmp = tmp_buffer + tmpBlock*nelems_*type_size_;
    void* main = main_buffer + mainBlock*nelems_*type_size_;
    if (copyToTemp){
      ::memcpy(tmp, main, copySize);
    } else {
      ::memcpy(main, tmp, copySize);
    }
  }
}

void
BruckAlltoallActor::startShuffle(Action *ac)
{
  if (result_buffer_ == nullptr) return;

  if (ac->partner == SEND_SHUFFLE){
    //shuffle to get ready for a send
    //shuffle from result_buffer into send_buffer
    shuffle(ac, send_buffer_, result_buffer_, true/*copy to temp send buffer*/);
  } else {
    shuffle(ac, recv_buffer_, result_buffer_, false/*copy from temp recv into result*/);
  }

}

void
BruckAlltoallActor::initDag()
{
  int log2nproc, num_rounds, nprocs_extra_round;
  BruckTree::computeTree(dom_nproc_, log2nproc, midpoint_, num_rounds, nprocs_extra_round);

  int partnerGap = 1;
  int me = dom_me_;
  int nproc = dom_nproc_;
  Action* prev_shuffle = nullptr;
  if (nprocs_extra_round) ++num_rounds;

  //similar to the allgather - makes no sense to run this on unpacked ata
  //first thing everyone should do is pack all their data

  for (int round=0; round < num_rounds; ++round){
    int up_partner = (me + partnerGap) % nproc;
    int down_partner = (me - partnerGap + nproc) % nproc;
    int intervalSendSize = partnerGap;
    int elemStride = partnerGap*2;
    int bruckIntervalSize = elemStride;

    int sendWindowSize = nproc - partnerGap;
    int numBruckIntervals = sendWindowSize / bruckIntervalSize;
    int numSendBlocks = numBruckIntervals * intervalSendSize;
    int remainder = sendWindowSize % bruckIntervalSize;
    int extraSendBlocks = std::min(intervalSendSize, remainder);
    numSendBlocks += extraSendBlocks;

    Action* send_shuffle = new ShuffleAction(round, SEND_SHUFFLE);
    Action* recv_shuffle = new ShuffleAction(round, RECV_SHUFFLE);
    Action* send = new SendAction(round, up_partner, SendAction::temp_send);
    //always recv into a temp buf - and leave it packed, do not unpack
    Action* recv = new RecvAction(round, down_partner, RecvAction::packed_temp_buf);

    int nelemsRound = numSendBlocks * nelems_;

    send->offset = 0;
    send->nelems = nelemsRound;

    recv->offset = 0;
    recv->nelems = nelemsRound;

    send_shuffle->offset = partnerGap;
    recv_shuffle->offset = partnerGap;
    send_shuffle->nelems = nelemsRound;
    recv_shuffle->nelems = nelemsRound;

    addDependency(prev_shuffle, send_shuffle);
    addDependency(prev_shuffle, send_shuffle);

    addDependency(send_shuffle, send);
    addDependency(send_shuffle, recv);

    addDependency(send, recv_shuffle);
    addDependency(recv, recv_shuffle);

    prev_shuffle = recv_shuffle;
    partnerGap *= 2;
  }
}

void
BruckAlltoallActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
BruckAlltoallActor::finalize()
{
  if (result_buffer_ == 0){
    return;
  }

  int total_size = dom_nproc_ * nelems_ * type_size_;
  int block_size = nelems_ * type_size_;
  char* tmp = new char[total_size];
  char* result = (char*) result_buffer_;
  for (int i=0; i < dom_nproc_; ++i){
    char* src = result + i*block_size;
    int dst_index = (dom_me_ + dom_nproc_ - i) % dom_nproc_;
    char* dst = tmp + dst_index*block_size;
    ::memcpy(dst, src, block_size);
  }

  ::memcpy(result, tmp, total_size);

  do_sumi_debug_print("final result buf",
    rank_str().c_str(), dense_me_,
    -1,
    0, nelems_*dom_nproc_,
    type_size_,
    result_buffer_.ptr);

  delete[] tmp;
}

void
DirectAlltoallActor::initBuffers()
{
  total_send_size_ = nelems_ * type_size_;
  total_recv_size_ = nelems_ * type_size_;
  recv_buffer_ = result_buffer_;
}

void
DirectAlltoallActor::finalizeBuffers()
{
}

void
DirectAlltoallActor::addAction(
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
DirectAlltoallActor::initDag()
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
    send->nelems = nelems_ * i;
    recv->offset = recv_offset;
    recv->nelems = nelems_ * i;

    sends[i] = send;
    recvs[i] = recv;
  }

  int num_initial = 3;
  for (int i=0; i < dom_nproc_; ++i){
    //move down for recvs
    addAction(recvs, -1, num_initial, i);
    //move up for sends
    addAction(sends, 1, num_initial, i);
  }

}

void
DirectAlltoallActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  my_api_->memcopy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
DirectAlltoallActor::finalize()
{
}


}
