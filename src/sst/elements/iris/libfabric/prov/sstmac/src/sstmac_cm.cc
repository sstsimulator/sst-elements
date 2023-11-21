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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sstmac.h"
#include "sstmac_av.h"

#include <sprockit/errors.h>

EXTERN_C DIRECT_FN STATIC  int sstmac_setname(fid_t fid, void *addr, size_t addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_getname(fid_t fid, void *addr, size_t *addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_getpeer(struct fid_ep *ep, void *addr, size_t *addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_connect(struct fid_ep *ep, const void *addr,
				  const void *param, size_t paramlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_accept(struct fid_ep *ep, const void *param,
				 size_t paramlen);
extern "C" DIRECT_FN  int sstmac_pep_open(struct fid_fabric *fabric,
			    struct fi_info *info, struct fid_pep **pep,
			    void *context);
extern "C" DIRECT_FN  int sstmac_pep_bind(struct fid *fid, struct fid *bfid, uint64_t flags);
EXTERN_C DIRECT_FN STATIC  int sstmac_reject(struct fid_pep *pep, fid_t handle,
				 const void *param, size_t paramlen);
DIRECT_FN STATIC int sstmac_shutdown(struct fid_ep *ep, uint64_t flags);
extern "C" DIRECT_FN  int sstmac_pep_listen(struct fid_pep *pep);
EXTERN_C DIRECT_FN STATIC  int sstmac_pep_getopt(fid_t fid, int level, int optname,
             void *optval, size_t *optlen);
static int sstmac_pep_close(fid_t fid);

struct fi_ops_cm sstmac_ep_ops_cm = {
  .size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = sstmac_getpeer,
  .connect = fi_no_connect,
  .listen = fi_no_listen,
  .accept = fi_no_accept,
  .reject = fi_no_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};

struct fi_ops_cm sstmac_ep_msg_ops_cm = {
	.size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = sstmac_getpeer,
  .connect = sstmac_connect,
	.listen = fi_no_listen,
  .accept = sstmac_accept,
	.reject = fi_no_reject,
  .shutdown = sstmac_shutdown,
	.join = fi_no_join,
};

struct fi_ops sstmac_pep_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_pep_close,
  .bind = sstmac_pep_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open,
};

struct fi_ops_ep sstmac_pep_ops_ep = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = fi_no_cancel,
  .getopt = sstmac_pep_getopt,
  .setopt = fi_no_setopt,
  .tx_ctx = fi_no_tx_ctx,
  .rx_ctx = fi_no_rx_ctx,
  .rx_size_left = fi_no_rx_size_left,
  .tx_size_left = fi_no_tx_size_left,
};

struct fi_ops_cm sstmac_pep_ops_cm = {
  .size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = fi_no_getpeer,
  .connect = fi_no_connect,
  .listen = sstmac_pep_listen,
  .accept = fi_no_accept,
  .reject = sstmac_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};

EXTERN_C DIRECT_FN STATIC  int sstmac_getname(fid_t fid, void *addr, size_t *addrlen)
{
  sstmac_fid_ep* ep = (sstmac_fid_ep*) fid;
  if (ep->domain->addr_format == FI_ADDR_STR){

  } else if (ep->domain->addr_format == FI_ADDR_SSTMAC){

  } else {
    spkt_abort_printf("internal error: got bad addr format");
  }

	int ret;
	size_t len = 0, cpylen;
#if 0
  struct sstmac_fid_ep *ep = NULL;
  struct sstmac_fid_sep *sep = NULL;
  struct sstmac_fid_pep *pep = NULL;
	bool is_fi_addr_str;
	struct fi_info *info;
  struct sstmac_ep_name *ep_name;

	if (OFI_UNLIKELY(addrlen == NULL)) {
    SSTMAC_INFO(FI_LOG_EP_CTRL, "parameter \"addrlen\" is NULL in "
      "sstmac_getname\n");
		return -FI_EINVAL;
	}

	switch (fid->fclass) {
	case FI_CLASS_EP:
    ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);
		info = ep->info;
		ep_name = &ep->src_addr;
		break;
	case FI_CLASS_SEP:
    sep = container_of(fid, struct sstmac_fid_sep, ep_fid);
		info = sep->info;
		ep_name = &sep->my_name;
		break;
	case FI_CLASS_PEP:
    pep = container_of(fid, struct sstmac_fid_pep,
				   pep_fid.fid);
		info = pep->info;
		ep_name = &pep->src_addr;
		break;
	default:
    SSTMAC_INFO(FI_LOG_EP_CTRL,
			  "Invalid fid class: %d\n",
			  fid->fclass);
		return -FI_EINVAL;
	}

	is_fi_addr_str = info->addr_format == FI_ADDR_STR;

	if (!addr) {
		if (OFI_UNLIKELY(is_fi_addr_str)) {
      *addrlen = SSTMAC_FI_ADDR_STR_LEN;
		} else {
      *addrlen = sizeof(struct sstmac_ep_name);
		}

		return -FI_ETOOSMALL;
	}

	if (OFI_UNLIKELY(is_fi_addr_str)) {
    ret = _sstmac_ep_name_to_str(ep_name, (char **) &addr);

		if (ret)
			return ret;

    len = SSTMAC_FI_ADDR_STR_LEN;
		cpylen = MIN(len, *addrlen);
	} else {
    len = sizeof(struct sstmac_ep_name);
		cpylen = MIN(len, *addrlen);
		memcpy(addr, ep_name, cpylen);
	}

	*addrlen = len;
#endif
	return (len == cpylen) ? FI_SUCCESS : -FI_ETOOSMALL;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_setname(fid_t fid, void *addr, size_t addrlen)
{
	int ret;
#if 0
  struct sstmac_fid_ep *ep = NULL;
  struct sstmac_fid_sep *sep = NULL;
  struct sstmac_fid_pep *pep = NULL;
	struct fi_info *info;
  struct sstmac_ep_name *ep_name;
	size_t len;

	if (OFI_UNLIKELY(addr == NULL)) {
    SSTMAC_INFO(FI_LOG_EP_CTRL, "parameter \"addr\" is NULL in "
      "sstmac_setname\n");
		return -FI_EINVAL;
	}

  len = sizeof(struct sstmac_ep_name);

	switch (fid->fclass) {
	case FI_CLASS_EP:
    ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);
		info = ep->info;
		ep_name = &ep->src_addr;
		break;
	case FI_CLASS_SEP:
    sep = container_of(fid, struct sstmac_fid_sep, ep_fid);
		info = sep->info;
		ep_name = &sep->my_name;
		break;
	case FI_CLASS_PEP:
    pep = container_of(fid, struct sstmac_fid_pep, pep_fid.fid);
		/* TODO: make sure we're unconnected. */
		pep->bound = 1;
		info = pep->info;
		ep_name = &pep->src_addr;
		break;
	default:
    SSTMAC_INFO(FI_LOG_EP_CTRL, "Invalid fid class: %d\n",
			  fid->fclass);
		return -FI_EINVAL;
	}

	if (OFI_UNLIKELY(info->addr_format == FI_ADDR_STR)) {
    len = SSTMAC_FI_ADDR_STR_LEN;

		if (addrlen != len)
			return -FI_EINVAL;

    ret = _sstmac_ep_name_from_str((const char *) addr,
					     ep_name);

		if (ret)
			return ret;

		return FI_SUCCESS;
	}

	if (addrlen != len)
		return -FI_EINVAL;

	memcpy(ep_name, addr, len);
#endif
	return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_getpeer(struct fid_ep *ep, void *addr,
				  size_t *addrlen)
{
	int ret;
	size_t len = 0, cpylen = 0;
#if 0
  struct sstmac_fid_ep *ep_priv = NULL;
  struct sstmac_fid_sep *sep_priv = NULL;
  struct sstmac_ep_name *ep_name = NULL;
	struct fi_info *info = NULL;

	if (OFI_UNLIKELY(addrlen == NULL || addr == NULL)) {
    SSTMAC_INFO(FI_LOG_EP_CTRL,
        "parameter is NULL in sstmac_getpeer\n");
		return -FI_EINVAL;
	}

	switch (ep->fid.fclass) {
	case FI_CLASS_EP:
    ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid.fid);
		info = ep_priv->info;
		ep_name = &ep_priv->dest_addr;
		break;

	case FI_CLASS_SEP:
    sep_priv = container_of(ep, struct sstmac_fid_sep, ep_fid);
		info = sep_priv->info;
    ep_name = (sstmac_ep_name*) info->dest_addr;
		break;

	default:
    SSTMAC_INFO(FI_LOG_EP_CTRL, "Invalid fid class: %d\n",
			  ep->fid.fclass);
			return -FI_EINVAL;
	}

	if (info->addr_format == FI_ADDR_STR) {
    ret = _sstmac_ep_name_to_str(ep_name, (char **) &addr);

		if (ret)
			return ret;

    len = SSTMAC_FI_ADDR_STR_LEN;
		cpylen = MIN(len, *addrlen);
	} else {
    len = sizeof(struct sstmac_ep_name);
		cpylen = MIN(len, *addrlen);
		memcpy(addr, ep_name, cpylen);
	}

	*addrlen = len;
#endif
	return (len == cpylen) ? FI_SUCCESS : -FI_ETOOSMALL;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_connect(struct fid_ep *ep, const void *addr,
				  const void *param, size_t paramlen)
{
	int ret, errno_keep;
#if 0
  struct sstmac_fid_ep *ep_priv;
	struct sockaddr_in saddr;
  struct sstmac_pep_sock_connreq req;
	struct fi_eq_cm_entry *eqe_ptr;
  struct sstmac_vc *vc;
  struct sstmac_mbox *mbox = NULL;
  struct sstmac_av_addr_entry av_entry;

	if (!ep || !addr || (paramlen && !param) ||
      paramlen > SSTMAC_CM_DATA_MAX_SIZE)
		return -FI_EINVAL;

  ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid.fid);

	COND_ACQUIRE(ep_priv->requires_lock, &ep_priv->vc_lock);

  if (ep_priv->conn_state != SSTMAC_EP_UNCONNECTED) {
		ret = -FI_EINVAL;
		goto err_unlock;
	}

  ret = _sstmac_pe_to_ip((sstmac_ep_name *)addr, &saddr);
	if (ret != FI_SUCCESS) {
    SSTMAC_INFO(FI_LOG_EP_CTRL,
        "Failed to translate sstmac_ep_name to IP\n");
		goto err_unlock;
	}

	/* Create new VC without CM data. */
  av_entry.sstmac_addr = ep_priv->dest_addr.sstmac_addr;
	av_entry.cm_nic_cdm_id = ep_priv->dest_addr.cm_nic_cdm_id;
  ret = _sstmac_vc_alloc(ep_priv, &av_entry, &vc);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to create VC:: %d\n",
			  ret);
		goto err_unlock;
	}
	ep_priv->vc = vc;

  ret = _sstmac_mbox_alloc(vc->ep->nic->mbox_hndl, &mbox);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_DATA,
        "_sstmac_mbox_alloc returned %s\n",
			  fi_strerror(-ret));
		goto err_mbox_alloc;
	}
	vc->smsg_mbox = mbox;

	ep_priv->conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ep_priv->conn_fd < 0) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to create connect socket, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_ENOSPC;
		goto err_socket;
	}

	/* Currently blocks until connected. */
	ret = connect(ep_priv->conn_fd, (struct sockaddr *)&saddr,
		      sizeof(saddr));
	if (ret) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to connect, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_EIO;
		goto err_connect;
	}

	memset(&req, 0, sizeof(req));
	req.info = *ep_priv->info;

	/* Note addrs are swapped. */
	memcpy(&req.dest_addr, (void *)&ep_priv->src_addr,
	       sizeof(req.dest_addr));
	memcpy(&ep_priv->dest_addr, addr, sizeof(ep_priv->dest_addr));
	memcpy(&req.src_addr, addr, sizeof(req.src_addr));

	if (ep_priv->info->tx_attr)
		req.tx_attr = *ep_priv->info->tx_attr;
	if (ep_priv->info->rx_attr)
		req.rx_attr = *ep_priv->info->rx_attr;
	if (ep_priv->info->ep_attr)
		req.ep_attr = *ep_priv->info->ep_attr;
	if (ep_priv->info->domain_attr)
		req.domain_attr = *ep_priv->info->domain_attr;
	if (ep_priv->info->fabric_attr)
		req.fabric_attr = *ep_priv->info->fabric_attr;

	req.vc_id = vc->vc_id;
	req.vc_mbox_attr.msg_type = SSTMAC_SMSG_TYPE_MBOX_AUTO_RETRANSMIT;
	req.vc_mbox_attr.msg_buffer = mbox->base;
	req.vc_mbox_attr.buff_size =  vc->ep->nic->mem_per_mbox;
	req.vc_mbox_attr.mem_hndl = *mbox->memory_handle;
	req.vc_mbox_attr.mbox_offset = (uint64_t)mbox->offset;
	req.vc_mbox_attr.mbox_maxcredit =
			ep_priv->domain->params.mbox_maxcredit;
	req.vc_mbox_attr.msg_maxsize = ep_priv->domain->params.mbox_msg_maxsize;
	req.cq_irq_mdh = ep_priv->nic->irq_mem_hndl;
	req.peer_caps = ep_priv->caps;
	req.key_offset = ep_priv->auth_key->key_offset;

	req.cm_data_len = paramlen;
	if (paramlen) {
		eqe_ptr = (struct fi_eq_cm_entry *)req.eqe_buf;
		memcpy(eqe_ptr->data, param, paramlen);
	}

	ret = write(ep_priv->conn_fd, &req, sizeof(req));
	if (ret != sizeof(req)) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to send req, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_EIO;
		goto err_write;
	}
	/* set fd to non-blocking now since we can't block within the eq
	 * progress system
	 */
	fi_fd_nonblock(ep_priv->conn_fd);

  ep_priv->conn_state = SSTMAC_EP_CONNECTING;

	COND_RELEASE(ep_priv->requires_lock, &ep_priv->vc_lock);

  SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Sent conn req: %p, %s\n",
		   ep_priv, inet_ntoa(saddr.sin_addr));

	return FI_SUCCESS;

