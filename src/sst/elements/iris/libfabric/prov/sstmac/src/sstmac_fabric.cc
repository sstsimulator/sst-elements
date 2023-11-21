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
/*
 * Copyright (c) 2014 Intel Corporation, Inc.  All rights reserved.
 * Copyright (c) 2015-2017 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2015-2017 Cray Inc. All rights reserved.
 * Copyrigth (c) 2019      Triad National Security, LLC. All rights
 *                         reserved.
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <ofi_util.h>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_errno.h>
#include "ofi_prov.h"

#include "sstmac.h"
#include "sstmac_wait.h"

#include <sstmac_sumi.hpp>

/* check if only one bit of a set is enabled, when one is required */
#define IS_EXCLUSIVE(x) \
	((x) && !((x) & ((x)-1)))

/* optional basic bits */
#define SSTMAC_MR_BASIC_OPT \
	(FI_MR_LOCAL)

/* optional scalable bits */
#define SSTMAC_MR_SCALABLE_OPT \
	(FI_MR_LOCAL)

/* required basic bits */
#define SSTMAC_MR_BASIC_REQ \
	(FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY)

/* required scalable bits */
#define SSTMAC_MR_SCALABLE_REQ \
	(FI_MR_MMU_NOTIFY)

#define SSTMAC_MR_BASIC_BITS \
	(SSTMAC_MR_BASIC_OPT | SSTMAC_MR_BASIC_REQ)

#define SSTMAC_MR_SCALABLE_BITS \
	(SSTMAC_MR_SCALABLE_OPT | SSTMAC_MR_SCALABLE_REQ)

#define SSTMAC_DEFAULT_USER_REGISTRATION_LIMIT 192
#define SSTMAC_DEFAULT_PROV_REGISTRATION_LIMIT 64
#define SSTMAC_DEFAULT_SHARED_MEMORY_TIMEOUT 30

int sstmac_default_user_registration_limit = SSTMAC_DEFAULT_USER_REGISTRATION_LIMIT;
int sstmac_default_prov_registration_limit = SSTMAC_DEFAULT_PROV_REGISTRATION_LIMIT;
uint32_t sstmac_wait_shared_memory_timeout = SSTMAC_DEFAULT_SHARED_MEMORY_TIMEOUT;

/* assume that the user will open additional fabrics later and that
   ptag information will need to be retained for the lifetime of the
   process. If the user sets this value, we can assume that they
   intend to be done with libfabric when the last fabric instance
   closes so that we can free the ptag information. */
int sstmac_dealloc_aki_on_fabric_close = 0;

#define SSTMAC_MAJOR_VERSION 1
#define SSTMAC_MINOR_VERSION 0

const struct fi_fabric_attr sstmac_fabric_attr = {
	.fabric = NULL,
	.name = NULL,
	.prov_name = NULL,
	.prov_version = FI_VERSION(SSTMAC_MAJOR_VERSION, SSTMAC_MINOR_VERSION),
};

extern "C" DIRECT_FN  int sstmac_fabric_trywait(struct fid_fabric *fabric, struct fid **fids, int count)
{
	return -FI_ENOSYS;
}

static int sstmac_fabric_close(fid_t fid);
static int sstmac_fab_ops_open(struct fid *fid, const char *ops_name,
        uint64_t flags, void **ops, void *context);

static struct fi_ops sstmac_fab_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_fabric_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open,
};

static struct fi_ops_fabric sstmac_fab_ops = {
  .size = sizeof(struct fi_ops_fabric),
  .domain = sstmac_domain_open,
  .passive_ep = sstmac_pep_open,
  .eq_open = sstmac_eq_open,
  .wait_open = sstmac_wait_open,
  .trywait = sstmac_fabric_trywait
};

static int sstmac_fabric_close(fid_t fid)
{
  //TODO
	return FI_SUCCESS;
}

/*
 * define methods needed for the SSTMAC fabric provider
 */
