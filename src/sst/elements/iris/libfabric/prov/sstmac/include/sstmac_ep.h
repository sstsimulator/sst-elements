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

#ifndef _SSTMAC_EP_H_
#define _SSTMAC_EP_H_

#include "sstmac.h"

#ifdef __cplusplus
extern "C" {
#endif

int sstmac_pep_open(struct fid_fabric *fabric,
		  struct fi_info *info, struct fid_pep **pep,
		  void *context);

int sstmac_scalable_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

int sstmac_pep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

ssize_t sstmac_cancel(fid_t fid, void *context);

int sstmac_getopt(fid_t fid, int level, int optname,
                   void *optval, size_t *optlen);

int sstmac_setopt(fid_t fid, int level, int optname,
                   const void *optval, size_t optlen);

DIRECT_FN int sstmac_ep_atomic_valid(struct fid_ep *ep,
				   enum fi_datatype datatype,
				   enum fi_op op, size_t *count);

DIRECT_FN int sstmac_ep_fetch_atomic_valid(struct fid_ep *ep,
					 enum fi_datatype datatype,
					 enum fi_op op, size_t *count);

DIRECT_FN int sstmac_ep_cmp_atomic_valid(struct fid_ep *ep,
				       enum fi_datatype datatype,
				       enum fi_op op, size_t *count);

#ifdef __cplusplus
}
#endif

#endif