err_write:
err_connect:
	close(ep_priv->conn_fd);
	ep_priv->conn_fd = -1;
err_socket:
  _sstmac_mbox_free((sstmac_mbox*) ep_priv->vc->smsg_mbox);
	ep_priv->vc->smsg_mbox = NULL;
err_mbox_alloc:
  _sstmac_vc_destroy(ep_priv->vc);
	ep_priv->vc = NULL;
err_unlock:
	COND_RELEASE(ep_priv->requires_lock, &ep_priv->vc_lock);
#endif
	return ret;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_accept(struct fid_ep *ep, const void *param,
				 size_t paramlen)
{
	int ret, errno_keep;
#if 0
  struct sstmac_vc *vc;
  struct sstmac_fid_ep *ep_priv;
  struct sstmac_pep_sock_conn *conn;
  struct sstmac_pep_sock_connresp resp;
	struct fi_eq_cm_entry eq_entry, *eqe_ptr;
  struct sstmac_mbox *mbox = NULL;
  struct sstmac_av_addr_entry av_entry;

  if (!ep || (paramlen && !param) || paramlen > SSTMAC_CM_DATA_MAX_SIZE)
		return -FI_EINVAL;

  ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid.fid);

	COND_ACQUIRE(ep_priv->requires_lock, &ep_priv->vc_lock);

	/* Look up and unpack the connection request used to create this EP. */
  conn = (struct sstmac_pep_sock_conn *)ep_priv->info->handle;
	if (!conn || conn->fid.fclass != FI_CLASS_CONNREQ) {
		ret = -FI_EINVAL;
		goto err_unlock;
	}

	/* Create new VC without CM data. */
  av_entry.sstmac_addr = ep_priv->dest_addr.sstmac_addr;
	av_entry.cm_nic_cdm_id = ep_priv->dest_addr.cm_nic_cdm_id;
  ret = _sstmac_vc_alloc(ep_priv, &av_entry, &vc);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL, "Failed to create VC: %d\n", ret);
		goto err_unlock;
	}
	ep_priv->vc = vc;
	ep_priv->vc->peer_caps = conn->req.peer_caps;
	ep_priv->vc->peer_key_offset = conn->req.key_offset;
	ep_priv->vc->peer_id = conn->req.vc_id;

  ret = _sstmac_mbox_alloc(vc->ep->nic->mbox_hndl, &mbox);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_DATA, "_sstmac_mbox_alloc returned %s\n",
			  fi_strerror(-ret));
		goto err_mbox_alloc;
	}
	vc->smsg_mbox = mbox;

	/* Initialize the SSTMAC connection. */
  ret = _sstmac_vc_smsg_init(vc, conn->req.vc_id,
				 &conn->req.vc_mbox_attr,
				 &conn->req.cq_irq_mdh);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
        "_sstmac_vc_smsg_init returned %s\n",
			  fi_strerror(-ret));
		goto err_smsg_init;
	}

  vc->conn_state = SSTMAC_VC_CONNECTED;

	/* Send ACK with VC attrs to allow peer to initialize SSTMAC connection. */
  resp.cmd = SSTMAC_PEP_SOCK_RESP_ACCEPT;

	resp.vc_id = vc->vc_id;
	resp.vc_mbox_attr.msg_type = SSTMAC_SMSG_TYPE_MBOX_AUTO_RETRANSMIT;
	resp.vc_mbox_attr.msg_buffer = mbox->base;
	resp.vc_mbox_attr.buff_size =  vc->ep->nic->mem_per_mbox;
	resp.vc_mbox_attr.mem_hndl = *mbox->memory_handle;
	resp.vc_mbox_attr.mbox_offset = (uint64_t)mbox->offset;
	resp.vc_mbox_attr.mbox_maxcredit =
			ep_priv->domain->params.mbox_maxcredit;
	resp.vc_mbox_attr.msg_maxsize =
			ep_priv->domain->params.mbox_msg_maxsize;
	resp.cq_irq_mdh = ep_priv->nic->irq_mem_hndl;
	resp.peer_caps = ep_priv->caps;
	resp.key_offset = ep_priv->auth_key->key_offset;

	resp.cm_data_len = paramlen;
	if (paramlen) {
		eqe_ptr = (struct fi_eq_cm_entry *)resp.eqe_buf;
		memcpy(eqe_ptr->data, param, paramlen);
	}

	ret = write(conn->sock_fd, &resp, sizeof(resp));
	if (ret != sizeof(resp)) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to send resp, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_EIO;
		goto err_write;
	}

	/* Notify user that this side is connected. */
	eq_entry.fid = &ep_priv->ep_fid.fid;

	ret = fi_eq_write(&ep_priv->eq->eq_fid, FI_CONNECTED, &eq_entry,
			  sizeof(eq_entry), 0);
	if (ret != sizeof(eq_entry)) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "fi_eq_write failed, err: %d\n", ret);
		goto err_eq_write;
	}

	/* Free the connection request. */
	free(conn);

  ep_priv->conn_state = SSTMAC_EP_CONNECTED;

	COND_RELEASE(ep_priv->requires_lock, &ep_priv->vc_lock);

  SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Sent conn accept: %p\n", ep_priv);

	return FI_SUCCESS;

