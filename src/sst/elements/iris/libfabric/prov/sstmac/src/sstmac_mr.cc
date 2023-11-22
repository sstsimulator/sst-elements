/**
Copyright 2009-2020 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2020, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <stdlib.h>
#include <string.h>

#include "sstmac.h"

static int __sstmac_mr_refresh(struct sstmac_fid_mem_desc *desc,
	uint64_t addr, uint64_t len);
static int fi_sstmac_mr_close(fid_t fid);
static int fi_sstmac_mr_control(struct fid *fid, int command, void *arg);
extern "C" DIRECT_FN  int sstmac_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context);
extern "C" DIRECT_FN  int sstmac_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context);
extern "C" DIRECT_FN  int sstmac_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr);
extern "C" DIRECT_FN  int sstmac_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags);

/* global declarations */
/* memory registration operations */
static struct fi_ops fi_sstmac_mr_ops = {
	.size = sizeof(struct fi_ops),
	.close = fi_sstmac_mr_close,
	.bind = fi_no_bind,
	.control = fi_sstmac_mr_control,
	.ops_open = fi_no_ops_open,
};

extern "C" DIRECT_FN  int sstmac_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = context;
  mr_impl->fid.ops = &fi_sstmac_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = (uint64_t) buf;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sstmac_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = context;
  mr_impl->fid.ops = &fi_sstmac_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = (uint64_t) iov;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sstmac_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr)
{
  fid_mr* mr_impl = (fid_mr*) calloc(1, sizeof(fid_mr));
  mr_impl->fid.fclass = FI_CLASS_MR;
  mr_impl->fid.context = attr->context;
  mr_impl->fid.ops = &fi_sstmac_mr_ops;
  mr_impl->mem_desc = mr_impl; //just point to self
  mr_impl->key = attr->requested_key;
  //yeah, sure, great, always succeeds
  return FI_SUCCESS;
}

static int fi_sstmac_mr_control(struct fid *fid, int command, void *arg)
{
  //no mem control operations need to be done in the simulator
  return -FI_EOPNOTSUPP;
}

static int fi_sstmac_mr_close(fid_t fid)
{
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sstmac_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
	return -FI_ENOSYS;
}

