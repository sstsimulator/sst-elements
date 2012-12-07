// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMTYPES_H
#define _MEMTYPES_H

namespace SST {
namespace MemHierarchy {
	// TODO:  Change 32/64 as needed
	typedef uint64_t Addr;

	typedef enum {
		ReadReq = 0,
		ReadResp,
		WriteReq,
		WriteResp,
		Invalidate,
		Fetch,
		Fetch_Invalidate,
		FetchResp,
		WriteBack,

		// Bus Control
		RequestBus,
		CancelBusRequest,
		BusClearToSend,

		ACK,
		NAK,
		NULLCMD,   // Must be last.
	} Command;

};
};

#endif /* _MEMTYPES_H */