err_eq_write:
err_write:
err_smsg_init:
  _sstmac_mbox_free((sstmac_mbox *)ep_priv->vc->smsg_mbox);
	ep_priv->vc->smsg_mbox = NULL;
err_mbox_alloc:
  _sstmac_vc_destroy(ep_priv->vc);
	ep_priv->vc = NULL;
err_unlock:
	COND_RELEASE(ep_priv->requires_lock, &ep_priv->vc_lock);
#endif
	return ret;
}

DIRECT_FN STATIC int sstmac_shutdown(struct fid_ep *ep, uint64_t flags)
{
	int ret;
#if 0
  struct sstmac_fid_ep *ep_priv;
	struct fi_eq_cm_entry eq_entry = {0};

	if (!ep)
		return -FI_EINVAL;

  ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid.fid);

	COND_ACQUIRE(ep_priv->requires_lock, &ep_priv->vc_lock);

	eq_entry.fid = &ep_priv->ep_fid.fid;

	ret = fi_eq_write(&ep_priv->eq->eq_fid, FI_SHUTDOWN, &eq_entry,
			  sizeof(eq_entry), 0);
	if (ret != sizeof(eq_entry)) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "fi_eq_write failed, err: %d\n", ret);
	} else {
		ret = FI_SUCCESS;
	}

	COND_RELEASE(ep_priv->requires_lock, &ep_priv->vc_lock);
