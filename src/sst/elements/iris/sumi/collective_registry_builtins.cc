// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iris/sumi/collective_registry.h>
#include <iris/sumi/allgather.h>
#include <iris/sumi/allreduce.h>
#include <iris/sumi/alltoall.h>
#include <iris/sumi/ring_allreduce.h>
#include <iris/sumi/reduce.h>
#include <iris/sumi/bcast.h>
#include <iris/sumi/scan.h>
#include <iris/sumi/gather.h>
#include <iris/sumi/scatter.h>

namespace SST::Iris::sumi {

SUMI_REGISTER_COLLECTIVE(allgather, "bruck", BruckAllgatherCollective,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)
SUMI_REGISTER_COLLECTIVE(allgather, "ring", RingAllgatherCollective,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(allreduce, "recdouble", WilkeHalvingAllreduce,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.fxn, a.cq_id, a.comm)
SUMI_REGISTER_COLLECTIVE(allreduce, "ring", RingAllreduce,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.fxn, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(alltoall, "bruck", BruckAlltoallCollective,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)
SUMI_REGISTER_COLLECTIVE(alltoall, "direct", DirectAlltoallCollective,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(reduce, "recdouble", WilkeHalvingReduce,
    a.engine, a.root, a.dst, a.src, a.nelems, a.type_size, a.tag, a.fxn, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(bcast, "btree", BinaryTreeBcastCollective,
    a.engine, a.root, a.dst, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(scan, "btree", SimultaneousBtreeScan,
    a.engine, a.dst, a.src, a.nelems, a.type_size, a.tag, a.fxn, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(gather, "btree", BtreeGather,
    a.engine, a.root, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)

SUMI_REGISTER_COLLECTIVE(scatter, "btree", BtreeScatter,
    a.engine, a.root, a.dst, a.src, a.nelems, a.type_size, a.tag, a.cq_id, a.comm)

}