static int sstmac_fabric_open(struct fi_fabric_attr *attr,
			    struct fid_fabric **fabric,
			    void *context)
{
  struct sstmac_fid_fabric* fab = (sstmac_fid_fabric*) calloc(1, sizeof(sstmac_fid_fabric));
  //might as well be consistent about memory issues
  if (!fab){
    return -ENOMEM;
  }

  fab->fab_fid.fid.fclass = FI_CLASS_FABRIC;
  fab->fab_fid.fid.context = context;
  fab->fab_fid.fid.ops = &sstmac_fab_fi_ops;
  fab->fab_fid.ops = &sstmac_fab_ops;

  FabricTransport* tport = sstmac_fabric();
  tport->init();
  fab->tport = (sstmac_fid_tport*) tport;

  *fabric = &fab->fab_fid;
	return FI_SUCCESS;
}

const char sstmac_fab_name[] = "sstmac";
const char sstmac_dom_name[] = "sstmac";

static void sstmac_fini(void)
{
}

#define SSTMAC_EP_CAPS   \
  (FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS | \
  FI_DIRECTED_RECV | FI_READ | FI_NAMED_RX_CTX | \
  FI_WRITE | FI_SEND | FI_RECV | FI_REMOTE_READ | FI_REMOTE_WRITE)

static struct fi_info *sstmac_allocinfo(void)
{
  struct fi_info *sstmac_info;

  sstmac_info = fi_allocinfo();
  if (sstmac_info == nullptr) {
    return nullptr;
  }

  sstmac_info->caps = SSTMAC_EP_PRIMARY_CAPS;
  sstmac_info->tx_attr->op_flags = 0;
  sstmac_info->rx_attr->op_flags = 0;
  sstmac_info->ep_attr->type = FI_EP_RDM;
  sstmac_info->ep_attr->protocol = FI_PROTO_SSTMAC;
  sstmac_info->ep_attr->max_msg_size = SSTMAC_MAX_MSG_SIZE;
  sstmac_info->ep_attr->mem_tag_format = FI_TAG_GENERIC;
  sstmac_info->ep_attr->tx_ctx_cnt = 1;
  sstmac_info->ep_attr->rx_ctx_cnt = 1;


  sstmac_info->domain_attr->name = strdup("sstmac");
  sstmac_info->domain_attr->threading = FI_THREAD_SAFE;
  sstmac_info->domain_attr->control_progress = FI_PROGRESS_AUTO;
  sstmac_info->domain_attr->data_progress = FI_PROGRESS_AUTO;
  sstmac_info->domain_attr->av_type = FI_AV_TABLE;

  //says the applicatoin is protected from buffer overruns on things like CQs
  //this is always true in SSTMAC since we push back on dynamic bufers
  sstmac_info->domain_attr->resource_mgmt = FI_RM_ENABLED;

  //compatibility enum, basically just says that you need to request via full virtual addr,
  //that memory regions must be allocated (backed by phsical pages), and that keys are allocated by provider
  sstmac_info->domain_attr->mr_mode = FI_MR_BASIC;

