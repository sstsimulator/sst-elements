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
#include <stdlib.h>
#include <signal.h>
#include "sumi_prov.h"
#include "sumi_wait.h"
#include <mercury/common/errors.h>
#include <vector>

static int sumi_wait_control(struct fid *wait, int command, void *arg);
extern "C" int sumi_wait_close(struct fid *wait);
extern "C" DIRECT_FN  int sumi_wait_wait(struct fid_wait *wait, int timeout);
extern "C" DIRECT_FN  int sumi_wait_open(struct fid_fabric *fabric,
			     struct fi_wait_attr *attr,
			     struct fid_wait **waitset);

static struct fi_ops sumi_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = sumi_wait_close,
	.bind = fi_no_bind,
	.control = sumi_wait_control,
	.ops_open = fi_no_ops_open
};

static struct fi_ops_wait sumi_wait_ops = {
	.size = sizeof(struct fi_ops_wait),
	.wait = sumi_wait_wait
};

struct sumi_fid_wait_obj {
};

struct sumi_fid_wait_set {
  struct fid_wait wait;
  struct fid_fabric *fabric;
  enum fi_cq_wait_cond cond_type;
  enum fi_wait_obj type;
  std::vector<fid*> listeners;
};

static int sumi_wait_control(struct fid *wait, int command, void *arg)
{
/*
	struct fid_wait *wait_fid_priv;

	SUMI_TRACE(WAIT_SUB, "\n");

	wait_fid_priv = container_of(wait, struct fid_wait, fid);
*/

	switch (command) {
	case FI_GETWAIT:
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
}


extern "C" void sstmaci_add_wait(struct fid_wait *wait, struct fid *wait_obj)
{
  sumi_fid_wait_set* waitset = (sumi_fid_wait_set*) wait;
  waitset->listeners.push_back(wait_obj);
}


extern "C" DIRECT_FN  int sumi_wait_wait(struct fid_wait *wait, int timeout)
{
	int err = 0, ret;
	char c;
  sst_hg_abort_printf("unimplemented: sumi_wait_wait for wait set");
  return FI_SUCCESS;
}

extern "C" int sumi_wait_close(struct fid *wait)
{
  free(wait);
	return FI_SUCCESS;
}

DIRECT_FN int sumi_wait_open(struct fid_fabric *fabric,
			     struct fi_wait_attr *attr,
			     struct fid_wait **waitset)
{
  struct sumi_fid_wait_set* waiter = (struct sumi_fid_wait_set*) calloc(1,sizeof(struct sumi_fid_wait_set));
  waiter->wait.fid.fclass = FI_CLASS_WAIT;
  waiter->wait.fid.ops = &sumi_fi_ops;
  waiter->fabric = fabric;
  *waitset = (fid_wait*) waiter;

  return FI_SUCCESS;
}

