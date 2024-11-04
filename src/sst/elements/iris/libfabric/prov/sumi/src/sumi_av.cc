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

/*
 * Copyright (c) 2015-2017 Cray Inc. All rights reserved.
 * Copyright (c) 2015-2017 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2019      Triad National Security, LLC.
 *                         All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//
// Address vector common code
//
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mercury/common/errors.h>

#include "sumi_prov.h"
#include "sumi_av.h"

EXTERN_C DIRECT_FN STATIC  int sumi_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
            uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sumi_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sumi_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sumi_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags);

EXTERN_C DIRECT_FN STATIC  int sumi_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen);

DIRECT_FN const char *sumi_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len);

static int sumi_av_close(fid_t fid);

/*******************************************************************************
 * FI_OPS_* data structures.
 ******************************************************************************/
static struct fi_ops_av sumi_av_ops = {
  .size = sizeof(struct fi_ops_av),
  .insert = sumi_av_insert,
  .insertsvc = sumi_av_insertsvc,
  .insertsym = sumi_av_insertsym,
  .remove = sumi_av_remove,
  .lookup = sumi_av_lookup,
  .straddr = sumi_av_straddr
};

static struct fi_ops sumi_fi_av_ops = {
  .size = sizeof(struct fi_ops),
  .close = sumi_av_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open
};

#define SUMI_MAX_ADDR_CHARS 7
#define SUMI_ADDR_FORMAT_STR "%7" PRIu64
#define SUMI_MAX_ADDR_LEN SUMI_MAX_ADDR_CHARS+1

/*
 * Note: this function (according to WG), is not intended to
 * typically be used in the critical path for messaging/rma/amo
 * requests
 */
EXTERN_C DIRECT_FN STATIC  int sumi_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen)
{
  sumi_fid_av* av_impl = (sumi_fid_av*) av;
  if (av_impl->domain->addr_format == FI_ADDR_SSTMAC){
    if (*addrlen < sizeof(uint64_t)){
      return -FI_EINVAL;
    }
    uint64_t* addr_int = (uint64_t*) addr;
    *addr_int = fi_addr;
  } else if (av_impl->domain->addr_format == FI_ADDR_STR){
    if (*addrlen < SUMI_MAX_ADDR_LEN){
      return -FI_EINVAL;
    }
    //all addresses are just strings of the rank
    snprintf((char*)addr, SUMI_MAX_ADDR_LEN, SUMI_ADDR_FORMAT_STR, fi_addr);
    *addrlen = SUMI_MAX_ADDR_LEN;
  } else {
    sst_hg_abort_printf("internal error: got addr format that isn't SSTMAC or STR");
  }
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
				    uint64_t flags, void *context)
{
  sumi_fid_av* av_impl = (sumi_fid_av*) av;
  if (av_impl->domain->addr_format == FI_ADDR_STR){
    char* addr_str = (char*) addr;
    for (int i=0; i < count; ++i){
      long long rank = std::atoll(addr_str);
      fi_addr[i] = rank;
      addr_str += SUMI_MAX_ADDR_LEN;
    }
  } else if (av_impl->domain->addr_format == FI_ADDR_SSTMAC) {
    uint64_t* addr_list = (uint64_t*) addr;
    for (int i=0; i < count; ++i){
      fi_addr[i] = addr_list[i];
    }
  } else {
    sst_hg_abort_printf("internal error: got addr format that isn't SSTMAC or STR");
  }
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags)
{
  //we don't need to do anything to remove stuff
  return FI_SUCCESS;
}

DIRECT_FN const char *sumi_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len)
{
  sumi_fid_av* av_impl = (sumi_fid_av*) av;
  char* ret = new char[SUMI_MAX_ADDR_LEN];

  if (av_impl->domain->addr_format == FI_ADDR_STR){
    ::strcpy(ret, (const char*)addr);
  } else if (av_impl->domain->addr_format == FI_ADDR_SSTMAC) {
    uint64_t* addr_ptr = (uint64_t*) addr;
    uint32_t rank = ADDR_RANK(*addr_ptr);
    uint16_t cq = ADDR_CQ(*addr_ptr);
    uint16_t rx = ADDR_QUEUE(*addr_ptr);
    snprintf(ret, 512, "%" PRIu32 ".%" PRIu16 ".%" PRIu16,
            rank, cq, rx);
  } else {
    sst_hg_abort_printf("internal error: got addr format that isn't SSTMAC or STR");
  }
  *len = ::strlen(ret);
  return ret;
}

static int sumi_av_close(fid_t fid)
{
  sumi_fid_av* av_impl = (sumi_fid_av*) fid;
  free(av_impl);
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sumi_av_bind(struct fid_av *av, struct fid *fid, uint64_t flags)
{
	return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sumi_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
			   struct fid_av **av, void *context)
{
  sumi_fid_av* av_impl = (sumi_fid_av*) calloc(1, sizeof(sumi_fid_av));
  av_impl->av_fid.fid.fclass = FI_CLASS_AV;
  *av = (fid_av*) av_impl;
  return FI_SUCCESS;
}
