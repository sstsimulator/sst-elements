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
*/


#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <assert.h>

#include "sstmac.h"

static int sstmac_srx_close(fid_t fid);
static int sstmac_stx_close(fid_t fid);
static int sstmac_domain_close(fid_t fid);
extern "C" DIRECT_FN  int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags);
static int
sstmac_domain_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context);

EXTERN_C DIRECT_FN STATIC  int sstmac_stx_open(struct fid_domain *dom,
				   struct fi_tx_attr *tx_attr,
				   struct fid_stx **stx, void *context);
extern "C" DIRECT_FN  int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags);
extern "C" DIRECT_FN  int sstmac_srx_context(struct fid_domain *domain,
			       struct fi_rx_attr *attr,
			       struct fid_ep **rx_ep, void *context);
extern "C" DIRECT_FN  int sstmac_domain_open(struct fid_fabric *fabric, struct fi_info *info,
			       struct fid_domain **dom, void *context);


static struct fi_ops_ep sstmac_srx_ep_base_ops = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = fi_no_cancel,
  .getopt = fi_no_getopt,
  .setopt = fi_no_setopt,
  .tx_ctx = fi_no_tx_ctx,
  .rx_ctx = fi_no_rx_ctx,
  .rx_size_left = fi_no_rx_size_left,
  .tx_size_left = fi_no_tx_size_left,
};

static struct fi_ops_cm sstmac_srx_cm_ops = {
  .size = sizeof(struct fi_ops_cm),
  .setname = fi_no_setname,
  .getname = fi_no_getname,
  .getpeer = fi_no_getpeer,
  .connect = fi_no_connect,
  .listen = fi_no_listen,
  .accept = fi_no_accept,
  .reject = fi_no_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};

static struct fi_ops_rma sstmac_srx_rma_ops = {
  .size = sizeof(struct fi_ops_rma),
  .read = fi_no_rma_read,
  .readv = fi_no_rma_readv,
  .readmsg = fi_no_rma_readmsg,
  .write = fi_no_rma_write,
  .writev = fi_no_rma_writev,
  .writemsg = fi_no_rma_writemsg,
  .inject = fi_no_rma_inject,
  .writedata = fi_no_rma_writedata,
  .injectdata = fi_no_rma_injectdata,
};

static struct fi_ops_atomic sstmac_srx_atomic_ops = {
  .size = sizeof(struct fi_ops_atomic),
  .write = fi_no_atomic_write,
  .writev = fi_no_atomic_writev,
  .writemsg = fi_no_atomic_writemsg,
  .inject = fi_no_atomic_inject,
  .readwrite = fi_no_atomic_readwrite,
  .readwritev = fi_no_atomic_readwritev,
  .readwritemsg = fi_no_atomic_readwritemsg,
  .compwrite = fi_no_atomic_compwrite,
  .compwritev = fi_no_atomic_compwritev,
  .compwritemsg = fi_no_atomic_compwritemsg,
  .writevalid = fi_no_atomic_writevalid,
  .readwritevalid = fi_no_atomic_readwritevalid,
  .compwritevalid = fi_no_atomic_compwritevalid,
};

static struct fi_ops sstmac_srx_ep_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_srx_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open,
};


static struct fi_ops sstmac_stx_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_stx_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open
};

static struct fi_ops sstmac_domain_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_domain_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = sstmac_domain_ops_open
};

static struct fi_ops_mr sstmac_domain_mr_ops = {
  .size = sizeof(struct fi_ops_mr),
  .reg = sstmac_mr_reg,
  .regv = sstmac_mr_regv,
  .regattr = sstmac_mr_regattr,
};

static struct fi_ops_domain sstmac_domain_ops = {
  .size = sizeof(struct fi_ops_domain),
  .av_open = sstmac_av_open,
  .cq_open = sstmac_cq_open,
  .endpoint = sstmac_ep_open,
  .scalable_ep = sstmac_sep_open,
  .cntr_open = sstmac_cntr_open,
  .poll_open = fi_no_poll_open,
  .stx_ctx = sstmac_stx_open,
  .srx_ctx = fi_no_srx_context
};

