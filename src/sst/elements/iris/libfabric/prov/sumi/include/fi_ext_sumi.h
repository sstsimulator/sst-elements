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

#ifndef _FI_EXT_SUMI_H_
#define _FI_EXT_SUMI_H_

#ifdef __cplusplus
extern "C" {
#endif

//#if HAVE_CONFIG_H
//#include <config.h>
//#endif /* HAVE_CONFIG_H */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define FI_SUMI_DOMAIN_OPS_1 "domain ops 1"
typedef enum dom_ops_val { SUMI_MSG_RENDEZVOUS_THRESHOLD,
			   SUMI_RMA_RDMA_THRESHOLD,
			   SUMI_CONN_TABLE_INITIAL_SIZE,
			   SUMI_CONN_TABLE_MAX_SIZE,
			   SUMI_CONN_TABLE_STEP_SIZE,
			   SUMI_VC_ID_TABLE_CAPACITY,
			   SUMI_MBOX_PAGE_SIZE,
			   SUMI_MBOX_NUM_PER_SLAB,
			   SUMI_MBOX_MAX_CREDIT,
			   SUMI_MBOX_MSG_MAX_SIZE,
			   SUMI_RX_CQ_SIZE,
			   SUMI_TX_CQ_SIZE,
			   SUMI_MAX_RETRANSMITS,
			   SUMI_ERR_INJECT_COUNT,
			   SUMI_MR_CACHE_LAZY_DEREG,
			   SUMI_MR_CACHE,
			   SUMI_MR_UDREG_REG_LIMIT,
			   SUMI_MR_SOFT_REG_LIMIT,
			   SUMI_MR_HARD_REG_LIMIT,
			   SUMI_MR_HARD_STALE_REG_LIMIT,
			   SUMI_XPMEM_ENABLE,
			   SUMI_DGRAM_PROGRESS_TIMEOUT,
			   SUMI_EAGER_AUTO_PROGRESS,
			   SUMI_NUM_DOM_OPS
} dom_ops_val_t;

#define FI_SUMI_EP_OPS_1 "ep ops 1"
typedef enum ep_ops_val {
	SUMI_HASH_TAG_IMPL = 0,
	SUMI_NUM_EP_OPS,
} ep_ops_val_t;

#define FI_SUMI_FAB_OPS_1 "fab ops 1"
typedef enum fab_ops_val {
	SUMI_WAIT_THREAD_SLEEP = 0,
	SUMI_DEFAULT_USER_REGISTRATION_LIMIT,
	SUMI_DEFAULT_PROV_REGISTRATION_LIMIT,
	SUMI_WAIT_SHARED_MEMORY_TIMEOUT,
	SUMI_NUM_FAB_OPS,
} fab_ops_val_t;

/* per domain sstmac provider specific ops */
struct fi_sumi_ops_domain {
	int (*set_val)(struct fid *fid, dom_ops_val_t t, void *val);
	int (*get_val)(struct fid *fid, dom_ops_val_t t, void *val);
	int (*flush_cache)(struct fid *fid);
};

#include <rdma/fi_atomic.h>
enum sumi_native_amo_types {
	SUMI_NAMO_AX = 0x20,
	SUMI_NAMO_AX_S,
	SUMI_NAMO_FAX,
	SUMI_NAMO_FAX_S,
};

struct fi_sumi_ops_ep {
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
struct sumi_ops_domain {
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

struct fi_sumi_ops_fab {
	int (*set_val)(struct fid *fid, fab_ops_val_t t, void *val);
	int (*get_val)(struct fid *fid, fab_ops_val_t t, void *val);
};

typedef enum sumi_auth_key_opt {
	SUMI_USER_KEY_LIMIT = 0,
	SUMI_PROV_KEY_LIMIT,
	SUMI_TOTAL_KEYS_NEEDED,
	SUMI_USER_KEY_MAX_PER_RANK,
	SUMI_MAX_AUTH_KEY_OPTS,
} sumi_auth_key_opt_t;

struct sumi_auth_key_attr {
	int user_key_limit;
	int prov_key_limit;
};

enum {
	SUMI_AKT_RAW = 0,
	SUMI_MAX_AKT_TYPES,
};

struct fi_sumi_raw_auth_key {
	uint32_t protection_key;
};

struct fi_sumi_auth_key {
	uint32_t type;
	union {
		struct fi_sumi_raw_auth_key raw;
	};
};

extern uint8_t* sumi_default_auth_key;
#define SUMI_PROV_DEFAULT_AUTH_KEY sumi_default_auth_key
#define SUMI_PROV_DEFAULT_AUTH_KEYLEN sizeof(struct fi_sumi_auth_key)

#define FI_SUMI_FAB_OPS_2 "fab ops 2"
struct fi_sumi_auth_key_ops_fab {
	int (*set_val)(
			uint8_t *auth_key,
			size_t auth_key_size,
			sumi_auth_key_opt_t opt,
			void *val);
	int (*get_val)(
			uint8_t *auth_key,
			size_t auth_key_size,
			sumi_auth_key_opt_t opt,
			void *val);
};

#ifdef __cplusplus
}
#endif

#endif /* _FI_EXT_SUMI_H_ */
