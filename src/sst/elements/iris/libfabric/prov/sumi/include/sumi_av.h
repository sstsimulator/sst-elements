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

#ifndef _SUMI_AV_H_
#define _SUMI_AV_H_

#include "sumi_prov.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sumi_av_addr_entry {
	struct sumi_address sumi_addr;
	struct {
		uint32_t name_type : 8;
		uint32_t cm_nic_cdm_id : 24;
		uint32_t cookie;
	};
	struct {
		uint32_t rx_ctx_cnt : 8;
		uint32_t key_offset: 12;
		uint32_t unused1 : 12;
	};
};

const char *sumi_av_straddr(struct fid_av *av,
			    const void *addr,
			    char *buf,
			    size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* _SUMI_AV_H_ */
