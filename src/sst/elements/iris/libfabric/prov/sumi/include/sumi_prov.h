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

#ifndef _SUMI_H_
#define _SUMI_H_

//#if HAVE_CONFIG_H
//#include <config.h>
//#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <rdma/fabric.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/providers/fi_prov.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_trigger.h>

#include <ofi.h>
#include <ofi_atomic.h>
#include <ofi_enosys.h>
#include <ofi_rbuf.h>
#include <ofi_list.h>
#include <ofi_file.h>

#include "fi_ext_sumi.h"


#ifdef __cplusplus
extern "C" {
#endif

struct sumi_mem_handle_t {
  int ignore;
};

/*
 * useful macros
 */
#ifndef FLOOR
#define FLOOR(a, b) ((long long)(a) - (((long long)(a)) % (b)))
#endif

#ifndef CEILING
#define CEILING(a, b) ((long long)(a) <= 0LL ? 0 : (FLOOR((a)-1, b) + (b)))
#endif

#ifndef compiler_barrier
#define compiler_barrier() asm volatile ("" ::: "memory")
#endif

#define SUMI_MAX_MSG_IOV_LIMIT 1
#define SUMI_MAX_RMA_IOV_LIMIT 1
#define SUMI_MAX_ATOMIC_IOV_LIMIT 1
#define SUMI_ADDR_CACHE_SIZE 5

/*
 * GNI GET alignment
 */

#define SUMI_READ_ALIGN		4
#define SUMI_READ_ALIGN_MASK	(SUMI_READ_ALIGN - 1)

/*
 * GNI IOV GET alignment
 *
 * We always pull 4byte chucks for unaligned GETs. To prevent stomping on
 * someone else's head or tail data, each segment must be four bytes
 * (i.e. SUMI_READ_ALIGN bytes).
 *
 * Note: "* 2" for head and tail
 */
#define SUMI_INT_TX_BUF_SZ (SUMI_MAX_MSG_IOV_LIMIT * SUMI_READ_ALIGN * 2)

/*
 * Flags
 * The 64-bit flag field is used as follows:
 * 1-grow up    common (usable with multiple operations)
 * 59-grow down operation specific (used for single call/class)
 * 60 - 63      provider specific
 */

#define SUMI_SUPPRESS_COMPLETION	(1ULL << 60)	/* TX only flag */

#define SUMI_RMA_RDMA			(1ULL << 61)	/* RMA only flag */
#define SUMI_RMA_INDIRECT		(1ULL << 62)	/* RMA only flag */
#define SUMI_RMA_CHAINED		(1ULL << 63)	/* RMA only flag */

#define SUMI_MSG_RENDEZVOUS		(1ULL << 61)	/* MSG only flag */
#define SUMI_MSG_GET_TAIL		(1ULL << 62)	/* MSG only flag */


#define SUMI_SUPPORTED_FLAGS (FI_NUMERICHOST | FI_SOURCE)

#define SUMI_DEFAULT_FLAGS (0)

#define SUMI_DOM_CAPS \
  (FI_LOCAL_COMM | FI_REMOTE_COMM | FI_SHARED_AV)

#define SUMI_EP_PRIMARY_CAPS                                               \
	(FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS |                            \
	 FI_DIRECTED_RECV | FI_READ | FI_NAMED_RX_CTX |                        \
	 FI_WRITE | FI_SEND | FI_RECV | FI_REMOTE_READ | FI_REMOTE_WRITE)

#define SUMI_EP_SEC_CAPS (FI_MULTI_RECV | FI_TRIGGER | FI_FENCE)

#define SUMI_EP_OP_FLAGS	(FI_INJECT | FI_MULTI_RECV | FI_COMPLETION | \
				 FI_INJECT_COMPLETE | FI_TRANSMIT_COMPLETE | \
				 FI_DELIVERY_COMPLETE)

#define SUMI_SENDMSG_FLAGS	(FI_REMOTE_CQ_DATA | FI_COMPLETION | \
				 FI_MORE | FI_INJECT | FI_INJECT_COMPLETE | \
				 FI_TRANSMIT_COMPLETE | FI_FENCE | FI_TRIGGER)
#define SUMI_RECVMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_MULTI_RECV)
#define SUMI_TRECVMSG_FLAGS \
	(SUMI_RECVMSG_FLAGS | FI_CLAIM | FI_PEEK | FI_DISCARD)

#define SUMI_WRITEMSG_FLAGS	(FI_REMOTE_CQ_DATA | FI_COMPLETION | \
				 FI_MORE | FI_INJECT | FI_INJECT_COMPLETE | \
				 FI_TRANSMIT_COMPLETE | FI_FENCE | FI_TRIGGER)
