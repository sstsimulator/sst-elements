// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SUMI_WAIT_H_
#define _SUMI_WAIT_H_

#include <rdma/fi_eq.h>
#include <ofi_list.h>

#ifdef __cplusplus
extern "C" {
#endif

int sumi_wait_open(struct fid_fabric *fabric, struct fi_wait_attr *attr,
		   struct fid_wait **waitset);
int sumi_wait_close(struct fid *wait);
int sumi_wait_wait(struct fid_wait *wait, int timeout);

void sstmaci_add_wait(struct fid_wait *wait, struct fid *wait_obj);

#ifdef __cplusplus
}
#endif

#endif
