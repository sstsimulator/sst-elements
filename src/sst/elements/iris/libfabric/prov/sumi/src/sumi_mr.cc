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


#include <stdlib.h>
#include <string.h>

#include "sumi_prov.h"

static int __sumi_mr_refresh(struct sumi_fid_mem_desc *desc,
	uint64_t addr, uint64_t len);
static int fi_sumi_mr_close(fid_t fid);
static int fi_sumi_mr_control(struct fid *fid, int command, void *arg);
extern "C" DIRECT_FN  int sumi_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context);
extern "C" DIRECT_FN  int sumi_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context);
extern "C" DIRECT_FN  int sumi_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr);
extern "C" DIRECT_FN  int sumi_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags);

/* global declarations */
/* memory registration operations */
static struct fi_ops fi_sumi_mr_ops = {
	.size = sizeof(struct fi_ops),
	.close = fi_sumi_mr_close,
	.bind = fi_no_bind,
	.control = fi_sumi_mr_control,
	.ops_open = fi_no_ops_open,
};

extern "C" DIRECT_FN  int sumi_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = context;
  mr_impl->fid.ops = &fi_sumi_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = (uint64_t) buf;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sumi_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = context;
  mr_impl->fid.ops = &fi_sumi_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = (uint64_t) iov;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sumi_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = attr->context;
  mr_impl->fid.ops = &fi_sumi_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = attr->requested_key;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

static int fi_sumi_mr_control(struct fid *fid, int command, void *arg)
{
  //no mem control operations need to be done in the simulator
  return -FI_EOPNOTSUPP;
}

static int fi_sumi_mr_close(fid_t fid)
{
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sumi_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
	return -FI_ENOSYS;
}

