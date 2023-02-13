// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _VLOADPAGE_H
#define _VLOADPAGE_H



#include "sst/core/interfaces/stdMem.h"
#include "sst/core/output.h"
#include "velf/velfinfo.h"
#include "sst/elements/mmu/mmu.h"
#include "os/vphysmemmanager.h"


namespace SST {
namespace Vanadis {

void loadPages( SST::Output* output, SST::Interfaces::StandardMem* mem_if, MMU_Lib::MMU* mmu,
            PhysMemManager* memMgr, unsigned pid, uint64_t virtAddr, std::vector<uint8_t>& buffer, uint64_t flags, int page_size );

}
}

#endif
