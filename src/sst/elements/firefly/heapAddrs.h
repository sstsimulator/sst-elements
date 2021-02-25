// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_COMPONENTS_FIREFLY_HEAP_ADDRS
#define _H_COMPONENTS_FIREFLY_HEAP_ADDRS

#include <map>
#include <queue>
#include <string>

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#define _H_HEAP_ADDRS_DBG 0

namespace SST {
namespace Firefly {

class HeapAddrs {

  public:

	~HeapAddrs() { }

	HeapAddrs( uint64_t start, size_t length ): m_curAddr(start), m_endAddr(start + length)  {
#if _H_HEAP_ADDRS_DBG
		printf("HeapAddrs::%s() 0x%" PRIx64 " %zu\n",__func__,start,length);
#endif
	}

	uint64_t alloc( size_t length ) {

		uint64_t addr;
		length = roundUp(length);

		if ( m_buckets.find(length) == m_buckets.end() || m_buckets[length].empty() ) {
			assert( m_curAddr + length <  m_endAddr );
			addr = m_curAddr;
			m_curAddr += length;
		} else {
			addr = m_buckets[length].front();
			m_buckets[length].pop();
		}
		m_used[addr] = length;
#if _H_HEAP_ADDRS_DBG
		printf("HeapAddrs::%s() addr=0x%" PRIx64 " length=%zu\n",__func__, addr, length );
#endif
		return addr;
	}

	void free( uint64_t addr ) {
		assert( m_used.find( addr ) != m_used.end() );

		size_t bucket = m_used[addr];
#if _H_HEAP_ADDRS_DBG
		printf("HeapAddrs::%s() addr=0x%" PRIx64 " length=%zu\n",__func__, addr, bucket );
#endif
		m_buckets[bucket].push(addr);
		m_used.erase(addr);
	}

  private:

	size_t roundUp( size_t length ) {
		if ( length % 16 ) {
			return (length + 16) & ~15;
		} else {
			return length;
		}
	}

	std::map<size_t, std::queue<uint64_t> > m_buckets;
	std::map<uint64_t, size_t> m_used;

	uint64_t m_curAddr;
	uint64_t m_endAddr;
};
}
}

#endif
