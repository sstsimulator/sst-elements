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

#include <string.h>
#include <assert.h>

#include "sumi_prov.h"
#include "sumi_wait.h"

#include <sumi_fabric.hpp>

static int sumi_cq_close(fid_t fid);
static int sumi_cq_control(struct fid *cq, int command, void *arg);
DIRECT_FN STATIC ssize_t sumi_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr);
DIRECT_FN STATIC ssize_t sumi_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sumi_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout);
extern "C" DIRECT_FN  int sumi_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context);
DIRECT_FN STATIC const char *sumi_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len);
EXTERN_C DIRECT_FN STATIC  int sumi_cq_signal(struct fid_cq *cq);
DIRECT_FN STATIC ssize_t sumi_cq_read(struct fid_cq *cq,
				      void *buf,
				      size_t count);
DIRECT_FN STATIC ssize_t sumi_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout);

static const struct fi_ops sumi_cq_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sumi_cq_close,
  .bind = fi_no_bind,
  .control = sumi_cq_control,
  .ops_open = fi_no_ops_open
};

static const struct fi_ops_cq sumi_cq_ops = {
  .size = sizeof(struct fi_ops_cq),
  .read = sumi_cq_read,
  .readfrom = sumi_cq_readfrom,
  .readerr = sumi_cq_readerr,
  .sread = sumi_cq_sread,
  .sreadfrom = sumi_cq_sreadfrom,
  .signal = sumi_cq_signal,
  .strerror = sumi_cq_strerror
};

static void* sstmaci_fill_cq_entry(enum fi_cq_format format, void* buf, FabricMessage* msg)
{
  FabricMessage* fmsg = static_cast<FabricMessage*>(msg);
  switch (format){
    case FI_CQ_FORMAT_UNSPEC:
    case FI_CQ_FORMAT_CONTEXT: {
      fi_cq_entry* entry = (fi_cq_entry*) buf;
      entry->op_context = msg->context();
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_MSG: {
      fi_cq_msg_entry* entry = (fi_cq_msg_entry*) buf;
      entry->op_context = msg->context();
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_DATA: {
      fi_cq_data_entry* entry = (fi_cq_data_entry*) buf;
      entry->op_context = msg->context();
      entry->buf = msg->localBuffer();
      entry->data = fmsg->immData();
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_TAGGED: {
      fi_cq_tagged_entry* entry = (fi_cq_tagged_entry*) buf;
      entry->op_context = msg->context();
      entry->buf = msg->localBuffer();
      entry->data = fmsg->immData();
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry->tag = fmsg->tag();
      entry++;
      return entry;
    }
  }
  return nullptr;
}

static int sumi_cq_close(fid_t fid)
{
  sumi_fid_cq* cq_impl = (sumi_fid_cq*) fid;
  FabricTransport* tport = (FabricTransport*) cq_impl->domain->fabric->tport;
  tport->deallocateCq(cq_impl->id);
  free(cq_impl);
	return FI_SUCCESS;
}

static ssize_t sstmaci_cq_read(bool blocking,
                        struct fid_cq *cq, void *buf,
                        size_t count, fi_addr_t *src_addr,
                        const void *cond, int timeout)
{
  sumi_fid_cq* cq_impl = (sumi_fid_cq*) cq;
  FabricTransport* tport = (FabricTransport*) cq_impl->domain->fabric->tport;
  RecvQueue* rq = (RecvQueue*) cq_impl->queue;

  size_t done = 0;
  while (done < count){
    double timeout_s = (blocking && timeout > 0) ? timeout*1e-3 : -1;
    SST::Iris::sumi::Message* msg = rq->progress.front(blocking, timeout_s);
    if (!msg){
      break;
    }
    if (src_addr){
      src_addr[done] = msg->sender();
    }
    buf = sstmaci_fill_cq_entry(cq_impl->format, buf, static_cast<FabricMessage*>(msg));
    done++;
  }
  return done ? done : -FI_EAGAIN;
}

DIRECT_FN STATIC ssize_t sumi_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout)
{
  return sstmaci_cq_read(true, cq, buf, count, src_addr, cond, timeout);
}

DIRECT_FN STATIC ssize_t sumi_cq_read(struct fid_cq *cq,
                void *buf, size_t count)
{
  return sstmaci_cq_read(false, cq, buf, count, nullptr, nullptr, -1);
}

DIRECT_FN STATIC ssize_t sumi_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout)
{
  return sstmaci_cq_read(true, cq, buf, count, nullptr, cond, timeout);
}

DIRECT_FN STATIC ssize_t sumi_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr)
{
  return sstmaci_cq_read(false, cq, buf, count, src_addr, nullptr, -1);
}