#define SUMI_READMSG_FLAGS	(FI_COMPLETION | FI_MORE | \
				 FI_FENCE | FI_TRIGGER)
#define SUMI_ATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_INJECT | \
				 FI_FENCE | FI_TRIGGER)
#define SUMI_FATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_FENCE | \
				 FI_TRIGGER)
#define SUMI_CATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_FENCE | \
				 FI_TRIGGER)

#define SUMI_RMA_COMPLETION_FLAGS	(FI_RMA | FI_READ | FI_WRITE)
#define SUMI_AMO_COMPLETION_FLAGS	(FI_ATOMIC | FI_READ | FI_WRITE)

#define SUMI_TX_SIZE_DEFAULT 100
#define SUMI_RX_SIZE_DEFAULT 1000

#define SUMI_RX_CTX_MAX_BITS	8
#define SUMI_SEP_MAX_CNT	(1 << (SUMI_RX_CTX_MAX_BITS - 1))

#define SUMI_MAX_MSG_SIZE (1<<31)
#define SUMI_CACHELINE_SIZE (64)
#define SUMI_INJECT_SIZE 64
#define SUMI_MAX_INJECT_SIZE 64

#define SUMI_FAB_MODES	0

#define SUMI_FAB_MODES_CLEAR (FI_MSG_PREFIX | FI_ASYNC_IOV)

struct sumi_address {
	uint32_t device_addr;
	uint32_t cdm_id;
};

#define SUMI_ADDR_UNSPEC(var) (((var).device_addr == -1) && \
				((var).cdm_id == -1))

#define SUMI_ADDR_EQUAL(a, b) (((a).device_addr == (b).device_addr) && \
				((a).cdm_id == (b).cdm_id))

#define SUMI_CREATE_CDM_ID	0

#define SUMI_EPN_TYPE_UNBOUND	(1 << 0)
#define SUMI_EPN_TYPE_BOUND	(1 << 1)
#define SUMI_EPN_TYPE_SEP	(1 << 2)

struct sumi_ep_name {
  struct sumi_address sumi_addr;
  struct {
    uint32_t name_type : 8;
    uint32_t cm_nic_cdm_id : 24;
    uint32_t cookie;
  };
  struct {
    uint32_t rx_ctx_cnt : 8;
    uint32_t key_offset : 12;
    uint32_t unused1 : 12;
    uint32_t unused2;
  };
  uint64_t reserved[3];
};

/* AV address string revision. */
#define SUMI_AV_STR_ADDR_VERSION  1

/*
 * 52 is the number of characters printed out in sumi_av_straddr.
 *  1 is for the null terminator
 */
#define SUMI_AV_MAX_STR_ADDR_LEN  (52 + 1)

/*
 * 15 is the number of characters for the device addr.
 *  1 is for the null terminator
 */
#define SUMI_AV_MIN_STR_ADDR_LEN  (15 + 1)

/*
 * 69 is the number of characters for the printable portion of the address
 *  1 is for the null terminator
 */
#define SUMI_FI_ADDR_STR_LEN (69 + 1)

/*
 * enum for blocking/non-blocking progress
 */
enum sumi_progress_type {
	SUMI_PRG_BLOCKING,
	SUMI_PRG_NON_BLOCKING
};


struct sumi_fid_tport;

/*
 * simple struct for sstmac fabric, may add more stuff here later
 */
struct sumi_fid_fabric {
  struct fid_fabric fab_fid;
  //this will actually be an SST/macro transport object
  struct sumi_fid_tport* tport;
};


extern struct fi_ops_cm sumi_ep_msg_ops_cm;
extern struct fi_ops_cm sumi_ep_ops_cm;

/*
 * Our domains are very simple because we have nothing complicated
 * with memory registration and we don't have to worry about
 * progress modes
 */
struct sumi_fid_domain {
	struct fid_domain domain_fid;
  struct sumi_fid_fabric *fabric;
  uint32_t addr_format;
};

struct sumi_fid_pep {
	struct fid_pep pep_fid;
	struct sumi_fid_fabric *fabric;
	struct fi_info *info;
	struct sumi_fid_eq *eq;
	struct sumi_ep_name src_addr;
	fastlock_t lock;
	int listen_fd;
	int backlog;
	int bound;
	size_t cm_data_size;
  //struct sumi_reference ref_cnt;
};

#define SUMI_CQS_PER_EP		8

struct sumi_fid_ep_ops_en {
	uint32_t msg_recv_allowed: 1;
	uint32_t msg_send_allowed: 1;
	uint32_t rma_read_allowed: 1;
	uint32_t rma_write_allowed: 1;
	uint32_t tagged_recv_allowed: 1;
	uint32_t tagged_send_allowed: 1;
	uint32_t atomic_read_allowed: 1;
	uint32_t atomic_write_allowed: 1;
};

