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


#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,NIC_DBG_RECV_MACHINE,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE, "%#03x\n",(unsigned char)buf[i]);
    }
}

bool Nic::EntryBase::copyIn( Output& dbg,
                        FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,
                "ioVec.size()=%lu\n", ioVec().size() );


    for ( ; currentVec() < ioVec().size();
                currentVec()++, currentPos() = 0 ) {

        if ( ioVec()[currentVec()].len ) {
            size_t toLen = ioVec()[currentVec()].len - currentPos();
            size_t fromLen = event.bufSize();
            size_t len = toLen < fromLen ? toLen : fromLen;

            DmaVec tmp;
            tmp.length = len;
            tmp.addr = ioVec()[currentVec()].addr.getSimVAddr() +
                                                    currentPos();
            vec.push_back( tmp );

            char* toPtr = (char*) ioVec()[currentVec()].addr.getBacking() +
                                                        currentPos();
            dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,
                            "toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            currentLen() += len;
            if ( ioVec()[currentVec()].addr.getBacking() ) {
                void *buf = event.bufPtr();
                if ( buf ) {
                    memcpy( toPtr, buf, len );
                }
                print( dbg, toPtr, len );
            }

            event.bufPop(len);

            currentPos() += len;
            if ( event.bufEmpty() &&
                    currentPos() != ioVec()[currentVec()].len )
            {
                dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,
                            "event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,
                "currentVec=%lu, currentPos=%lu\n",
                currentVec(), currentPos());
    return ( currentVec() == ioVec().size() ) ;
}

void Nic::EntryBase::copyOut( Output& dbg, int vc, int numBytes,
                FireflyNetworkEvent& event,
                std::vector<DmaVec>& vec  )
{
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Send: "
                    "%d: ioVec.size()=%lu\n", vc, ioVec().size() );

    for ( ; currentVec() < ioVec().size() &&
                event.bufSize() <  numBytes;
                currentVec()++, currentPos() = 0 ) {

        dbg.verbose(CALL_INFO,3,1,"vec[%lu].len %lu\n",currentVec(),
                    ioVec()[currentVec()].len );

        if ( ioVec()[currentVec()].len ) {
            size_t toLen = numBytes - event.bufSize();
            size_t fromLen = ioVec()[currentVec()].len - currentPos();

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,3,1,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from =
                    (const char*) ioVec()[currentVec()].addr.getBacking() + currentPos();
            DmaVec tmp;
            tmp.addr = ioVec()[currentVec()].addr.getSimVAddr() + currentPos();
            tmp.length = len;
            vec.push_back(tmp);

            if ( ioVec()[currentVec()].addr.getBacking()) {
                event.bufAppend( from, len );
            } else {
                event.bufAppend( NULL, len );
            }

            currentPos() += len;
            if ( event.bufSize() == numBytes && currentPos() != ioVec()[currentVec()].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,3,1,"currentVec()=%lu, currentPos()=%lu\n", currentVec(), currentPos());
}
