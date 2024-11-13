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
