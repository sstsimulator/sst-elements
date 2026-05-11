// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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
#include <stdlib.h>
#include <assert.h>

#include "sumi_prov.h"
#include "sumi_wait.h"

#include <sumi_fabric.hpp>
#include <mercury/components/operating_system.h>

// Simulated idle (in ns) applied to non-blocking CQ reads that find no
// completion. Default 1 ns prevents the simulator from burning unbounded
// simulated cycles in a pure spin (the native OFI behavior). Override with
// SUMI_CQ_NONBLOCK_YIELD_NS; set to 0 to disable the yield. Resolved once.
static uint64_t sumi_cq_nonblock_yield_ns()
{
  static uint64_t cached = [](){
    uint64_t v = 1;
    if (const char* env = getenv("SUMI_CQ_NONBLOCK_YIELD_NS")){
      char* end = nullptr;
      long long parsed = strtoll(env, &end, 0);
      if (end != env && parsed >= 0){
        v = (uint64_t)parsed;
      }
    }
    return v;
  }();
  return cached;
}

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
  // Intentionally do not delete cq_impl->queue (RecvQueue) here. In MV2 the
  // CQ close happens during simulator teardown; deleting the RecvQueue at
  // this point can strand threads still blocked in its progress queue or
  // race with late callbacks. The RecvQueue is leaked until process exit,
  // which is acceptable for the finalize-only call path we exercise today.
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

  double pragma_timeout = -1;
  tport->configureNextPoll(blocking, pragma_timeout);

  size_t done = 0;
  while (done < count){
    double timeout_s = -1;
    if (blocking && timeout > 0) {
      timeout_s = timeout * 1e-3;
    } else if (blocking && pragma_timeout > 0) {
      timeout_s = pragma_timeout;
    }
    SST::Iris::sumi::Message* msg = rq->progress.front(blocking, timeout_s);
    if (!msg){
      if (!blocking) {
        uint64_t ns = sumi_cq_nonblock_yield_ns();
        if (ns){
          auto* os = SST::Hg::OperatingSystem::currentOs();
          // Caveat: blockTimeout yields the simulated thread. If a caller
          // holds a pthread mutex across fi_cq_read, another thread that needs
          // that mutex to make progress can deadlock. MV2's progress engine
          // does not hold mutexes here, but future callers should avoid that
          // pattern. Set SUMI_CQ_NONBLOCK_YIELD_NS=0 to disable the yield.
          os->blockTimeout(SST::Hg::TimeDelta(ns * 1e-9));
        }
      }
      break;
    }
    if (src_addr){
      src_addr[done] = msg->sender();
    }
    buf = sstmaci_fill_cq_entry(cq_impl->format, buf, static_cast<FabricMessage*>(msg));
    rq->progress.pop();
    delete msg;
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
  FabricTransport* tport = (FabricTransport*) domain_impl->fabric->tport;
  int id = tport->allocateCqId();
  sumi_fid_cq* cq_impl = (sumi_fid_cq*) calloc(1, sizeof(sumi_fid_cq));
  cq_impl->cq_fid.fid.fclass = FI_CLASS_CQ;
  cq_impl->cq_fid.fid.context = context;
  cq_impl->cq_fid.fid.ops = const_cast<fi_ops*>(&sumi_cq_fi_ops);
  cq_impl->cq_fid.ops = const_cast<fi_ops_cq*>(&sumi_cq_ops);
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
  // it++ in body so we can erase tmp
  for (auto it = tagged_recvs.begin(); it != tagged_recvs.end(); ){
    auto tmp = it++;
    TaggedRecv& r = *tmp;
    if (srcMatches(r.src_rank, msg) && matches(msg, r.tag, r.tag_ignore)){
      finishMatch(r.buf, r.size, msg);
      tagged_recvs.erase(tmp);
      return;
    }
  }
  unexp_tagged_recvs.push_back(msg);
}

void RecvQueue::postRecv(uint32_t size, void* buf, uint64_t tag,
                         uint64_t tag_ignore, bool tagged, uint32_t src_rank){
  if (tagged){
    // it++ in body so we can erase tmp
    for (auto it = unexp_tagged_recvs.begin(); it != unexp_tagged_recvs.end(); ){
      auto tmp = it++;
      FabricMessage* msg = *tmp;
      if (srcMatches(src_rank, msg) && matches(msg, tag, tag_ignore)){
        finishMatch(buf, size, msg);
        unexp_tagged_recvs.erase(tmp);
        return;
      }
    }
    //nothing matched
    tagged_recvs.emplace_back(size, buf, tag, tag_ignore, src_rank);
  } else {
    // it++ in body so we can erase tmp
    for (auto it = unexp_recvs.begin(); it != unexp_recvs.end(); ){
      auto tmp = it++;
      FabricMessage* msg = *tmp;
      if (srcMatches(src_rank, msg)){
        finishMatch(buf, size, msg);
        unexp_recvs.erase(tmp);
        return;
      }
    }
    recvs.emplace_back(size, buf, src_rank);
  }
}

void RecvQueue::incoming(SST::Iris::sumi::Message* msg){
  FabricMessage* fmsg = static_cast<FabricMessage*>(msg);
  if (fmsg->SST::Hg::NetworkMessage::type() == SST::Hg::NetworkMessage::posted_send){
    if (fmsg->flags() & FI_TAGGED){
      matchTaggedRecv(fmsg);
    } else {
      // it++ in body so we can erase tmp
      for (auto it = recvs.begin(); it != recvs.end(); ){
        auto tmp = it++;
        Recv& r = *tmp;
        if (srcMatches(r.src_rank, fmsg)){
          finishMatch(r.buf, r.size, fmsg);
          recvs.erase(tmp);
          return;
        }
      }
      unexp_recvs.push_back(fmsg);
    }
  } else {
    //all other messages go right through
    progress.incoming(fmsg);
  }
}