#endif
	return ret;
}


/******************************************************************************
 *
 * Passive endpoint handling
 *
 *****************************************************************************/

EXTERN_C DIRECT_FN STATIC  int sstmac_pep_getopt(fid_t fid, int level, int optname,
				     void *optval, size_t *optlen)
{
#if 0
	if (!fid || !optval || !optlen)
		return -FI_EINVAL;
	else if (level != FI_OPT_ENDPOINT)
		return -FI_ENOPROTOOPT;

	switch (optname) {
	case FI_OPT_CM_DATA_SIZE:
    *(size_t *)optval = SSTMAC_CM_DATA_MAX_SIZE;
		*optlen = sizeof(size_t);
		break;
	default:
		return -FI_ENOPROTOOPT;
	}
#endif
	return 0;
}

static int sstmac_pep_close(fid_t fid)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_pep *pep;
	int references_held;

  pep = container_of(fid, struct sstmac_fid_pep, pep_fid.fid);

  references_held = _sstmac_ref_put(pep);
	if (references_held)
    SSTMAC_INFO(FI_LOG_EP_CTRL, "failed to fully close pep due "
			  "to lingering references. references=%i pep=%p\n",
			  references_held, pep);
#endif
	return ret;
}

extern "C" DIRECT_FN  int sstmac_pep_bind(struct fid *fid, struct fid *bfid, uint64_t flags)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_pep  *pep;
  struct sstmac_fid_eq *eq;

	if (!fid || !bfid)
		return -FI_EINVAL;

  pep = container_of(fid, struct sstmac_fid_pep, pep_fid.fid);

	fastlock_acquire(&pep->lock);

	switch (bfid->fclass) {
	case FI_CLASS_EQ:
    eq = container_of(bfid, struct sstmac_fid_eq, eq_fid.fid);
		if (pep->fabric != eq->fabric) {
			ret = -FI_EINVAL;
			break;
		}

		if (pep->eq) {
			ret = -FI_EINVAL;
			break;
		}

		pep->eq = eq;
    _sstmac_eq_poll_obj_add(eq, &pep->pep_fid.fid);
    _sstmac_ref_get(eq);

    SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Bound EQ to PEP: %p, %p\n",
			   eq, pep);
		break;
	default:
		ret = -FI_ENOSYS;
		break;
	}

	fastlock_release(&pep->lock);
