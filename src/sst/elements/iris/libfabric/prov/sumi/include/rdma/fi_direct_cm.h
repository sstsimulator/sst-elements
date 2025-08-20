/*
 * Copyright (c) 2015-2016 Los Alamos National Security, LLC. All
 * rights reserved.
 * Copyright (c) 2015-2016 Cray Inc.  All rights reserved.
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

#ifndef _FI_DIRECT_CM_H_
#define _FI_DIRECT_CM_H_

#define FABRIC_DIRECT_CM 1

/*******************************************************************************
 * GNI API Functions
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>

extern int sumi_setname(fid_t fid, void *addr, size_t addrlen);

extern int sumi_getname(fid_t fid, void *addr, size_t *addrlen);

extern int sumi_getpeer(struct fid_ep *ep, void *addr, size_t *addrlen);

extern int sumi_listen(struct fid_pep *pep);

extern int sumi_connect(struct fid_ep *ep, const void *addr, const void *param,
			size_t paramlen);

extern int sumi_accept(struct fid_ep *ep, const void *param, size_t paramlen);

extern int sumi_reject(struct fid_pep *pep, fid_t handle, const void *param,
		       size_t paramlen);

extern int sumi_shutdown(struct fid_ep *ep, uint64_t flags);

/*******************************************************************************
 * Libfabric API Functions
 ******************************************************************************/
static inline int fi_setname(fid_t fid, void *addr, size_t addrlen)
{
	return sumi_setname(fid, addr, addrlen);
}

static inline int fi_getname(fid_t fid, void *addr, size_t *addrlen)
{
	return sumi_getname(fid, addr, addrlen);
}

static inline int fi_getpeer(struct fid_ep *ep, void *addr, size_t *addrlen)
{
	return sumi_getpeer(ep, addr, addrlen);
}

static inline int fi_listen(struct fid_pep *pep)
{
	return sumi_listen(pep);
}

static inline int fi_connect(struct fid_ep *ep, const void *addr,
			     const void *param, size_t paramlen)
{
	return sumi_connect(ep, addr, param, paramlen);
}

static inline int fi_accept(struct fid_ep *ep, const void *param,
			    size_t paramlen)
{
	return sumi_accept(ep, param, paramlen);
}

static inline int fi_reject(struct fid_pep *pep, fid_t handle,
			    const void *param, size_t paramlen)
{
	return sumi_reject(pep, handle, param, paramlen);
}

static inline int fi_shutdown(struct fid_ep *ep, uint64_t flags)
{
	return sumi_shutdown(ep, flags);
}

#endif /* _FI_DIRECT_CM_H_ */
