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
 * Copyright (c) 2015-2019 Cray Inc. All rights reserved.
 * Copyright (c) 2015-2018 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2019 Triad National Security, LLC. All rights reserved.
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

/*
 * Endpoint common code
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sumi_prov.h"
#include "sumi_ep.h"

#include <sumi_fabric.hpp>
#include <mercury/components/operating_system.h>

EXTERN_C DIRECT_FN STATIC  int sumi_ep_control(fid_t fid, int command, void *arg);
static int sumi_ep_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
                              void **ops, void *context);

static struct fi_ops sumi_ep_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sumi_ep_close,
  .bind = sumi_ep_bind,
  .control = sumi_ep_control,
  .ops_open = sumi_ep_ops_open,
};

DIRECT_FN STATIC ssize_t sumi_ep_rx_size_left(struct fid_ep *ep);
DIRECT_FN STATIC ssize_t sumi_ep_tx_size_left(struct fid_ep *ep);
static struct fi_ops_ep sumi_ep_ops = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = sumi_cancel,
  .getopt = sumi_getopt,
  .setopt = sumi_setopt,
  .tx_ctx = fi_no_tx_ctx,
  .rx_ctx = fi_no_rx_ctx,
  .rx_size_left = sumi_ep_rx_size_left,
  .tx_size_left = sumi_ep_tx_size_left,
};

DIRECT_FN STATIC ssize_t sumi_ep_recv(struct fid_ep *ep, void *buf, size_t len,
              void *desc, fi_addr_t src_addr,
              void *context);
DIRECT_FN STATIC ssize_t sumi_ep_recvv(struct fid_ep *ep,
               const struct iovec *iov,
               void **desc, size_t count,
               fi_addr_t src_addr,
               void *context);
DIRECT_FN STATIC ssize_t sumi_ep_recvmsg(struct fid_ep *ep,
           const struct fi_msg *msg,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_ep_send(struct fid_ep *ep, const void *buf,
              size_t len, void *desc,
              fi_addr_t dest_addr, void *context);
DIRECT_FN STATIC ssize_t sumi_ep_sendv(struct fid_ep *ep,
               const struct iovec *iov,
               void **desc, size_t count,
               fi_addr_t dest_addr,
               void *context);
DIRECT_FN STATIC ssize_t sumi_ep_sendmsg(struct fid_ep *ep,
           const struct fi_msg *msg,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_ep_msg_inject(struct fid_ep *ep, const void *buf,
              size_t len, fi_addr_t dest_addr);
DIRECT_FN STATIC ssize_t sumi_ep_senddata(struct fid_ep *ep, const void *buf,
            size_t len, void *desc, uint64_t data,
            fi_addr_t dest_addr, void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
           uint64_t data, fi_addr_t dest_addr);
static struct fi_ops_msg sumi_ep_msg_ops = {
  .size = sizeof(struct fi_ops_msg),
  .recv = sumi_ep_recv,
  .recvv = sumi_ep_recvv,
  .recvmsg = sumi_ep_recvmsg,
  .send = sumi_ep_send,
  .sendv = sumi_ep_sendv,
  .sendmsg = sumi_ep_sendmsg,
  .inject = sumi_ep_msg_inject,
  .senddata = sumi_ep_senddata,
  .injectdata = sumi_ep_msg_injectdata,
};

DIRECT_FN STATIC ssize_t sumi_ep_read(struct fid_ep *ep, void *buf, size_t len,
              void *desc, fi_addr_t src_addr, uint64_t addr,
              uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
        size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
        void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags);
DIRECT_FN STATIC ssize_t
sumi_ep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
        fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
         size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
         void *context);
DIRECT_FN STATIC ssize_t sumi_ep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
        uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_ep_rma_inject(struct fid_ep *ep, const void *buf,
              size_t len, fi_addr_t dest_addr,
              uint64_t addr, uint64_t key);
DIRECT_FN STATIC ssize_t
sumi_ep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
      uint64_t data, fi_addr_t dest_addr, uint64_t addr,
      uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
           uint64_t data, fi_addr_t dest_addr, uint64_t addr,
           uint64_t key);
static struct fi_ops_rma sumi_ep_rma_ops = {
  .size = sizeof(struct fi_ops_rma),
  .read = sumi_ep_read,
  .readv = sumi_ep_readv,
  .readmsg = sumi_ep_readmsg,
  .write = sumi_ep_write,
  .writev = sumi_ep_writev,
  .writemsg = sumi_ep_writemsg,
  .inject = sumi_ep_rma_inject,
  .writedata = sumi_ep_writedata,
  .injectdata = sumi_ep_rma_injectdata,
};

DIRECT_FN STATIC ssize_t sumi_ep_trecv(struct fid_ep *ep, void *buf, size_t len,
               void *desc, fi_addr_t src_addr,
               uint64_t tag, uint64_t ignore,
               void *context);
DIRECT_FN STATIC ssize_t sumi_ep_trecvv(struct fid_ep *ep,
          const struct iovec *iov,
          void **desc, size_t count,
          fi_addr_t src_addr,
          uint64_t tag, uint64_t ignore,
          void *context);
DIRECT_FN STATIC ssize_t sumi_ep_trecvmsg(struct fid_ep *ep,
            const struct fi_msg_tagged *msg,
            uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_ep_tsend(struct fid_ep *ep, const void *buf,
               size_t len, void *desc,
               fi_addr_t dest_addr, uint64_t tag,
               void *context);
DIRECT_FN STATIC ssize_t sumi_ep_tsendv(struct fid_ep *ep,
          const struct iovec *iov,
          void **desc, size_t count,
          fi_addr_t dest_addr,
          uint64_t tag, void *context);
DIRECT_FN STATIC ssize_t sumi_ep_tsendmsg(struct fid_ep *ep,
            const struct fi_msg_tagged *msg,
            uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_ep_tinject(struct fid_ep *ep, const void *buf,
           size_t len, fi_addr_t dest_addr,
           uint64_t tag);
DIRECT_FN STATIC ssize_t sumi_ep_tinjectdata(struct fid_ep *ep, const void *buf,
               size_t len, uint64_t data,
               fi_addr_t dest_addr, uint64_t tag);
DIRECT_FN STATIC ssize_t sumi_ep_tsenddata(struct fid_ep *ep, const void *buf,
             size_t len, void *desc,
             uint64_t data, fi_addr_t dest_addr,
             uint64_t tag, void *context);
struct fi_ops_tagged sumi_ep_tagged_ops = {
  .size = sizeof(struct fi_ops_tagged),
  .recv = sumi_ep_trecv,
  .recvv = sumi_ep_trecvv,
  .recvmsg = sumi_ep_trecvmsg,
  .send = sumi_ep_tsend,
  .sendv = sumi_ep_tsendv,
  .sendmsg = sumi_ep_tsendmsg,
  .inject = sumi_ep_tinject,
  .senddata = sumi_ep_tsenddata,
  .injectdata = sumi_ep_tinjectdata,
};

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
         void *desc, fi_addr_t dest_addr, uint64_t addr,
         uint64_t key, enum fi_datatype datatype, enum fi_op op,
         void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
          size_t count, fi_addr_t dest_addr, uint64_t addr,
          uint64_t key, enum fi_datatype datatype, enum fi_op op,
          void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
      uint64_t flags);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
          fi_addr_t dest_addr, uint64_t addr, uint64_t key,
          enum fi_datatype datatype, enum fi_op op);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
       void *desc, void *result, void *result_desc,
       fi_addr_t dest_addr, uint64_t addr, uint64_t key,
       enum fi_datatype datatype, enum fi_op op,
       void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
        void **desc, size_t count, struct fi_ioc *resultv,
        void **result_desc, size_t result_count,
        fi_addr_t dest_addr, uint64_t addr, uint64_t key,
        enum fi_datatype datatype, enum fi_op op,
        void *context);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
          struct fi_ioc *resultv, void **result_desc,
          size_t result_count, uint64_t flags);
DIRECT_FN STATIC ssize_t
sumi_ep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
       void *desc, const void *compare, void *compare_desc,
       void *result, void *result_desc, fi_addr_t dest_addr,
       uint64_t addr, uint64_t key, enum fi_datatype datatype,
       enum fi_op op, void *context);
DIRECT_FN STATIC ssize_t sumi_ep_atomic_compwritev(struct fid_ep *ep,
               const struct fi_ioc *iov,
               void **desc,
               size_t count,
               const struct fi_ioc *comparev,
               void **compare_desc,
               size_t compare_count,
               struct fi_ioc *resultv,
               void **result_desc,
               size_t result_count,
               fi_addr_t dest_addr,
               uint64_t addr, uint64_t key,
               enum fi_datatype datatype,
               enum fi_op op,
               void *context);
DIRECT_FN STATIC ssize_t sumi_ep_atomic_compwritemsg(struct fid_ep *ep,
                 const struct fi_msg_atomic *msg,
                 const struct fi_ioc *comparev,
                 void **compare_desc,
                 size_t compare_count,
                 struct fi_ioc *resultv,
                 void **result_desc,
                 size_t result_count,
                 uint64_t flags);

struct fi_ops_atomic sumi_ep_atomic_ops = {
  .size = sizeof(struct fi_ops_atomic),
  .write = sumi_ep_atomic_write,
  .writev = sumi_ep_atomic_writev,
  .writemsg = sumi_ep_atomic_writemsg,
  .inject = sumi_ep_atomic_inject,
  .readwrite = sumi_ep_atomic_readwrite,
  .readwritev = sumi_ep_atomic_readwritev,
  .readwritemsg = sumi_ep_atomic_readwritemsg,
  .compwrite = sumi_ep_atomic_compwrite,
  .compwritev = sumi_ep_atomic_compwritev,
  .compwritemsg = sumi_ep_atomic_compwritemsg,
  .writevalid = sumi_ep_atomic_valid,
  .readwritevalid = sumi_ep_fetch_atomic_valid,
  .compwritevalid = sumi_ep_cmp_atomic_valid,
};

static ssize_t sstmaci_ep_recv(struct fid_ep* ep, void* buf, size_t len, fi_addr_t src_addr, void* context,
                               uint64_t tag, uint64_t tag_ignore, uint64_t flags)
{
  sumi_fid_ep* ep_impl = (sumi_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  if (src_addr != FI_ADDR_UNSPEC){
    return -FI_EINVAL;
  }

  RecvQueue* rq = (RecvQueue*) ep_impl->recv_cq->queue;
  rq->postRecv(len, buf, tag, tag_ignore, bool(flags & FI_TAGGED));

  return 0;
}

DIRECT_FN STATIC ssize_t sumi_ep_recv(struct fid_ep *ep, void *buf, size_t len,
				      void *desc, fi_addr_t src_addr,
				      void *context)
{
  uint64_t ignore = 0;
  return sstmaci_ep_recv(ep, buf, len, src_addr, context, 0, ~ignore, 0);
}

DIRECT_FN STATIC ssize_t sumi_ep_recvv(struct fid_ep *ep,
				       const struct iovec *iov,
				       void **desc, size_t count,
				       fi_addr_t src_addr,
				       void *context)
{
  uint64_t ignore = 0;
  if (count == 1){
    return sstmaci_ep_recv(ep, iov[0].iov_base, iov[0].iov_len, src_addr, context, 0, ~ignore, 0);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sumi_ep_recvmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags)
{
  return -FI_ENOSYS;
}

static ssize_t sstmaci_ep_send(struct fid_ep* ep, const void* buf, size_t len,
                               fi_addr_t dest_addr, void* context,
                               uint64_t tag, uint64_t data, uint64_t flags)
{
  sumi_fid_ep* ep_impl = (sumi_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  uint32_t dest_rank = ADDR_RANK(dest_addr);
  uint16_t remote_cq = ADDR_CQ(dest_addr);
  //uint16_t recv_queue = ADDR_QUEUE(dest_addr);

  flags |= FI_SEND;

  tport->postSend<FabricMessage>(dest_rank, len, const_cast<void*>(buf),
                                 ep_impl->send_cq->id, // rma operations go to the tx
                                 remote_cq, SST::Iris::sumi::Message::pt2pt, ep_impl->qos,
                                 tag, FabricMessage::no_imm_data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t sumi_ep_send(struct fid_ep *ep, const void *buf,
				      size_t len, void *desc,
				      fi_addr_t dest_addr, void *context)
{
  return sstmaci_ep_send(ep, buf, len, dest_addr, context,
                         FabricMessage::no_tag, FabricMessage::no_imm_data, 0);
}

DIRECT_FN STATIC ssize_t sumi_ep_sendv(struct fid_ep *ep,
				       const struct iovec *iov,
				       void **desc, size_t count,
				       fi_addr_t dest_addr,
				       void *context)
{
  if (count == 1){
    return sstmaci_ep_send(ep, iov[0].iov_base, iov[0].iov_len, dest_addr, context,
                           FabricMessage::no_tag, FabricMessage::no_imm_data, 0);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sumi_ep_sendmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_msg_inject(struct fid_ep *ep, const void *buf,
					    size_t len, fi_addr_t dest_addr)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_senddata(struct fid_ep *ep, const void *buf,
					  size_t len, void *desc, uint64_t data,
					  fi_addr_t dest_addr, void *context)
{
  return sstmaci_ep_send(ep, buf, len, dest_addr, context,
                         FabricMessage::no_tag, data, FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t
sumi_ep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		       uint64_t data, fi_addr_t dest_addr)
{
  return -FI_ENOSYS;
}

static ssize_t sstmaci_ep_read(struct fid_ep *ep, void *buf, size_t len,
                               fi_addr_t src_addr, uint64_t addr,
                               void *context)
{
  sumi_fid_ep* ep_impl = (sumi_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  uint32_t src_rank = ADDR_RANK(src_addr);

  uint64_t flags = 0;
  int remote_cq = SST::Iris::sumi::Message::no_ack;
  if (ep_impl->op_flags & FI_REMOTE_READ){
    remote_cq = ADDR_CQ(src_addr);
    flags |= FI_REMOTE_READ;
  }
  flags |= FI_READ;

  tport->rdmaGet<FabricMessage>(src_rank, len, buf, (void*) addr,
                                ep_impl->send_cq->id, // rma operations go to the tx
                                remote_cq,
                                SST::Iris::sumi::Message::pt2pt, ep_impl->qos,
                                FabricMessage::no_tag, FabricMessage::no_imm_data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t sumi_ep_read(struct fid_ep *ep, void *buf, size_t len,
				      void *desc, fi_addr_t src_addr, uint64_t addr,
				      uint64_t key, void *context)
{
  return sstmaci_ep_read(ep, buf, len, src_addr, addr, context);
}

DIRECT_FN STATIC ssize_t
sumi_ep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	      size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
	      void *context)
{
  if (count == 1){
    return sstmaci_ep_read(ep, iov[0].iov_base, iov[0].iov_len, src_addr, addr, context);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t
sumi_ep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags)
{
  return -FI_ENOSYS;
}

static ssize_t sstmaci_ep_write(struct fid_ep *ep, const void *buf, size_t len,
                                fi_addr_t dest_addr, uint64_t addr, void *context,
                                uint64_t data, uint64_t flags)
{
  sumi_fid_ep* ep_impl = (sumi_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  int remote_cq = SST::Iris::sumi::Message::no_ack;
  if (ep_impl->op_flags & FI_REMOTE_WRITE){
    remote_cq = ADDR_CQ(dest_addr);
    flags |= FI_REMOTE_WRITE;
  }
  flags |= FI_WRITE;

  uint32_t src_rank = ADDR_RANK(dest_addr);
  tport->rdmaPut<FabricMessage>(src_rank, len, const_cast<void*>(buf), (void*) addr,
                                ep_impl->send_cq->id, // rma operations go to the tx
                                remote_cq,
                                SST::Iris::sumi::Message::pt2pt, ep_impl->qos,
                                FabricMessage::no_tag, data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t
sumi_ep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	      fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context)
{
  return sstmaci_ep_write(ep, buf, len, dest_addr, addr, context,
                          FabricMessage::no_imm_data, FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t
sumi_ep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
	       size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
	       void *context)
{
  if (count == 1){
    return sstmaci_ep_write(ep, iov[0].iov_base, iov[0].iov_len, dest_addr, addr, context,
                            FabricMessage::no_imm_data, 0);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sumi_ep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
				uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_rma_inject(struct fid_ep *ep, const void *buf,
					    size_t len, fi_addr_t dest_addr,
					    uint64_t addr, uint64_t key)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		  uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		  uint64_t key, void *context)
{
  return sstmaci_ep_write(ep, buf, len, dest_addr, addr, context, data, FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t
sumi_ep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		       uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		       uint64_t key)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_trecv(struct fid_ep *ep, void *buf, size_t len,
				       void *desc, fi_addr_t src_addr,
				       uint64_t tag, uint64_t ignore,
				       void *context)
{
  return sstmaci_ep_recv(ep, buf, len, src_addr, context, tag, ignore, FI_TAGGED);
}

DIRECT_FN STATIC ssize_t sumi_ep_trecvv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t src_addr,
					uint64_t tag, uint64_t ignore,
					void *context)
{
  if (count == 1){
    return sstmaci_ep_recv(ep, iov[0].iov_base, iov[0].iov_len,
        src_addr, context, tag, ignore, FI_TAGGED);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sumi_ep_trecvmsg(struct fid_ep *ep,
					  const struct fi_msg_tagged *msg,
					  uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_tsend(struct fid_ep *ep, const void *buf,
				       size_t len, void *desc,
				       fi_addr_t dest_addr, uint64_t tag,
				       void *context)
{
  return sstmaci_ep_send(ep, buf, len, dest_addr, context, tag,
                         FabricMessage::no_imm_data, FI_TAGGED);
}

DIRECT_FN STATIC ssize_t sumi_ep_tsendv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t dest_addr,
					uint64_t tag, void *context)
{
  if (count == 1){
    return sstmaci_ep_send(ep, iov[0].iov_base, iov[0].iov_len, dest_addr, context,
                           tag, FabricMessage::no_imm_data, FI_TAGGED);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sumi_ep_tsendmsg(struct fid_ep *ep,
					  const struct fi_msg_tagged *msg,
					  uint64_t flags)
{
  return -FI_ENOSYS;
}


DIRECT_FN STATIC ssize_t sumi_ep_tsenddata(struct fid_ep *ep, const void *buf,
             size_t len, void *desc,
             uint64_t data, fi_addr_t dest_addr,
             uint64_t tag, void *context)
{
  return sstmaci_ep_send(ep, buf, len, dest_addr, context, tag, data,
                         FI_TAGGED | FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t sumi_ep_tinject(struct fid_ep *ep, const void *buf,
					 size_t len, fi_addr_t dest_addr,
					 uint64_t tag)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_tinjectdata(struct fid_ep *ep, const void *buf,
					     size_t len, uint64_t data,
					     fi_addr_t dest_addr, uint64_t tag)
{
  return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sumi_ep_atomic_valid(struct fid_ep *ep,
					  enum fi_datatype datatype,
					  enum fi_op op, size_t *count)
{
  return 0;
}

extern "C" DIRECT_FN  int sumi_ep_fetch_atomic_valid(struct fid_ep *ep,
						enum fi_datatype datatype,
						enum fi_op op, size_t *count)
{
  return 0;
}

extern "C" DIRECT_FN  int sumi_ep_cmp_atomic_valid(struct fid_ep *ep,
					      enum fi_datatype datatype,
					      enum fi_op op, size_t *count)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
		     void *desc, fi_addr_t dest_addr, uint64_t addr,
		     uint64_t key, enum fi_datatype datatype, enum fi_op op,
		     void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
		      size_t count, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
		      fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		      enum fi_datatype datatype, enum fi_op op)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, void *result, void *result_desc,
			 fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			 enum fi_datatype datatype, enum fi_op op,
			 void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			  void **desc, size_t count, struct fi_ioc *resultv,
			  void **result_desc, size_t result_count,
			  fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			    struct fi_ioc *resultv, void **result_desc,
			    size_t result_count, uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sumi_ep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, const void *compare, void *compare_desc,
			 void *result, void *result_desc, fi_addr_t dest_addr,
			 uint64_t addr, uint64_t key, enum fi_datatype datatype,
			 enum fi_op op, void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_atomic_compwritev(struct fid_ep *ep,
						   const struct fi_ioc *iov,
						   void **desc,
						   size_t count,
						   const struct fi_ioc *comparev,
						   void **compare_desc,
						   size_t compare_count,
						   struct fi_ioc *resultv,
						   void **result_desc,
						   size_t result_count,
						   fi_addr_t dest_addr,
						   uint64_t addr, uint64_t key,
						   enum fi_datatype datatype,
						   enum fi_op op,
						   void *context)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sumi_ep_atomic_compwritemsg(struct fid_ep *ep,
						     const struct fi_msg_atomic *msg,
						     const struct fi_ioc *comparev,
						     void **compare_desc,
						     size_t compare_count,
						     struct fi_ioc *resultv,
						     void **result_desc,
						     size_t result_count,
						     uint64_t flags)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_ep_control(fid_t fid, int command, void *arg)
{
  return -FI_ENOSYS;
}

extern "C" int sumi_ep_close(fid_t fid)
{
  sumi_fid_ep* ep = (sumi_fid_ep*) fid;
  free(ep);
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sumi_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
  //this can always be cast to an endpiont regardless of whether
  //it is a simple endpoint or tx/rx context
  sumi_fid_ep* ep = (sumi_fid_ep*) fid;
  FabricTransport* tport = (FabricTransport*) ep->domain->fabric->tport;
  switch(bfid->fclass)
  {
    case FI_CLASS_EQ: {
      sumi_fid_eq* eq = (sumi_fid_eq*) bfid;
      if (ep->domain->fabric != eq->fabric) {
        return -FI_EINVAL;
      }
      ep->eq = eq;
      break;
    }
    case FI_CLASS_CQ: {
      sumi_fid_cq* cq = (sumi_fid_cq*) bfid;

      if (ep->domain != cq->domain) {
        return -FI_EINVAL;
      }

      if (flags & FI_TRANSMIT) {
        if (ep->send_cq) {
          return -FI_EINVAL; //can't rebind send CQ
        }
        ep->send_cq = cq;
        if (flags & FI_SELECTIVE_COMPLETION) {
          //for now, don't allow selective completion
          //all messages will generate completion entries
          return -FI_EINVAL;
        }
      }

      if (flags & FI_RECV) {
        if (ep->recv_cq) {
          return -FI_EINVAL;
        }
        ep->recv_cq = cq;

        if (flags & FI_SELECTIVE_COMPLETION) {
          //for now, don't allow selective completion
          //all messages will generate completion entries
          return -FI_EINVAL;
        }
      }

      RecvQueue* rq = new RecvQueue(SST::Hg::OperatingSystem::currentOs());
      cq->queue = (sumi_progress_queue*) rq;
      tport->allocateCq(cq->id, std::bind(&RecvQueue::incoming, rq, std::placeholders::_1));
      break;
    }
    case FI_CLASS_AV: {
      sumi_fid_av* av = (sumi_fid_av*) bfid;
      if (ep->domain != av->domain) {
        return -FI_EINVAL;
      }
      break;
    }
    case FI_CLASS_CNTR: //TODO
      return -FI_EINVAL;
    case FI_CLASS_MR: //TODO
      return -FI_EINVAL;
    case FI_CLASS_SRX_CTX:

    case FI_CLASS_STX_CTX:
      //we really don't need to do anything here
      //in some sense, all sstmacro endpoints are shared transmit contexts
      break;
    default:
      return -FI_ENOSYS;
      break;

  }
  return FI_SUCCESS;
}


extern "C" DIRECT_FN  int sumi_ep_open(struct fid_domain *domain, struct fi_info *info,
			   struct fid_ep **ep, void *context)
{
  sumi_fid_ep* ep_impl = (sumi_fid_ep*) calloc(1, sizeof(sumi_fid_ep));
  ep_impl->ep_fid.fid.fclass = FI_CLASS_EP;
  ep_impl->ep_fid.fid.context = context;
  ep_impl->ep_fid.fid.ops = &sumi_ep_fi_ops;
  ep_impl->ep_fid.ops = &sumi_ep_ops;
  ep_impl->ep_fid.msg = &sumi_ep_msg_ops;
  ep_impl->ep_fid.rma = &sumi_ep_rma_ops;
  ep_impl->ep_fid.tagged = &sumi_ep_tagged_ops;
  ep_impl->ep_fid.atomic = &sumi_ep_atomic_ops;
  ep_impl->domain = (sumi_fid_domain*) domain;
  ep_impl->caps = info->caps;
  if (info->tx_attr){
    ep_impl->op_flags = info->tx_attr->op_flags;
  }
  if (info->rx_attr){
    ep_impl->op_flags = info->rx_attr->op_flags;
  }

  ep_impl->type = info->ep_attr->type;

  switch(ep_impl->type){
  case FI_EP_DGRAM:
  case FI_EP_RDM:
  case FI_EP_MSG:
    break;
  default:
    return -FI_EINVAL;
  }

  *ep = (fid_ep*) ep_impl;
  return FI_SUCCESS;
}

DIRECT_FN STATIC ssize_t sumi_ep_cancel(fid_t fid, void *context)
{
  return -FI_ENOSYS;
}

ssize_t sumi_cancel(fid_t fid, void *context)
{
  return -FI_ENOSYS;
}

static int
sumi_ep_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context)
{
  return -FI_EINVAL;
}

EXTERN_C DIRECT_FN STATIC  int sumi_ep_getopt(fid_t fid, int level, int optname,
				    void *optval, size_t *optlen)
{
  return -FI_ENOSYS;
}

extern "C" int sumi_getopt(fid_t fid, int level, int optname,
				    void *optval, size_t *optlen)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_ep_setopt(fid_t fid, int level, int optname,
				    const void *optval, size_t optlen)
{
  return -FI_ENOPROTOOPT;
}

extern "C" int sumi_setopt(fid_t fid, int level, int optname,
				    const void *optval, size_t optlen)
{
  return -FI_EINVAL;
}

DIRECT_FN STATIC ssize_t sumi_ep_rx_size_left(struct fid_ep *ep)
{
  return SUMI_RX_SIZE_DEFAULT;
}

DIRECT_FN STATIC ssize_t sumi_ep_tx_size_left(struct fid_ep *ep)
{
  return SUMI_TX_SIZE_DEFAULT;
}

EXTERN_C DIRECT_FN STATIC  int sumi_tx_context(struct fid_ep *ep, int index,
				     struct fi_tx_attr *attr,
				     struct fid_ep **tx_ep, void *context)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_rx_context(struct fid_ep *ep, int index,
				     struct fi_rx_attr *attr,
				     struct fid_ep **rx_ep, void *context)
{
	return -FI_ENOSYS;
}