#define SUMI_INT_TX_POOL_SIZE 128
#define SUMI_INT_TX_POOL_COUNT 256

struct sumi_int_tx_buf {
	struct slist_entry e;
	uint8_t *buf;
	struct sumi_fid_mem_desc *md;
};

typedef int sumi_return_t;

struct sumi_int_tx_ptrs {
	struct slist_entry e;
	void *sl_ptr;
	void *buf_ptr;
	struct sumi_fid_mem_desc *md;
};

struct sumi_int_tx_pool {
	bool enabled;
	int nbufs;
	fastlock_t lock;
	struct slist sl;
	struct slist bl;
};

struct sumi_addr_cache_entry {
	fi_addr_t addr;
	struct sumi_vc *vc;
};

enum sumi_conn_state {
	SUMI_EP_UNCONNECTED,
	SUMI_EP_CONNECTING,
	SUMI_EP_CONNECTED,
	SUMI_EP_SHUTDOWN
};

struct sumi_fid_ep {
	struct fid_ep ep_fid;
	enum fi_ep_type type;
	struct sumi_fid_domain *domain;
	uint64_t op_flags;
	uint64_t caps;
  uint16_t rx_id;
	struct sumi_fid_cq *send_cq;
	struct sumi_fid_cq *recv_cq;
	struct sumi_fid_cntr *send_cntr;
	struct sumi_fid_cntr *recv_cntr;
	struct sumi_fid_cntr *write_cntr;
	struct sumi_fid_cntr *read_cntr;
	struct sumi_fid_cntr *rwrite_cntr;
	struct sumi_fid_cntr *rread_cntr;
	struct sumi_fid_av *av;
	struct sumi_fid_stx *stx_ctx;
	struct sumi_fid_eq *eq;
  int qos;
};

struct sumi_fid_sep {
	struct fid_ep ep_fid;
	enum fi_ep_type type;
	struct fid_domain *domain;
	struct fi_info *info;
	uint64_t caps;
	uint32_t cdm_id_base;
	struct fid_ep **ep_table;
	struct fid_ep **tx_ep_table;
	struct fid_ep **rx_ep_table;
	bool *enabled;
	struct sumi_cm_nic *cm_nic;
	struct sumi_fid_av *av;
	struct sumi_ep_name my_name;
	fastlock_t sep_lock;
  //struct sumi_reference ref_cnt;
};

struct sumi_fid_trx {
  sumi_fid_ep ep;
};

struct sumi_fid_stx {
	struct fid_stx stx_fid;
	struct sumi_fid_domain *domain;
	struct sumi_nic *nic;
	struct sumi_auth_key *auth_key;
  //struct sumi_reference ref_cnt;
};

struct sumi_fid_av {
  fid_av av_fid;
  sumi_fid_domain* domain;
};

enum sumi_fab_req_type {
	SUMI_FAB_RQ_SEND,
	SUMI_FAB_RQ_SENDV,
	SUMI_FAB_RQ_TSEND,
	SUMI_FAB_RQ_TSENDV,
	SUMI_FAB_RQ_RDMA_WRITE,
	SUMI_FAB_RQ_RDMA_READ,
	SUMI_FAB_RQ_RECV,
	SUMI_FAB_RQ_RECVV,
	SUMI_FAB_RQ_TRECV,
	SUMI_FAB_RQ_TRECVV,
	SUMI_FAB_RQ_MRECV,
	SUMI_FAB_RQ_AMO,
	SUMI_FAB_RQ_FAMO,
	SUMI_FAB_RQ_CAMO,
	SUMI_FAB_RQ_END_NON_NATIVE,
  SUMI_FAB_RQ_START_NATIV,
  SUMI_FAB_RQ_NAMO_AX,
  SUMI_FAB_RQ_NAMO_AX_S,
  SUMI_FAB_RQ_NAMO_FAX,
  SUMI_FAB_RQ_NAMO_FAX_S,
	SUMI_FAB_RQ_MAX_TYPES,
};

struct sumi_fab_req_rma {
	uint64_t                 loc_addr;
	struct sumi_fid_mem_desc *loc_md;
	size_t                   len;
	uint64_t                 rem_addr;
	uint64_t                 rem_mr_key;
	uint64_t                 imm;
	ofi_atomic32_t           outstanding_txds;
	sumi_return_t             status;
	struct slist_entry       sle;
};

struct sumi_fid_eq {
  struct fid_eq eq_fid;
  struct sumi_fid_fabric* fabric;
  struct fid_wait* wait;
  struct fi_eq_attr attr;
};

