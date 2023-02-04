// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <math.h>
#include "os/vloadpage.h"

namespace SST {
namespace Vanadis {

void loadPages( SST::Output* output, SST::Interfaces::StandardMem* mem_if, MMU_Lib::MMU* mmu,
            PhysMemManager* memMgr, unsigned pid, uint64_t virtAddr, std::vector<uint8_t>& buffer, uint64_t flags, int page_size )
{
    int shift = log2( (double) page_size );
    assert( 0 == (virtAddr & (page_size-1) ) );
    assert( 0 == buffer.size() % page_size );

    uint64_t pageVirtAddr = virtAddr; 
    int numPages = buffer.size() / page_size;
    for ( int i = 0; i < numPages ; i++ ) {
        uint64_t physAddr;
        int physPageNum;
        if ( mmu ) {
            try {
                physPageNum = memMgr->allocPage( PhysMemManager::PageSize::FourKB );
            } catch ( int err ) {
                output->fatal(CALL_INFO, -1, "Error: ran out of physical memory\n");
            }
            mmu->map( pid, pageVirtAddr >> shift, physPageNum, page_size, flags );
            physAddr = physPageNum << shift;
        } else {
            physAddr = pageVirtAddr;
            physPageNum = pageVirtAddr >> shift;
        }

#if 0 
        // given sendUntimedData() only accepts a vector<uint8_t> we need to copy data into a properly sized vector 
        std::vector< uint8_t > pageBuffer( buffer.begin() + offset, buffer.begin() + offset + page_size );
#endif

        printf( "pageVirtAddr=%#" PRIx64 " physPageNum=%d physAddr=%#" PRIx64 "\n", pageVirtAddr, physPageNum, physAddr );
        output->verbose( CALL_INFO, 2, 0, "pageVirtAddr=%#" PRIx64 " physPageNum=%d physAddr=%#" PRIx64 "\n", pageVirtAddr, physPageNum, physAddr );

#if 1
        uint64_t offset = 0;
        for ( int num = 0; num < page_size/64; num++ ) {
            std::vector< uint8_t > tmp( buffer.begin() + offset, buffer.begin() + offset + 64  );
            Interfaces::StandardMem::Request* req = new SST::Interfaces::StandardMem::Write( physAddr + offset, tmp.size(), tmp );
            printf("%s() %s\n",__func__,req->getString().c_str());
            offset += 64;
            mem_if->send(req);
        }

#else
// This won't work for applications started after init
        mem_if->sendUntimedData(new SST::Interfaces::StandardMem::Write( physAddr, page_size, pageBuffer ) );
#endif
        pageVirtAddr += page_size;
        offset += page_size; 
    }
}


}
}