  //the size of the mr_key, just make it 64-bit for now
  sstmac_info->domain_attr->mr_key_size = sizeof(uint64_t);
  //the size of the data written to completion queues as part of an event, make 64-bit for now
  sstmac_info->domain_attr->cq_data_size = sizeof(uint64_t);
  //just set to a big number - the maximum number of completion queues
  sstmac_info->domain_attr->cq_cnt = 1000;
  //set to largest possible 32-bit number - the maxmimum number of endpoints
  sstmac_info->domain_attr->ep_cnt = std::numeric_limits<uint32_t>::max();
  //just set to a big number - the maximnum number of tx contexts that can be handled
  sstmac_info->domain_attr->tx_ctx_cnt = 1000;
  //just set to a big number - the maximum number of tx contexts that can be handled
  sstmac_info->domain_attr->rx_ctx_cnt = 1000;
  //just set to a big number
  sstmac_info->domain_attr->max_ep_tx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_rx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_stx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_srx_ctx = 1000;
  //just set to a big number - the maximum number of different completion counters
  sstmac_info->domain_attr->cntr_cnt = 1000;
  //we don't really do anything with mr_iov, so allow a big number
  sstmac_info->domain_attr->mr_iov_limit = 10000;
  //we support everything - send local, send remote, share av tables
  sstmac_info->domain_attr->caps = SSTMAC_DOM_CAPS;
  //we place no restrictions on domains having the exact same capabilities to communicate
  sstmac_info->domain_attr->mode = 0;
  //we don't really do anything with keys, so...
  sstmac_info->domain_attr->auth_key = 0;
  sstmac_info->domain_attr->max_err_data = sizeof(uint64_t);
  //we have no limit on the number of memory regions
  sstmac_info->domain_attr->mr_cnt = std::numeric_limits<uint32_t>::max();



  sstmac_info->next = NULL;
  sstmac_info->addr_format = FI_ADDR_STR;
  sstmac_info->src_addrlen = sizeof(struct sstmac_ep_name);
  sstmac_info->dest_addrlen = sizeof(struct sstmac_ep_name);
  sstmac_info->src_addr = NULL;
  sstmac_info->dest_addr = NULL;


  sstmac_info->tx_attr->caps = SSTMAC_EP_PRIMARY_CAPS;
  sstmac_info->tx_attr->msg_order = FI_ORDER_SAS;
  sstmac_info->tx_attr->comp_order = FI_ORDER_NONE;
  sstmac_info->tx_attr->inject_size = SSTMAC_INJECT_SIZE;
  sstmac_info->tx_attr->size = SSTMAC_TX_SIZE_DEFAULT;
  sstmac_info->tx_attr->iov_limit = SSTMAC_MAX_MSG_IOV_LIMIT;
  sstmac_info->tx_attr->rma_iov_limit = SSTMAC_MAX_RMA_IOV_LIMIT;

  sstmac_info->rx_attr->caps = SSTMAC_EP_PRIMARY_CAPS;
  sstmac_info->rx_attr->msg_order = FI_ORDER_SAS;
  sstmac_info->rx_attr->comp_order = FI_ORDER_NONE;
  sstmac_info->rx_attr->size = SSTMAC_RX_SIZE_DEFAULT;
  sstmac_info->rx_attr->iov_limit = SSTMAC_MAX_MSG_IOV_LIMIT;

  return sstmac_info;
}

static int sstmac_ep_getinfo(enum fi_ep_type ep_type, uint32_t version,
      const char *node, const char *service,
      uint64_t flags, const struct fi_info *hints,
      struct fi_info **info_ptr)
{
  fi_info* info = sstmac_allocinfo();
  if (!info){
    //practically never a issue, but might as well
    //be super safe and match other transports
    return -FI_ENOMEM;
  }

  ErrorDeallocate err(info, [](void* ptr){
    fi_info* info = (fi_info*) ptr;
    fi_freeinfo(info);
  });

  //if hints are given, the ep types better be compatible
  if ((hints && hints->ep_attr) &&
      (hints->ep_attr->type != FI_EP_UNSPEC &&
       hints->ep_attr->type != ep_type)) {
    return -FI_ENODATA;
  }