#define SSTMAC_MR_MODE_DEFAULT FI_MR_BASIC
#define SSTMAC_NUM_PTAGS 256

/*******************************************************************************
 * API function implementations.
 ******************************************************************************/

static int sstmac_srx_close(fid_t fid)
{
  return 0;
}

static int sstmac_stx_close(fid_t fid)
{
#if 0
  struct sstmac_fid_stx *stx;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

  stx = container_of(fid, struct sstmac_fid_stx, stx_fid.fid);
  if (stx->stx_fid.fid.fclass != FI_CLASS_STX_CTX)
    return -FI_EINVAL;

  _sstmac_ref_put(stx->domain);
  _sstmac_ref_put(stx);
#endif
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_stx_open(struct fid_domain *dom,
				   struct fi_tx_attr *tx_attr,
				   struct fid_stx **stx, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_domain *domain;
	struct sstmac_fid_stx *stx_priv;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

	domain = container_of(dom, struct sstmac_fid_domain, domain_fid.fid);
	if (domain->domain_fid.fid.fclass != FI_CLASS_DOMAIN) {
		ret = -FI_EINVAL;
		goto err;
	}

  stx_priv = (sstmac_fid_stx*) calloc(1, sizeof(*stx_priv));
	if (!stx_priv) {
		ret = -FI_ENOMEM;
		goto err;
	}

	stx_priv->domain = domain;
	stx_priv->auth_key = NULL;
	stx_priv->nic = NULL;

	_sstmac_ref_init(&stx_priv->ref_cnt, 1, __stx_destruct);

	_sstmac_ref_get(stx_priv->domain);

	stx_priv->stx_fid.fid.fclass = FI_CLASS_STX_CTX;
	stx_priv->stx_fid.fid.context = context;
  stx_priv->stx_fid.fid.ops = &sstmac_stx_ops;
	stx_priv->stx_fid.ops = NULL;
	domain->num_allocd_stxs++;

	*stx = &stx_priv->stx_fid;
#endif
err:
	return ret;
}



static int sstmac_domain_close(fid_t fid)
{
	int ret = FI_SUCCESS, references_held;
#if 0
	struct sstmac_fid_domain *domain;
	int i;
	struct sstmac_mr_cache_info *info;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

	domain = container_of(fid, struct sstmac_fid_domain, domain_fid.fid);
	if (domain->domain_fid.fid.fclass != FI_CLASS_DOMAIN) {
		ret = -FI_EINVAL;
		goto err;
	}

  for (i = 0; i < SSTMAC_NUM_PTAGS; i++) {
		info = &domain->mr_cache_info[i];

		if (!domain->mr_cache_info[i].inuse)
			continue;

		/* before checking the refcnt,
		 * flush the memory registration cache
		 */
		if (info->mr_cache_ro) {
			fastlock_acquire(&info->mr_cache_lock);
			ret = _sstmac_mr_cache_flush(info->mr_cache_ro);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_DOMAIN,
					  "failed to flush memory cache on domain close\n");
				fastlock_release(&info->mr_cache_lock);
				goto err;
			}
			fastlock_release(&info->mr_cache_lock);
		}

		if (info->mr_cache_rw) {
			fastlock_acquire(&info->mr_cache_lock);
			ret = _sstmac_mr_cache_flush(info->mr_cache_rw);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_DOMAIN,
					  "failed to flush memory cache on domain close\n");
				fastlock_release(&info->mr_cache_lock);
				goto err;
			}
			fastlock_release(&info->mr_cache_lock);
		}
	}

	/*
	 * if non-zero refcnt, there are eps, mrs, and/or an eq associated
	 * with this domain which have not been closed.
	 */

	references_held = _sstmac_ref_put(domain);

	if (references_held) {
    SSTMAC_INFO(FI_LOG_DOMAIN, "failed to fully close domain due to "
			  "lingering references. references=%i dom=%p\n",
			  references_held, domain);
	}

  SSTMAC_INFO(FI_LOG_DOMAIN, "sstmac_domain_close invoked returning %d\n",
		  ret);
