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

#pragma once

#include <libraries/system/system_library.h>
#include_next <unistd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

#ifndef SSTHG_NO_REPLACEMENTS
#define sleep       SST::Hg::SystemLibrary::ssthg_sleep
#endif

#define gethostname ssthg_gethostname
#define gethostid   ssthg_gethostid
#define alarm       ssthg_alarm

#ifdef __cplusplus
extern "C" {
#endif
int ssthg_gethostname(const char* name, size_t sz);

long ssthg_gethostid();

unsigned int ssthg_sleep(unsigned int secs);

unsigned int ssthg_sleepUntil(double t);

unsigned int ssthg_alarm(unsigned int);

#ifdef __cplusplus
}
#endif

