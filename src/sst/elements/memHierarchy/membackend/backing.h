// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __SST_MEMH_BACKEND_BACKING
#define __SST_MEMH_BACKEND_BACKING

#include <fcntl.h>
#include <unistd.h>
#include "sst/elements/memHierarchy/util.h"

namespace SST {
namespace MemHierarchy {
namespace Backend {

class Backing {
public:
    Backing( ) { }
    ~Backing() { }

    virtual void set( Addr addr, uint8_t value ) = 0;
    virtual uint8_t get( Addr addr) = 0;
};

class BackingMMAP : public Backing {
public:
    BackingMMAP(std::string memoryFile, size_t size, size_t offset = 0) : Backing(), m_fd(-1), m_size(size), m_offset(offset) {
        int flags = MAP_PRIVATE;
        if ( ! memoryFile.empty() ) {
            m_fd = open(memoryFile.c_str(), O_RDWR);
            if ( m_fd < 0) {
                throw 1;
            }
        } else {
            flags  |= MAP_ANON;
        }
        m_buffer = (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, flags, m_fd, 0);

        if ( m_buffer == MAP_FAILED) {
            throw 2;
        }
    }

    ~BackingMMAP() {
        munmap( m_buffer, m_size );
        if ( -1 != m_fd ) {
            close( m_fd );
        }
    }

    void set( Addr addr, uint8_t value ) {
        m_buffer[addr - m_offset ] = value;
    }
    
    uint8_t get( Addr addr ) {
        return m_buffer[addr - m_offset];
    }
private:
    uint8_t* m_buffer;
    int m_fd;
    int m_size;
    size_t m_offset;
};

class BackingMalloc : public Backing {
public:
    BackingMalloc(size_t size) {
        m_buffer.reserve(size); /* Guess what our size will be */
    }

    void set( Addr addr, uint8_t value ) {
        m_buffer[addr] = value;
    }

    uint8_t get( Addr addr ) {
        if (m_buffer.find(addr) == m_buffer.end())
            m_buffer[addr] = 0;
        return m_buffer[addr];
    }
private:
    std::unordered_map<Addr,uint8_t> m_buffer;
};

}
}
}

#endif
