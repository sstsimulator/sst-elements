// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_MEM_REQ_H
#define COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_MEM_REQ_H

struct MemReq {
    MemReq( Hermes::Vaddr addr, size_t length, int pid = -1) :
        addr(addr), length(length), pid(pid) {}

	Hermes::Vaddr addr;
	size_t length;
    int     pid;
};

#endif
