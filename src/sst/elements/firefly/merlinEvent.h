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


#ifndef COMPONENTS_FIREFLY_MERLINEVENT_H
#define COMPONENTS_FIREFLY_MERLINEVENT_H

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
namespace Firefly {


#define FOO 0
class FireflyNetworkEvent : public Event {

  public:

    FireflyNetworkEvent( ) : offset(0), bufLen(0), m_isHdr(false), m_isTail(false), m_isCtrl(false), pktOverhead(0) {
        buf.reserve( 1000 );
        assert( 0 == buf.size() );
    }

    FireflyNetworkEvent( int pktOverhead, size_t reserve = 1000 ) : offset(0), bufLen(0),
            m_isHdr(false), m_isTail(false), m_isCtrl(false), pktOverhead(pktOverhead) {
        buf.reserve( reserve );
        assert( 0 == buf.size() );
    }

    void setCtrl() { m_isCtrl = true; }
    bool isCtrl() { return m_isCtrl; }
    void setHdr() { m_isHdr = true; }
    void clearHdr() { m_isHdr = false; }
    bool isHdr() { return m_isHdr; }
    void setTail() { m_isTail = true; }
    bool isTail() { return m_isTail; }
    int calcPayloadSizeInBits() { return payloadSize() * 8; }
    int payloadSize() { return pktOverhead + bufSize(); }
    void setSrcNode(int node ) { srcNode = node; }
    void setSrcPid( int pid ) { srcPid = pid; }
    void setDestPid( int pid ) { destPid = pid; }

    int getSrcNode() { return srcNode; }
    int getSrcPid() { return srcPid; }
    int getDestPid() { return destPid; }

    size_t bufSize() {
        return bufLen - offset;
    }

    bool bufEmpty() {
        return ( bufLen == offset );
    }
    void* bufPtr( size_t len = 0 ) {
        if ( offset + len < buf.size() ) {
            return &buf[offset + len];
        } else {
            return NULL;
        }
    }

    void bufPop( size_t len ) {

        offset += len;
        assert( offset <= bufLen );
    }

    void bufAppend( const void* ptr , size_t len ) {
        if ( ptr ) {
            buf.resize( bufLen + len);
            memcpy( &buf[bufLen], (const char*) ptr, len );
        }
        bufLen += len;
    }

    FireflyNetworkEvent(const FireflyNetworkEvent *me) :
        Event()
    {
        buf = me->buf;
        seq = me->seq;
        srcNode = me->srcNode;
        srcPid = me->srcPid;
        destPid = me->destPid;
        m_isHdr = me->m_isHdr;
        m_isTail = me->m_isTail;
        m_isCtrl = me->m_isCtrl;
        offset = me->offset;
        pktOverhead = me->pktOverhead;
    }

    FireflyNetworkEvent(const FireflyNetworkEvent &me) :
        Event()
    {
        buf = me.buf;
        seq = me.seq;
        srcNode = me.srcNode;
        srcPid = me.srcPid;
        destPid = me.destPid;
        m_isHdr = me.m_isHdr;
        m_isTail = me.m_isTail;
        m_isCtrl = me.m_isCtrl;
        offset = me.offset;
        pktOverhead = me.pktOverhead;
    }

    virtual Event* clone(void) override
    {
        return new FireflyNetworkEvent(*this);
    }

    // void setDest( int _dest ) {
    //     dest = _dest;
    // }

    // void setSrc( int _src ) {
    //     src = _src;
    // }
    // void setPktSize() {
    //     size_in_bits = buf.size() * 8;
    // }

  private:

    uint16_t        seq;
    int             srcNode;
    int             srcPid;
    int             destPid;
    bool            m_isHdr;
    bool            m_isTail;
    bool            m_isCtrl;
    int             pktOverhead;

    size_t          offset;
    size_t          bufLen;
    std::vector<unsigned char>     buf;

  public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & seq;
        ser & offset;
        ser & bufLen;
        ser & buf;
        ser & srcNode;
        ser & srcPid;
        ser & destPid;
        ser & pktOverhead;
        ser & m_isHdr;
        ser & m_isTail;
        ser & m_isCtrl;
    }

    ImplementSerializable(SST::Firefly::FireflyNetworkEvent);

};

}
}

#endif