#endif
	return ret;
}

extern "C" DIRECT_FN  int sstmac_pep_listen(struct fid_pep *pep)
{
	int ret, errno_keep;
#if 0
  struct sstmac_fid_pep *pep_priv;
	struct sockaddr_in saddr;
	int sockopt = 1;

	if (!pep)
		return -FI_EINVAL;

  pep_priv = container_of(pep, struct sstmac_fid_pep, pep_fid.fid);

	fastlock_acquire(&pep_priv->lock);

	if (!pep_priv->eq) {
		ret = -FI_EINVAL;
		goto err_unlock;
	}

	pep_priv->listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (pep_priv->listen_fd < 0) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to create listening socket, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_ENOSPC;
		goto err_unlock;
	}

	ret = setsockopt(pep_priv->listen_fd, SOL_SOCKET, SO_REUSEADDR,
			 &sockopt, sizeof(sockopt));
	if (ret < 0) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "setsockopt(SO_REUSEADDR) failed, err: %s\n",
			  strerror(errno_keep));
	}

	/* Bind to the ipogif interface using resolved service number as CDM
	 * ID. */
  ret = _sstmac_local_ipaddr(&saddr);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL, "Failed to find local IP\n");
		ret = -FI_ENOSPC;
		goto err_sock;
	}

	/* If source addr was not specified, use auto assigned port. */
	if (pep_priv->bound)
    saddr.sin_port = pep_priv->src_addr.sstmac_addr.cdm_id;
	else
		saddr.sin_port = 0;

	ret = bind(pep_priv->listen_fd, &saddr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to bind listening socket, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_ENOSPC;
		goto err_sock;
	}

	ret = listen(pep_priv->listen_fd, pep_priv->backlog);
	if (ret < 0) {
		errno_keep = errno;
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to start listening socket, err: %s\n",
			  strerror(errno_keep));
		ret = -FI_ENOSPC;
		goto err_sock;
	}

	fastlock_release(&pep_priv->lock);

  SSTMAC_DEBUG(FI_LOG_EP_CTRL,
		   "Configured PEP for listening: %p (%s:%d)\n",
		   pep, inet_ntoa(saddr.sin_addr), saddr.sin_port);

	return FI_SUCCESS;

