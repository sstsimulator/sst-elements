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

#ifndef _H_VANADIS_NODE_OS_FD
#define _H_VANADIS_NODE_OS_FD

#include <cstdint>
#include <cstdio>
#include <string>

#include <fcntl.h>

namespace SST {
namespace Vanadis {

class VanadisOSFileDescriptor {
public:
    VanadisOSFileDescriptor(const char* file_path) : path(file_path), fd(-1)
	{
        fd = open( file_path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU );
        if ( -1 == fd ) {
            throw errno;
        }
    }

    VanadisOSFileDescriptor(const char* file_path, int flags, mode_t mode) : path(file_path) 
    {
        fd = open(file_path, flags, mode );
        if ( -1 == fd ) {
            throw errno;
        }		
    }

    VanadisOSFileDescriptor(const char* file_path, int dirfd, int flags, mode_t mode) : path(file_path)
    {
        fd = openat(dirfd, file_path, flags, mode );
        if ( -1 == fd ) {
            throw errno;
        }		
    }

    ~VanadisOSFileDescriptor() { 
        close(fd);
    }

    const char* getPath() const {
        if (path.size() == 0) {
            return "";
        } else {
            return &path[0];
        }
    }

    int getFileDescriptor() {
        return fd;
    }

protected:
    std::string path;
    int fd;
};

} // namespace Vanadis
} // namespace SST

#endif
