// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_OPENAT
#define _H_VANADIS_SYSCALL_OPENAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallOpenAtEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallOpenAtEvent() : VanadisSyscallEvent() {}
	VanadisSyscallOpenAtEvent( uint32_t core, uint32_t thr, uint64_t dirfd, uint64_t path_ptr, uint64_t flags ) :
		VanadisSyscallEvent(core, thr),
		openat_dirfd( dirfd ), openat_path_ptr( path_ptr ), openat_flags( flags ) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_OPENAT;
	}

	int64_t getDirectoryFileDescriptor() const { return openat_dirfd; }
	uint64_t getPathPointer() const { return openat_path_ptr; }
	int64_t getFlags() const { return openat_flags; }

private:
	int64_t   openat_dirfd;
	uint64_t  openat_path_ptr;
	int64_t   openat_flags;

};

}
}

#endif
