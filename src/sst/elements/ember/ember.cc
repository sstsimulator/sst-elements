// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "emberlinearmap.h"
#include "embercustommap.h" //NetworkSim: added custom rank map

#include "mpi/motifs/emberinit.h"
#include "mpi/motifs/emberfini.h"
#include "mpi/motifs/embernull.h"

#include "shmem/motifs/emberShmemTest.h"
#include "shmem/motifs/emberShmemWait.h"
#include "shmem/motifs/emberShmemWaitUntil.h"
#include "shmem/motifs/emberShmemPut.h"
#include "shmem/motifs/emberShmemGet.h"
#include "shmem/motifs/emberShmemGetNBI.h"
#include "shmem/motifs/emberShmemPutv.h"
#include "shmem/motifs/emberShmemGetv.h"
#include "shmem/motifs/emberShmemFadd.h"
#include "shmem/motifs/emberShmemAdd.h"
#include "shmem/motifs/emberShmemCswap.h"
#include "shmem/motifs/emberShmemSwap.h"
#include "shmem/motifs/emberShmemBarrierAll.h"
#include "shmem/motifs/emberShmemBarrier.h"
#include "shmem/motifs/emberShmemBroadcast.h"
#include "shmem/motifs/emberShmemCollect.h"
#include "shmem/motifs/emberShmemFcollect.h"
#include "shmem/motifs/emberShmemAlltoall.h"
#include "shmem/motifs/emberShmemAlltoalls.h"
#include "shmem/motifs/emberShmemReduction.h"
#include "shmem/motifs/emberShmemRing.h"
#include "shmem/motifs/emberShmemRing2.h"
#include "shmem/motifs/emberShmemAtomicInc.h"
#include "shmem/motifs/emberShmemAtomicIncV2.h"
#include "shmem/motifs/emberShmemFAM_Get2.h"
#include "shmem/motifs/emberShmemFAM_AtomicInc.h"
