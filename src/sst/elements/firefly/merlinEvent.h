// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
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
    uint16_t        seq;
    int             src;

    FireflyNetworkEvent( size_t reserve = 1000 ) : offset(0), bufLen(0) {
        buf.reserve( reserve );
        assert( 0 == buf.size() );
    }

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
        src = me->src;
    }

    FireflyNetworkEvent(const FireflyNetworkEvent &me) :
        Event()
    {
        buf = me.buf;
        seq = me.seq;
        src = me.src;
    }

    virtual Event* clone(void)
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

    std::vector<unsigned char>     buf;
    size_t          offset;
    size_t          bufLen;

//    friend class boost::serialization::access;
//    template<class Archive>
//    void
//    serialize(Archive & ar, const unsigned int version )
//    {
//        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
//        ar & BOOST_SERIALIZATION_NVP(seq);
//        ar & BOOST_SERIALIZATION_NVP(bufLen);
//        ar & BOOST_SERIALIZATION_NVP(buf);
//        ar & BOOST_SERIALIZATION_NVP(src);
//    }
    
  public:	
    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
        ser & seq;
        ser & bufLen;
        ser & buf;
        ser & src;
    }
    
    ImplementSerializable(SST::Firefly::FireflyNetworkEvent);     
    
};

}
}

#endif