DIRECT_FN STATIC ssize_t sumi_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
					 uint64_t flags)
{
  //there are never any errors in the simulator!
  return -FI_EINVAL;
}

DIRECT_FN STATIC const char *sumi_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len)
{
	return NULL;
}

EXTERN_C DIRECT_FN STATIC  int sumi_cq_signal(struct fid_cq *cq)
{
  sst_hg_abort_printf("unimplemented: ssmac_cq_signal");
	return FI_SUCCESS;
}

static int sumi_cq_control(struct fid *cq, int command, void *arg)
{

	switch (command) {
	case FI_GETWAIT:
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
}

extern "C" DIRECT_FN  int sumi_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context)
{
  sumi_fid_domain* domain_impl = (sumi_fid_domain*) domain;
  FabricTransport* tport = (FabricTransport*) domain_impl;
  int id = tport->allocateCqId();
  sumi_fid_cq* cq_impl = (sumi_fid_cq*) calloc(1, sizeof(sumi_fid_cq));
  cq_impl->domain = domain_impl;
  cq_impl->id = id;
  cq_impl->format = attr->format;

  struct fi_wait_attr requested = {
    .wait_obj = attr->wait_obj,
    .flags = 0
  };

  switch (attr->wait_obj) {
  case FI_WAIT_UNSPEC:
  case FI_WAIT_FD:
  case FI_WAIT_MUTEX_COND:
    sumi_wait_open(&cq_impl->domain->fabric->fab_fid, &requested, &cq_impl->wait);
    break;
  case FI_WAIT_SET:
    sstmaci_add_wait(attr->wait_set, &cq_impl->cq_fid.fid);
    break;
  default:
    break;
  }
  *cq = (fid_cq*) cq_impl;
  return FI_SUCCESS;
}

void RecvQueue::finishMatch(void* buf, uint32_t size, FabricMessage *msg)
{
  //found a match
  if (size >= msg->payloadBytes()){
    if (buf && msg->localBuffer()){
      msg->matchRecv(buf);
    }
    progress.incoming(msg);
  } else {
    delete msg;
  }
}

void RecvQueue::matchTaggedRecv(FabricMessage* msg){
  for (auto it = tagged_recvs.begin(); it != tagged_recvs.end(); ++it){
    auto tmp = it++;
    TaggedRecv& r = *tmp;
    if (matches(msg, r.tag, r.tag_ignore)){
      finishMatch(r.buf, r.size, msg);
      tagged_recvs.erase(tmp);
      return;
    }
  }
  unexp_tagged_recvs.push_back(msg);
}

void RecvQueue::postRecv(uint32_t size, void* buf, uint64_t tag, uint64_t tag_ignore, bool tagged){
  if (tagged){
    if (unexp_tagged_recvs.empty()){
      tagged_recvs.emplace_back(size, buf, tag, tag_ignore);
    } else {
      for (auto it = unexp_tagged_recvs.begin(); it != unexp_tagged_recvs.end(); ++it){
        auto tmp = it++;
        FabricMessage* msg = *tmp;
        if (matches(msg, tag, tag_ignore)){
          finishMatch(buf, size, msg);
          return;
        }
      }
    }
    //nothing matched
    tagged_recvs.emplace_back(size, buf, tag, tag_ignore);
  } else {
    if (unexp_recvs.empty()){
      recvs.emplace_back(size, buf);
    } else {
      FabricMessage* msg = unexp_recvs.front();
      unexp_recvs.pop_front();;
      finishMatch(buf, size, msg);
    }
  }
}

void RecvQueue::incoming(SST::Iris::sumi::Message* msg){
  FabricMessage* fmsg = static_cast<FabricMessage*>(msg);
  if (fmsg->SST::Hg::NetworkMessage::type() == SST::Hg::NetworkMessage::posted_send){
    if (fmsg->flags() & FI_TAGGED){
      matchTaggedRecv(fmsg);
    } else {
      if (recvs.empty()){
        unexp_recvs.push_back(fmsg);
      } else {
        Recv& r = recvs.front();
        finishMatch(r.buf, r.size, fmsg);
        recvs.pop_front();
      }
    }
  } else {
    //all other messages go right through
    progress.incoming(fmsg);
  }
}

