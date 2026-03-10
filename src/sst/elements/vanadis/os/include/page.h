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
    Page( PhysMemManager* mem ) : mem_(mem), ref_cnt_(1) {
        ppn_ = mem_->allocPage( PhysMemManager::PageSize::FourKB );
        PageDbg("ppn=%" PRIu32 "\n",ppn_);
    }

    Page( PhysMemManager* mem, uint32_t ppn, uint32_t ref_cnt ) : mem_(mem), ppn_(ppn), ref_cnt_(ref_cnt) {
    }

    ~Page() {
        PageDbg("ppn=%" PRIu32 "\n",ppn_);
        assert( 0 == ref_cnt_ );
        mem_->freePage( PhysMemManager::PageSize::FourKB, ppn_ );
    }

    uint32_t getRefCnt() {
        PageDbg("ppn=%" PRIu32 " ref_cnt=%" PRIu32 "\n",ppn,ref_cnt_);
        return ref_cnt_;
    }
    uint32_t getPPN() {
        return ppn_;
    }

    void incRefCnt() {
        ++ref_cnt_;
        PageDbg("ppn=%" PRIu32 " ref_cnt=%" PRIu32 "\n",ppn_,ref_cnt_);
    }
    uint32_t decRefCnt() {
        assert( ref_cnt_ > 0 );
        --ref_cnt_;
        PageDbg("ppn=%" PRIu32 " ref_cnt=%" PRIu32 "\n",ppn_,ref_cnt_);
        return ref_cnt_;
    }

    std::string snapshot() {
        std::stringstream ss;
        ss << "ppn: " << ppn_;
        ss << ", ref_cnt: " << ref_cnt_;
        return ss.str();
    }

  private:
    PhysMemManager* mem_;   /* Pointer to page table manager */
    uint32_t ref_cnt_;      /* Reference count per page */
    uint32_t ppn_;          /* Physical page number (index) */
};

}
}
}

#endif
