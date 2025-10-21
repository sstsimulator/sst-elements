/*
 * Copyright (c) Intel Corporation. All rights reserved.
 * Copyright (c) 2015-2019 Cisco Systems, Inc. All rights reserved.
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

#ifndef HOOK_PROV_H
#define HOOK_PROV_H

#include <stdint.h>
#include <ofi.h>
#include "ofi_hook.h"

int hook_bind(struct fid *fid, struct fid *bfid, uint64_t flags);
int hook_control(struct fid *fid, int command, void *arg);
int hook_ops_open(struct fid *fid, const char *name,
			 uint64_t flags, void **ops, void *context);
int hook_close(struct fid *fid);

#if HAVE_PERF
#include "hook_perf.h"
#else
#define perf_msg_ops hook_msg_ops
#define perf_rma_ops hook_rma_ops
#define perf_tagged_ops hook_tagged_ops
#define perf_cntr_ops hook_cntr_ops
#define perf_cq_ops hook_cq_ops

#define hook_perf_create hook_fabric_create
#define hook_perf_destroy hook_fabric_destroy

#endif /* HAVE_PERF */
#endif /* HOOK_PROV_H */
