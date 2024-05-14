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

#include <iris/sumi/transport_fwd.h>
#include <set>
#include <map>
#include <vector>

#pragma once

namespace SST::Iris::sumi {

class Communicator {
 public:
  class RankCallback {
   public:
    virtual void rankResolved(int global_rank, int comm_rank) = 0;
  };

  virtual int nproc() const = 0;

  int myCommRank() const {
    return my_comm_rank_;
  }

  virtual ~Communicator(){}

  /**
   * @brief comm_to_global_rank
   * In the given communicator, map a comm-specific rank
   * to the actual global rank received at launch.
   * Can return unresolved_rank if not yet known.
   * @param comm_rank
   * @return The physical rank in the global communicator.
  */
  virtual int commToGlobalRank(int comm_rank) const = 0;

  virtual int globalToCommRank(int global_rank) const = 0;

  virtual std::set<int> globalRankSetIntersection(const std::set<int>& neighbors) const = 0;

  virtual bool supportsSmp() const {
    return false;
  }

  bool smpBalanced() const {
    return smp_balanced_;
  }

  static const int unresolved_rank = -1;

  void createSmpCommunicator(const std::set<int>& neighbors,
                             CollectiveEngine* engine, int cq_id);

  Communicator* smpComm() const {
    return smp_comm_;
  }

  Communicator* ownerComm() const {
    return owner_comm_;
  }

  void registerRankCallback(RankCallback* cback){
    rank_callbacks_.insert(cback);
  }

  void eraseRankCallback(RankCallback* cback){
    rank_callbacks_.erase(cback);
  }

 protected:
  Communicator(int comm_rank) :
    my_comm_rank_(comm_rank),
    smp_comm_(nullptr),
    owner_comm_(nullptr),
    smp_balanced_(false)
  {}

  void rankResolved(int global_rank, int comm_rank);

 private:
  int my_comm_rank_;

  /**
   * Domain ranks do not need immediate resolution to physical ranks
   * If there is a delay in resolution, allow callbacks to be registered
  */
  std::set<RankCallback*> rank_callbacks_;

  Communicator* smp_comm_;
  Communicator* owner_comm_;
  bool smp_balanced_;

};

class GlobalCommunicator :
  public Communicator
{
 public:
  GlobalCommunicator(Transport* tport);

  int nproc() const override;

  bool supportsSmp() const override {
    return true;
  }

  int commToGlobalRank(int comm_rank) const override;

  int globalToCommRank(int global_rank) const override;

  std::set<int> globalRankSetIntersection(const std::set<int>& neighbors) const override {
    return neighbors;
  }

 private:
  Transport* transport_;
};

class MapCommunicator :
  public Communicator
{
 public:
  MapCommunicator(int rank, std::vector<int>&& local_to_global);

  int nproc() const override {
    return global_to_local_.size();
  }

  int commToGlobalRank(int comm_rank) const override;

  int globalToCommRank(int global_rank) const override;

  std::set<int> globalRankSetIntersection(const std::set<int> &neighbors) const override;

 private:
  std::map<int, int> global_to_local_;
  std::vector<int> local_to_global_;
};

class ShiftedCommunicator :
  public Communicator
{
 public:
  ShiftedCommunicator(Communicator* dom, int left_shift) :
    Communicator((dom->myCommRank() - left_shift + dom->nproc()) % dom->nproc()),
    dom_(dom),
    nproc_(dom->nproc()),
    shift_(left_shift)
  {}

  int nproc() const override {
    return dom_->nproc();
  }

  int commToGlobalRank(int comm_rank) const override {
    int shifted_rank = (comm_rank + shift_) % nproc_;
    return dom_->commToGlobalRank(shifted_rank);
  }

  int globalToCommRank(int global_rank) const override {
    int comm_rank = dom_->globalToCommRank(global_rank);
    int shifted_rank = (comm_rank - shift_ + nproc_) % nproc_;
    return shifted_rank;
  }

 private:
  Communicator* dom_;
  int nproc_;
  int shift_;

};

class IndexCommunicator :
  public Communicator
{
 public:
  /**
   * @brief index_domain
   * @param nproc
   * @param proc_list
   */
  IndexCommunicator(int comm_rank, int nproc, std::vector<int>&& proc_list) :
    Communicator(comm_rank),
    proc_list_(std::move(proc_list)), nproc_(nproc)
  {
  }

  int nproc() const override {
    return nproc_;
  }

  int commToGlobalRank(int comm_rank) const override {
    return proc_list_[comm_rank];
  }

  int globalToCommRank(int global_rank) const override;

  std::set<int> globalRankSetIntersection(const std::set<int> &neighbors) const override;

 private:
  std::vector<int> proc_list_;
  int nproc_;

};

class RotateCommunicator :
  public Communicator
{
 public:
  /**
   * @brief rotate_domain
   * @param nproc
   * @param shift
   * @param me
   */
  RotateCommunicator(int my_global_rank, int nproc, int shift) :
    Communicator(globalToCommRank(my_global_rank)),
    nproc_(nproc), shift_(shift)
  {
  }

  int nproc() const override {
    return nproc_;
  }

  int commToGlobalRank(int comm_rank) const override {
    return (comm_rank + shift_) %  nproc_;
  }

  int globalToCommRank(int global_rank) const override {
    return (global_rank + nproc_ - shift_) % nproc_;
  }

  bool supportsSmp() const override {
    return false;
  }

  std::set<int> globalRankSetIntersection(const std::set<int> &neighbors) const override;

 private:
  int nproc_;
  int shift_;

};

class SubrangeCommunicator :
  public Communicator
{
 public:
  SubrangeCommunicator(int my_global_rank, int start, int nproc) :
    Communicator(globalToCommRank(my_global_rank)),
    nproc_(nproc), start_(start)
  {
  }

  int nproc() const override {
    return nproc_;
  }

  int commToGlobalRank(int comm_rank) const override {
    return comm_rank + start_;
  }

  int globalToCommRank(int global_rank) const override {
    return global_rank - start_;
  }

  bool supportsSmp() const override {
    return true;
  }

  std::set<int> globalRankSetIntersection(const std::set<int>& neighbors) const override;

 private:
  int nproc_;
  int start_;
};

}