err_sock:
	close(pep_priv->listen_fd);
err_unlock:
	fastlock_release(&pep_priv->lock);
#endif
	return ret;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_listen(struct fid_pep *pep)
{
        return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_reject(struct fid_pep *pep, fid_t handle,
				 const void *param, size_t paramlen)
{
#if 0
  struct sstmac_fid_pep *pep_priv;
  struct sstmac_pep_sock_conn *conn;
  struct sstmac_pep_sock_connresp resp;
	struct fi_eq_cm_entry *eqe_ptr;
	int ret;

	if (!pep)
		return -FI_EINVAL;

  pep_priv = container_of(pep, struct sstmac_fid_pep, pep_fid.fid);

	fastlock_acquire(&pep_priv->lock);

  conn = (struct sstmac_pep_sock_conn *)handle;
	if (!conn || conn->fid.fclass != FI_CLASS_CONNREQ) {
		fastlock_release(&pep_priv->lock);
		return -FI_EINVAL;
	}

  resp.cmd = SSTMAC_PEP_SOCK_RESP_REJECT;

	resp.cm_data_len = paramlen;
	if (paramlen) {
		eqe_ptr = (struct fi_eq_cm_entry *)resp.eqe_buf;
		memcpy(eqe_ptr->data, param, paramlen);
	}

	ret = write(conn->sock_fd, &resp, sizeof(resp));
	if (ret != sizeof(resp)) {
		fastlock_release(&pep_priv->lock);
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "Failed to send resp, errno: %d\n",
			  errno);
		return -FI_EIO;
	}

	close(conn->sock_fd);
	free(conn);

	fastlock_release(&pep_priv->lock);

  SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Sent conn reject: %p\n", pep_priv);
#endif
	return FI_SUCCESS;
}


