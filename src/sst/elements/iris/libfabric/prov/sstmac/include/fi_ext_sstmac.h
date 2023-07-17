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
 * Copyright (c) 2015-2017 Cray Inc.  All rights reserved.
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

#ifndef _FI_EXT_SSTMAC_H_
#define _FI_EXT_SSTMAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdbool.h>

#define FI_SSTMAC_DOMAIN_OPS_1 "domain ops 1"
typedef enum dom_ops_val { SSTMAC_MSG_RENDEZVOUS_THRESHOLD,
			   SSTMAC_RMA_RDMA_THRESHOLD,
			   SSTMAC_CONN_TABLE_INITIAL_SIZE,
			   SSTMAC_CONN_TABLE_MAX_SIZE,
			   SSTMAC_CONN_TABLE_STEP_SIZE,
			   SSTMAC_VC_ID_TABLE_CAPACITY,
			   SSTMAC_MBOX_PAGE_SIZE,
			   SSTMAC_MBOX_NUM_PER_SLAB,
			   SSTMAC_MBOX_MAX_CREDIT,
			   SSTMAC_MBOX_MSG_MAX_SIZE,
			   SSTMAC_RX_CQ_SIZE,
			   SSTMAC_TX_CQ_SIZE,
			   SSTMAC_MAX_RETRANSMITS,
			   SSTMAC_ERR_INJECT_COUNT,
			   SSTMAC_MR_CACHE_LAZY_DEREG,
			   SSTMAC_MR_CACHE,
			   SSTMAC_MR_UDREG_REG_LIMIT,
			   SSTMAC_MR_SOFT_REG_LIMIT,
			   SSTMAC_MR_HARD_REG_LIMIT,
			   SSTMAC_MR_HARD_STALE_REG_LIMIT,
			   SSTMAC_XPMEM_ENABLE,
			   SSTMAC_DGRAM_PROGRESS_TIMEOUT,
			   SSTMAC_EAGER_AUTO_PROGRESS,
			   SSTMAC_NUM_DOM_OPS
} dom_ops_val_t;

#define FI_SSTMAC_EP_OPS_1 "ep ops 1"
typedef enum ep_ops_val {
	SSTMAC_HASH_TAG_IMPL = 0,
	SSTMAC_NUM_EP_OPS,
} ep_ops_val_t;

#define FI_SSTMAC_FAB_OPS_1 "fab ops 1"
typedef enum fab_ops_val {
	SSTMAC_WAIT_THREAD_SLEEP = 0,
	SSTMAC_DEFAULT_USER_REGISTRATION_LIMIT,
	SSTMAC_DEFAULT_PROV_REGISTRATION_LIMIT,
	SSTMAC_WAIT_SHARED_MEMORY_TIMEOUT,
	SSTMAC_NUM_FAB_OPS,
} fab_ops_val_t;

/* per domain sstmac provider specific ops */
struct fi_sstmac_ops_domain {
	int (*set_val)(struct fid *fid, dom_ops_val_t t, void *val);
	int (*get_val)(struct fid *fid, dom_ops_val_t t, void *val);
	int (*flush_cache)(struct fid *fid);
};

#include <rdma/fi_atomic.h>
enum sstmac_native_amo_types {
	SSTMAC_NAMO_AX = 0x20,
	SSTMAC_NAMO_AX_S,
	SSTMAC_NAMO_FAX,
	SSTMAC_NAMO_FAX_S,
};

struct fi_sstmac_ops_ep {
	int (*set_val)(struct fid *fid, ep_ops_val_t t, void *val);
	int (*get_val)(struct fid *fid, ep_ops_val_t t, void *val);
	size_t (*native_amo)(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, void *result, void *result_desc,
			     /*void *desc,*/ fi_addr_t dest_addr, uint64_t addr,
			     uint64_t key, enum fi_datatype datatype,
			     int req_type,
			     void *context);
};

/* per domain parameters */
struct sstmac_ops_domain {
	uint32_t msg_rendezvous_thresh;
	uint32_t rma_rdma_thresh;
	uint32_t ct_init_size;
	uint32_t ct_max_size;
	uint32_t ct_step;
	uint32_t vc_id_table_capacity;
	uint32_t mbox_page_size;
	uint32_t mbox_num_per_slab;
	uint32_t mbox_maxcredit;
	uint32_t mbox_msg_maxsize;
	uint32_t rx_cq_size;
	uint32_t tx_cq_size;
	uint32_t max_retransmits;
	int32_t err_inject_count;
	bool xpmem_enabled;
	uint32_t dgram_progress_timeout;
	uint32_t eager_auto_progress;
};

struct fi_sstmac_ops_fab {
	int (*set_val)(struct fid *fid, fab_ops_val_t t, void *val);
	int (*get_val)(struct fid *fid, fab_ops_val_t t, void *val);
};

typedef enum sstmac_auth_key_opt {
	SSTMAC_USER_KEY_LIMIT = 0,
	SSTMAC_PROV_KEY_LIMIT,
	SSTMAC_TOTAL_KEYS_NEEDED,
	SSTMAC_USER_KEY_MAX_PER_RANK,
	SSTMAC_MAX_AUTH_KEY_OPTS,
} sstmac_auth_key_opt_t;

struct sstmac_auth_key_attr {
	int user_key_limit;
	int prov_key_limit;
};

enum {
	SSTMAC_AKT_RAW = 0,
	SSTMAC_MAX_AKT_TYPES,
};

struct fi_sstmac_raw_auth_key {
	uint32_t protection_key;
};

struct fi_sstmac_auth_key {
	uint32_t type;
	union {
		struct fi_sstmac_raw_auth_key raw;
	};
};

extern uint8_t* sstmac_default_auth_key;
#define SSTMAC_PROV_DEFAULT_AUTH_KEY sstmac_default_auth_key
#define SSTMAC_PROV_DEFAULT_AUTH_KEYLEN sizeof(struct fi_sstmac_auth_key)

#define FI_SSTMAC_FAB_OPS_2 "fab ops 2"
struct fi_sstmac_auth_key_ops_fab {
	int (*set_val)(
			uint8_t *auth_key,
			size_t auth_key_size,
			sstmac_auth_key_opt_t opt,
			void *val);
	int (*get_val)(
			uint8_t *auth_key,
			size_t auth_key_size,
			sstmac_auth_key_opt_t opt,
			void *val);
};

#ifdef __cplusplus
}
#endif

#endif /* _FI_EXT_SSTMAC_H_ */
