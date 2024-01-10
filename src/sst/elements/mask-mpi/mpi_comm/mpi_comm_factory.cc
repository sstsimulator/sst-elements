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

#include <mpi_comm/mpi_comm_factory.h>
#include <mpi_comm/mpi_comm.h>
#include <mpi_comm/mpi_comm_cart.h>
#include <mpi_comm/mpi_group.h>
#include <mpi_api.h>
#include <mpi_integers.h>
#include <mpi_types.h>
#include <mercury/common/errors.h>
#include <mercury/common/stl_string.h>

#include <unusedvariablemacro.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <stdint.h>
#include <iterator>

namespace SST::Hg {
void apiLock();
void apiUnlock();
}

namespace SST::MASKMPI {

//
// Build comm_world using information retrieved from the environment.
//
MpiCommFactory::MpiCommFactory(SoftwareId sid, MpiApi* parent) :
  parent_(parent),
  aid_(sid.app_),
  next_id_(1),
  worldcomm_(nullptr),
  selfcomm_(nullptr)
{
}

MpiCommFactory::~MpiCommFactory()
{
  //these will get deleted by loop over comm map
  //in ~mpi_api()
  //if (worldcomm_) delete worldcomm_;
  //if (selfcomm_) delete selfcomm_;
}

void
MpiCommFactory::init(int rank, int nproc)
{
  next_id_ = 2;

  MpiGroup* g = new MpiGroup(nproc);
  g->setId(MPI_GROUP_WORLD);
  worldcomm_ = new MpiComm(MPI_COMM_WORLD, rank, g, aid_);

  std::vector<TaskId> selfp;
  selfp.push_back(TaskId(rank));
  MpiGroup* g2 = new MpiGroup(selfp);
  g2->setId(MPI_GROUP_SELF);
  selfcomm_ = new MpiComm(MPI_COMM_SELF, int(0), g2, aid_);
}

MpiComm*
MpiCommFactory::comm_dup(MpiComm* caller)
{
  MpiComm* ret = this->commCreate(caller, caller->group_);
  ret->dupKeyvals(caller);
  return ret;
}


MPI_Comm
MpiCommFactory::commNewIdAgree(MpiComm* commPtr)
{
  int inputID = next_id_;
  int outputID = 0;
  auto op = parent_->startAllreduce(commPtr, 1, MPI_INT, MPI_MAX, &inputID, &outputID);
  parent_->waitCollective(std::move(op));

  next_id_ = outputID + 1;
  return outputID;
}

MpiComm*
MpiCommFactory::commCreateGroup(MpiComm* caller, MpiGroup* group)
{
  //now find my rank
  int newrank = group->rankOfTask(caller->myTask());
  if (newrank >= 0) {
    //okay... this is a little wonky
    //I need to create the new communicator on a reserved ID first
    MPI_Comm tmpId = caller->id() * 1000 + 100000;
    MpiComm* newComm = new MpiComm(tmpId, newrank, group, aid_);
    MPI_Comm cid = commNewIdAgree(newComm);
    newComm->setId(cid);
    return newComm;
  } else {
    //there is no guarantee that an MPI rank in the comm, but not in the group
    //will actually make this call
    //all ranks in the comm but not the group should return immediately
    //and not participate in the collective
    return MpiComm::comm_null;
  }
}

MpiComm*
MpiCommFactory::commCreate(MpiComm* caller, MpiGroup* group)
{
  MPI_Comm cid = commNewIdAgree(caller);

  //now find my rank
  int newrank = group->rankOfTask(caller->myTask());

  if (newrank >= 0) {
    return new MpiComm(cid, newrank, group, aid_);
  } else {
    return MpiComm::comm_null;
  }

}

typedef std::map<int, std::list<int> > key_to_ranks_map;
#if !SST_HG_DISTRIBUTED_MEMORY || SST_HG_MMAP_COLLECTIVES

typedef std::map<int, key_to_ranks_map> color_to_key_map;
//comm id, comm root task id, tag

struct comm_split_entry {
  int* buf;
  int refcount;
  comm_split_entry() : buf(0), refcount(0) {}
};

static std::map<int, //app ID
        std::map<int, //comm ID
          std::map<int, //comm rank 0 comm world
              std::map<int, comm_split_entry> //tag
          >
         >
       > comm_split_entries;
#endif


//
// MPI_Comm_split.
//
MpiComm*
MpiCommFactory::commSplit(MpiComm* caller, int my_color, int my_key)
{
  key_to_ranks_map key_map;
  int mydata[3];
  mydata[0] = next_id_;
  mydata[1] = my_color;
  mydata[2] = my_key;

  AppId aid = parent_->sid().app_;

  //printf("Rank %d = {%d %d %d}\n",
  //       caller->rank(), next_id_, my_color, my_key);

#if SST_HG_DISTRIBUTED_MEMORY && !SST_HG_MMAP_COLLECTIVES
  int* result = new int[3*caller->size()];
  parent_->allgather(&mydata, 3, MPI_INT,
                     result, 3, MPI_INT,
                     caller->id());
#else
  SST::Hg::apiLock();
  int root = caller->peerTask(int(0));
  int tag = caller->nextCollectiveTag();
  comm_split_entry& entry = comm_split_entries[aid][int(caller->id())][root][tag];
  entry.refcount++;
  if (entry.buf == 0){
#if SST_HG_MMAP_COLLECTIVES
    char fname[256];
    size_t len = 3*caller->size()*sizeof(int);
    sprintf(fname, "%d.%d.%d", int(caller->id()), root, tag);
    int fd = shm_open(fname, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
    if (fd < 0){ //oops, someone else already created it
      fd = shm_open(fname, O_RDWR, S_IRWXU);
    }
    if (fd < 0){
      spkt_throw_printf(sprockit::value_error,
        "invalid fd %d shm_open on %s: error=%d",
        fd, fname, errno);
    }
    ftruncate(fd, len);
    void* bufptr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (bufptr == ((void*)-1)){
      spkt_throw_printf(sprockit::value_error,
        "bad mmap on shm_open %s:%d: error=%d",
        fname, fd, errno);
    }
    entry.buf = (int*)bufptr;
#else
    entry.buf = new int[3*caller->size()];
#endif
  }
  int* result = entry.buf;
  int* mybuf = result + 3*caller->rank();
  mybuf[0] = mydata[0];
  mybuf[1] = mydata[1];
  mybuf[2] = mydata[2];
  SST::Hg::apiUnlock();

  //just model the allgather

  auto op = parent_->startAllgather("MPI_Comm_split_allgather", caller->id(),
                                      3, MPI_INT, 3, MPI_INT, nullptr, nullptr);
  parent_->waitCollective(std::move(op));
#endif

  MpiComm* ret;
  if (my_color < 0){ //I'm not part of this!
    ret = MpiComm::comm_null;
  } else {
    int cid = -1;
    int ninput_ranks = caller->size();
    int new_comm_size = 0;
    for (unsigned rank = 0; rank < ninput_ranks; rank++) {
      int* thisdata = result + 3*rank;

      int comm_id = thisdata[0];
      int color = thisdata[1];
      int key = thisdata[2];

      if (color >= 0 && color == my_color){
        key_map[key].push_back(rank);
        ++new_comm_size;
      }

      if (comm_id > cid) {
        cid = comm_id;
      }
    }


    //the next id I use needs to be greater than this
    next_id_ = cid + 1;

    std::vector<TaskId> task_list(new_comm_size);

    key_to_ranks_map::iterator it, end = key_map.end();
    //iterate map in sorted order
    int next_rank = 0;
    int my_new_rank = -1;
    for (it=key_map.begin(); it != end; ++it){
      std::list<int>& ranks = it->second;
      ranks.sort();
      std::list<int>::iterator rit, rend = ranks.end();
      for (rit=ranks.begin(); rit != rend; ++rit, ++next_rank){
        int comm_rank = *rit;
        TaskId tid = caller->peerTask(int(comm_rank));
        task_list[next_rank] = tid;
        if (comm_rank == caller->rank()){
          my_new_rank = next_rank;
        }
      }
    }
    MpiGroup* grp = new MpiGroup(std::move(task_list));
    ret = new MpiComm(cid, my_new_rank, grp, aid_, true/*delete this group*/);
  }

#if !SST_HG_DISTRIBUTED_MEMORY || SST_HG_MMAP_COLLECTIVES
  entry.refcount--;
  if (entry.refcount == 0){
#if SST_HG_MMAP_COLLECTIVES
    munmap(entry.buf, len);
    shm_unlink(fname);
#else
    delete[] result;
#endif
  }
#endif

  return ret;
}

MpiComm*
MpiCommFactory::createCart(MpiComm* caller, int ndims,
                              const int *dims, const int *periods, int reorder)
{
  MPI_Comm cid = commNewIdAgree(caller);

  //now find my rank
  int newrank = caller->group_->rankOfTask(caller->myTask());

  if (newrank >= 0) {
    return new MpiCommCart(cid, newrank, caller->group_,
                     aid_, ndims, dims, periods, reorder);
  } else {
    return MpiComm::comm_null;
  }
}

}