struct sumi_progress_queue;
struct sumi_fid_cq {
  struct fid_cq cq_fid;
  struct sumi_fid_domain *domain;
  int id; //the sumi CQ id allocated to this
  enum fi_cq_format format;
  size_t entry_size;
  struct fid_wait *wait;
  sumi_progress_queue* queue;
};

struct sumi_fid_srx {
  struct fid_ep ep_fid;
  sumi_fid_domain* domain;
};

#define ADDR_CQ(addr)    ((addr >> 16) | 0xFFFF)
#define ADDR_RANK(addr)  ((addr >> 32) | 0xFFFFFFFF)
#define ADDR_QUEUE(addr) (addr | 0xFFFF)
#define ADDR_RANK_BITS(rank)    (rank << 32)
#define ADDR_QUEUE_BITS(queue)  (queue)
#define ADDR_CQ_BITS(cq)        (cq << 16)

extern const char sumi_fab_name[];
extern const char sumi_dom_name[];


/*
 * prototypes for fi ops methods
 */
int sumi_domain_open(struct fid_fabric *fabric, struct fi_info *info,
		     struct fid_domain **domain, void *context);

int sumi_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
		 struct fid_av **av, void *context);

int sumi_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
		 struct fid_cq **cq, void *context);

int sumi_ep_open(struct fid_domain *domain, struct fi_info *info,
		   struct fid_ep **ep, void *context);

int sumi_pep_open(struct fid_fabric *fabric,
		  struct fi_info *info, struct fid_pep **pep,
		  void *context);

int sumi_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
		 struct fid_eq **eq, void *context);

int sumi_mr_reg(struct fid *fid, const void *buf, size_t len,
		uint64_t access, uint64_t offset, uint64_t requested_key,
		uint64_t flags, struct fid_mr **mr_o, void *context);

int sumi_mr_regv(struct fid *fid, const struct iovec *iov,
                 size_t count, uint64_t access,
                 uint64_t offset, uint64_t requested_key,
                 uint64_t flags, struct fid_mr **mr, void *context);

int sumi_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
                    uint64_t flags, struct fid_mr **mr);

int sumi_cntr_open(struct fid_domain *domain, struct fi_cntr_attr *attr,
		 struct fid_cntr **cntr, void *context);

int sumi_sep_open(struct fid_domain *domain, struct fi_info *info,
		 struct fid_ep **sep, void *context);

int sumi_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

int sumi_ep_close(fid_t fid);

void _sumi_init(void);

#ifdef FABRIC_DIRECT_ENABLED
#define DIRECT_FN __attribute__((visibility ("default")))
#define STATIC
#define EXTERN_C extern "C"
#else
#define DIRECT_FN
#define STATIC static
#define EXTERN_C
#endif

#ifdef __cplusplus
}

#include <mercury/operating_system/process/progress_queue.h>
#include <sumi/message.h>
#include <sumi_fabric.hpp>

struct ErrorDeallocate {
  template <class T, class Lambda>
  ErrorDeallocate(T* t, Lambda&& l) :
    ptr(t), dealloc(std::forward<Lambda>(l))
  {
  }

  void success(){
    ptr = nullptr;
  }

  ~ErrorDeallocate(){
    if (ptr) dealloc(ptr);
  }

  void* ptr;
  std::function<void(void*)> dealloc;
};

struct RecvQueue {

  struct Recv {
    uint32_t size;
    void* buf;
    Recv(uint32_t s, void* b) :
      size(s), buf(b)
    {
    }
  };

  struct TaggedRecv {
    uint32_t size;
    void* buf;
    uint64_t tag;
    uint64_t tag_ignore;
    TaggedRecv(uint32_t s, void* b, uint64_t t, uint64_t ti) :
      size(s), buf(b), tag(t), tag_ignore(ti)
    {
    }
  };

  RecvQueue(SST::Hg::OperatingSystemAPI* os) :
    progress(os)
  {
  }

  bool matches(FabricMessage* msg, uint64_t tag, uint64_t ignore){
    return (msg->tag() & ~ignore) == (tag & ~ignore);
  }

  std::list<Recv> recvs;
  std::list<TaggedRecv> tagged_recvs;
  std::list<FabricMessage*> unexp_recvs;
  std::list<FabricMessage*> unexp_tagged_recvs;

  SST::Hg::SingleProgressQueue<SST::Iris::sumi::Message> progress;

  void finishMatch(void* buf, uint32_t size, FabricMessage* fmsg);

  void matchTaggedRecv(FabricMessage* msg);

  void postRecv(uint32_t size, void* buf, uint64_t tag, uint64_t tag_ignore, bool tagged);

  void incoming(SST::Iris::sumi::Message* msg);

};


#endif

#endif /* _SUMI_H_ */