  if (hints){
    //we are being asked for an endpoint with particular features
    //let's ensure that what was requested is actually possible
    if (hints->addr_format == FI_ADDR_STR){
      info->addr_format = FI_ADDR_STR;
    } else if (hints->addr_format == FI_ADDR_SSTMAC){
      info->addr_format = FI_ADDR_SSTMAC;
    } else {
      return -FI_ENODATA;
    }

    if (hints->ep_attr){
      //validate the request endpoint attrs are valid
      switch(hints->ep_attr->protocol){
        case FI_PROTO_UNSPEC:
        case FI_PROTO_SSTMAC:
          break; //this better be unspecified or SSTMAC-specific
        default:
          return -FI_ENODATA;
      }

      if ((hints->ep_attr->tx_ctx_cnt > SSTMAC_SEP_MAX_CNT) &&
        (hints->ep_attr->tx_ctx_cnt != FI_SHARED_CONTEXT)) {
          //exceeds the maximum number of contexts and is not
          //asking for a single shared context
          return -FI_ENODATA;
      } else if (hints->ep_attr->tx_ctx_cnt != 0) {
        //valid requested value
        info->ep_attr->tx_ctx_cnt = hints->ep_attr->tx_ctx_cnt;
      }

      if ((hints->ep_attr->rx_ctx_cnt > SSTMAC_SEP_MAX_CNT) &&
        (hints->ep_attr->rx_ctx_cnt != FI_SHARED_CONTEXT)) {
          //exceeds the maximum number of contexts and is not
          //asking for a single shared context
          return -FI_ENODATA;
      } else if (hints->ep_attr->rx_ctx_cnt != 0){
        //valid requested value
        info->ep_attr->rx_ctx_cnt = hints->ep_attr->rx_ctx_cnt;
      }

      if (hints->ep_attr->max_msg_size > SSTMAC_MAX_MSG_SIZE){
        return -FI_ENODATA;
      }
    }
    //finish ep_attr check

    if (hints->mode){
      if ( (hints->mode & SSTMAC_FAB_MODES) != SSTMAC_FAB_MODES ){
        //the application explicitly does not support modes
        //that the sstmac provider uses
        return -FI_ENODATA;
      }
      //tell the application which modes it supports
      //are not actually needed for high performance
      info->mode = info->mode & ~SSTMAC_FAB_MODES_CLEAR;
    }

    if (hints->caps){
      if ((hints->caps & SSTMAC_EP_CAPS) != hints->caps){
        // app is requesting capabilities I don't have
        return -FI_ENODATA;
      } else {
        info->caps = hints->caps;
      }
    }


    if (hints->tx_attr){
      if ((hints->tx_attr->op_flags & SSTMAC_EP_OP_FLAGS) != hints->tx_attr->op_flags){
        //the app is requesting operations I don't support
        return -FI_ENODATA;
      }
      if (hints->tx_attr->inject_size > SSTMAC_MAX_INJECT_SIZE){
        return -FI_ENODATA;
      }

      //we current do not support any ordering relationships
      //if the app requires ordering, we can't help it
      if (hints->tx_attr->comp_order && hints->tx_attr->comp_order != FI_ORDER_NONE){
        return -FI_ENODATA;
      }
      if (hints->tx_attr->msg_order && hints->tx_attr->msg_order != FI_ORDER_NONE){
        return -FI_ENODATA;
      }

      if (hints->tx_attr->caps){
        if (hints->caps){
          if ((hints->caps & hints->rx_attr->caps) != hints->rx_attr->caps){
            //the endpoint capabilities are LOWER than the tx capabilities, not okay
            return -FI_ENODATA;
          }
        }

        if ((hints->rx_attr->caps & SSTMAC_EP_CAPS) != hints->rx_attr->caps){
          //requesting more capabilities than I have
          return -FI_ENODATA;
        }
      }
    }

    if (hints->rx_attr){
      if ((hints->rx_attr->op_flags & SSTMAC_EP_OP_FLAGS) != hints->tx_attr->op_flags){
        //the app is requesting operations I don't support
        return -FI_ENODATA;
      }

      //we current do not support any ordering relationships
      //if the app requires ordering, we can't help it
      if (hints->rx_attr->comp_order && hints->rx_attr->comp_order != FI_ORDER_NONE){
        return -FI_ENODATA;
      }
      if (hints->rx_attr->msg_order && hints->rx_attr->msg_order != FI_ORDER_NONE){
        return -FI_ENODATA;
      }
    }

    if (hints->domain_attr) {
      // we support all progress modes, messages will make progress in the background
      // regardless of application threads - so it doesn't matter whether the app chooses
      // to have progress threads or not
      if (hints->domain_attr->control_progress != FI_PROGRESS_UNSPEC){
        info->domain_attr->control_progress = hints->domain_attr->control_progress;
      }
      if (hints->domain_attr->data_progress != FI_PROGRESS_UNSPEC){
        info->domain_attr->data_progress = hints->domain_attr->data_progress;
      }

      // yeah, whatever, I don't care. This is just a simulator so there is no registration, really.
      // so however the app wants to think it is registering memory is fine with me
      switch(hints->domain_attr->mr_mode){
        case FI_MR_UNSPEC:
        case FI_MR_BASIC:
          info->domain_attr->mr_mode = FI_MR_BASIC;
          break;
        case FI_MR_SCALABLE:
          //there is so scant little documentation on this I couldn't possibly figure
          //out how to implement this in the simulator
          return -FI_ENODATA;
      }

      // AFAICT, the hints don't really matter for threading here
      // the only thing we really need to be aware of is whether
      // the app is going to serialize access to completion data structures
      // otherwise just follow whatever threading model we want
      // no providers seem to care about this
      if (hints->domain_attr->threading == FI_THREAD_COMPLETION){
        info->domain_attr->threading = FI_THREAD_COMPLETION;
      }

      if (hints->domain_attr->caps) {
        if ((hints->domain_attr->caps & SSTMAC_DOM_CAPS) != SSTMAC_DOM_CAPS) {
          return -FI_ENODATA;
        }
        info->domain_attr->caps = hints->domain_attr->caps;
      }
    }
  }


