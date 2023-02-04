// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _BASE_H
#define _BASE_H

#include <rdmaNicHostInterface.h>
void writeCmd( NicCmd* cmd );
void base_init();
int base_n_pes(); 
int base_my_pe();
void base_make_progress();
Addr_t getNicBase(); 

#endif
