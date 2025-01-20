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

#include "sumi_prov.h"
#include "sumi_poll.h"


extern "C" DIRECT_FN  int sumi_poll_open(struct fid_domain *domain,
			     struct fi_poll_attr *attr,
			     struct fid_poll **pollset)
{
	return -FI_ENOSYS;
}


extern "C" DIRECT_FN  int sumi_poll_poll(struct fid_poll *pollset, void **context, int count)
{
	return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sumi_poll_add(struct fid_poll *pollset, struct fid *event_fid,
			    uint64_t flags)
{
	return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sumi_poll_del(struct fid_poll *pollset, struct fid *event_fid,
			    uint64_t flags)
{
	return -FI_ENOSYS;
}