#endif
err:
	return ret;
}

/*
 * sstmac_domain_ops provides a means for an application to better
 * control allocation of underlying aries resources associated with
 * the domain.  Examples will include controlling size of underlying
 * hardware CQ sizes, max size of RX ring buffers, etc.
 */

static const uint32_t default_msg_rendezvous_thresh = 16*1024;
static const uint32_t default_rma_rdma_thresh = 8*1024;
static const uint32_t default_ct_init_size = 64;
static const uint32_t default_ct_max_size = 16384;
static const uint32_t default_ct_step = 2;
static const uint32_t default_vc_id_table_capacity = 128;
//static const uint32_t default_mbox_page_size = SSTMAC_PAGE_2MB;
static const uint32_t default_mbox_num_per_slab = 2048;
static const uint32_t default_mbox_maxcredit = 64;
static const uint32_t default_mbox_msg_maxsize = 16384;
/* rx cq bigger to avoid having to deal with rx overruns so much */
static const uint32_t default_rx_cq_size = 16384;
static const uint32_t default_tx_cq_size = 2048;
static const uint32_t default_max_retransmits = 5;
static const int32_t default_err_inject_count = 0;
static const uint32_t default_dgram_progress_timeout = 100;
static const uint32_t default_eager_auto_progress = 0;

extern "C" DIRECT_FN  int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags)
{
	return -FI_ENOSYS;
}

static int
sstmac_domain_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context)
{
  return -FI_EINVAL;;
}



extern "C" DIRECT_FN  int sstmac_domain_open(struct fid_fabric *fabric, struct fi_info *info,
             struct fid_domain **dom_ptr, void *context)
{
  if (info->domain_attr->mr_mode & FI_MR_SCALABLE){
    return -FI_EINVAL;
  }

  sstmac_fid_domain* domain = (sstmac_fid_domain*) calloc(1, sizeof(sstmac_fid_domain));
  sstmac_fid_fabric* fabric_impl = (sstmac_fid_fabric*) fabric;
  //we don't really have to do a ton of work here
  //memory registration is not an issue
  //and we always make progress in the background without the app requiring an extra progress thread
  domain->domain_fid.fid.fclass = FI_CLASS_DOMAIN;
  domain->domain_fid.fid.context = context;
  domain->domain_fid.fid.ops = &sstmac_domain_fi_ops;
  domain->domain_fid.ops = &sstmac_domain_ops;
  domain->domain_fid.mr = &sstmac_domain_mr_ops;
  domain->fabric = fabric_impl;
  domain->addr_format = info->addr_format;
  *dom_ptr = (fid_domain*) domain;
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sstmac_srx_context(struct fid_domain *domain,
			       struct fi_rx_attr *attr,
			       struct fid_ep **rx_ep, void *context)
{
  sstmac_fid_srx* srx_impl = (sstmac_fid_srx*) calloc(1, sizeof(sstmac_fid_srx));
  srx_impl->ep_fid.fid.fclass = FI_CLASS_SRX_CTX;
  srx_impl->ep_fid.fid.context = context;
  srx_impl->ep_fid.fid.ops = &sstmac_srx_ep_ops;
  srx_impl->ep_fid.ops = &sstmac_srx_ep_base_ops;
  srx_impl->ep_fid.cm = &sstmac_srx_cm_ops;
  srx_impl->ep_fid.rma = &sstmac_srx_rma_ops;
  srx_impl->ep_fid.atomic = &sstmac_srx_atomic_ops;
  srx_impl->domain = (sstmac_fid_domain*) domain;
  *rx_ep = (fid_ep*) srx_impl;
	return -FI_ENOSYS;
}


