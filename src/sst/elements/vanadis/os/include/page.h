// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_PAGE
#define _H_VANADIS_NODE_OS_INCLUDE_PAGE

#include <string>
#include <set>

#include "os/vphysmemmanager.h"

#if 0 
#define PageDbg( format, ... ) printf( "Page::%s() " format, __func__, ##__VA_ARGS__ )
#else
#define PageDbg( format, ... )
#endif


namespace SST {
namespace Vanadis {

namespace OS {

class Page {
  public:
    Page( PhysMemManager* mem ) : mem(mem), refCnt(1) {
        ppn = mem->allocPage( PhysMemManager::PageSize::FourKB ); 
        PageDbg("ppn=%d\n",ppn);
    }
    ~Page() {
        PageDbg("ppn=%d\n",ppn);
        assert( 0 == refCnt );
        mem->freePage( PhysMemManager::PageSize::FourKB, ppn );
    }

    unsigned getRefCnt() {
        PageDbg("ppn=%d refCnt=%d\n",ppn,refCnt);
        return refCnt;
    }
    unsigned getPPN() {
        return ppn;
    }

    void incRefCnt() { 
        ++refCnt; 
        PageDbg("ppn=%d refCnt=%d\n",ppn,refCnt);
    }
    unsigned decRefCnt() {
        assert( refCnt > 0 );
        --refCnt;
        PageDbg("ppn=%d refCnt=%d\n",ppn,refCnt);
        return refCnt;
    }

  private:
    PhysMemManager* mem;
    unsigned refCnt; 
    unsigned ppn;
};

}
}
}

#endif