  // standard call in all providers to do some version-specific
  // twiddling of flags
  // whatever, I don't the simulator cares about any of this
  ofi_alter_info(info, hints, version);

  info->fabric_attr->name = strdup(sstmac_fab_name);

  // we sanity checked these above
  // just go ahead and make them all the same
  info->tx_attr->caps = info->caps;
  info->tx_attr->mode = info->mode;
  info->rx_attr->caps = info->caps;
  info->rx_attr->mode = info->mode;

  *info_ptr = info;

  err.success();
  return FI_SUCCESS;
}

static int sstmac_getinfo(uint32_t version,
      const char *node, const char *service,
      uint64_t flags, const struct fi_info *hints,
      struct fi_info **info)
{
  int ret = 0;
  struct fi_info *info_ptr;

  *info = nullptr;

  ret = sstmac_ep_getinfo(FI_EP_MSG, version, node, service, flags, hints, &info_ptr);
  if (ret == FI_SUCCESS) {
    info_ptr->next = *info;
    *info = info_ptr;
  }

  ret = sstmac_ep_getinfo(FI_EP_DGRAM, version, node, service, flags, hints, &info_ptr);
  if (ret == FI_SUCCESS) {
    info_ptr->next = *info;
    *info = info_ptr;
  }

  ret = sstmac_ep_getinfo(FI_EP_RDM, version, node, service, flags, hints, &info_ptr);
  if (ret == FI_SUCCESS) {
    info_ptr->next = *info;
    *info = info_ptr;
  }

  return *info ? FI_SUCCESS : -FI_ENODATA;
}

struct fi_provider sstmac_prov = {
	.version = FI_VERSION(SSTMAC_MAJOR_VERSION, SSTMAC_MINOR_VERSION),
	.fi_version = OFI_VERSION_LATEST,
  .name = "sstmac",
	.getinfo = sstmac_getinfo,
	.fabric = sstmac_fabric_open,
	.cleanup = sstmac_fini
};

__attribute__((visibility ("default"),EXTERNALLY_VISIBLE)) \
struct fi_provider* fi_prov_ini(void)
{
	struct fi_provider *provider = NULL;
	sstmac_return_t status;
  //sstmac_version_info_t lib_version;
	int num_devices;
	int ret;
	return (provider);
}
