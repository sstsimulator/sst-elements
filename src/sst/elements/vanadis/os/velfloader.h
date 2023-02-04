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

#ifndef _H_VANADIS_OS_LOAD_ELF
#define _H_VANADIS_OS_LOAD_ELF

#include "sst/core/interfaces/stdMem.h"
#include "velf/velfinfo.h"
#include "sst/elements/mmu/mmu.h"
#include "os/vphysmemmanager.h"
#include "os/include/process.h"

namespace SST {
namespace Vanadis {

void loadElfFile( Output*, Interfaces::StandardMem*, MMU_Lib::MMU*, PhysMemManager*, VanadisELFInfo*, int hwThread, int page_size, OS::ProcessInfo* );
uint8_t* readElfPage( Output*, VanadisELFInfo*, int vpn, int page_size );

}
}
#endif