extern "C" DIRECT_FN  int sstmac_pep_open(struct fid_fabric *fabric,
			    struct fi_info *info, struct fid_pep **pep,
			    void *context)
{
#if 0
  struct sstmac_fid_fabric *fabric_priv;
  struct sstmac_fid_pep *pep_priv;
  struct sstmac_ep_name *ep_name;

	if (!fabric || !info || !pep)
		return -FI_EINVAL;

  fabric_priv = container_of(fabric, struct sstmac_fid_fabric, fab_fid);

  pep_priv = (sstmac_fid_pep *) calloc(1, sizeof(*pep_priv));
	if (!pep_priv)
		return -FI_ENOMEM;

	pep_priv->pep_fid.fid.fclass = FI_CLASS_PEP;
	pep_priv->pep_fid.fid.context = context;

  pep_priv->pep_fid.fid.ops = &sstmac_pep_fi_ops;
  pep_priv->pep_fid.ops = &sstmac_pep_ops_ep;
  pep_priv->pep_fid.cm = &sstmac_pep_ops_cm;
	pep_priv->fabric = fabric_priv;
	pep_priv->info = fi_dupinfo(info);
	pep_priv->info->addr_format = info->addr_format;

	pep_priv->listen_fd = -1;
	pep_priv->backlog = 5; /* TODO set via fi_control parameter. */
	fastlock_init(&pep_priv->lock);

	if (info->src_addr) {
    ep_name = (sstmac_ep_name *) info->src_addr;
		info->src_addrlen = sizeof(struct sockaddr_in);

		pep_priv->bound = 1;
		memcpy(&pep_priv->src_addr, ep_name, info->src_addrlen);
	} else {
		pep_priv->bound = 0;
	}

  _sstmac_ref_init(&pep_priv->ref_cnt, 1, __pep_destruct);

	*pep = &pep_priv->pep_fid;

  SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Opened PEP: %p\n", pep_priv);
#endif
	return FI_SUCCESS;
}



