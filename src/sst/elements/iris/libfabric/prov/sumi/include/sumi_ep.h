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

#ifndef _SUMI_EP_H_
#define _SUMI_EP_H_

#include "sumi_prov.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int sumi_pep_open(struct fid_fabric *fabric,
		  struct fi_info *info, struct fid_pep **pep,
		  void *context);

int sumi_scalable_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

int sumi_pep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

ssize_t sumi_cancel(fid_t fid, void *context);

int sumi_getopt(fid_t fid, int level, int optname,
                   void *optval, size_t *optlen);

int sumi_setopt(fid_t fid, int level, int optname,
                   const void *optval, size_t optlen);

DIRECT_FN int sumi_ep_atomic_valid(struct fid_ep *ep,
				   enum fi_datatype datatype,
				   enum fi_op op, size_t *count);

DIRECT_FN int sumi_ep_fetch_atomic_valid(struct fid_ep *ep,
					 enum fi_datatype datatype,
					 enum fi_op op, size_t *count);

DIRECT_FN int sumi_ep_cmp_atomic_valid(struct fid_ep *ep,
				       enum fi_datatype datatype,
				       enum fi_op op, size_t *count);

#ifdef __cplusplus
}
#endif

#endif
