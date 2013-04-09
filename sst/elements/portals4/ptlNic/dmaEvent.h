// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _dmaEvent_h
#define _dmaEvent_h

#include <sst/core/event.h>

#include "ptlNicTypes.h"

namespace SST {
namespace Portals4 {

class DmaEvent : public SST::Event {

  public:
    enum Type { Read, Write };

    DmaEvent( Type _type, Addr _addr, uint8_t* _buf,
                size_t _size, void* _key ) :
        type( _type ),
        addr( _addr ),
        buf( (uint64_t) _buf ),
        size( _size ), // this is a hack, used by PtlNicMMIF
        key( _key )
    { 
    }
    Type        type; 
    Addr        addr;     
    uint64_t    buf;
    size_t      size;
    size_t      _size;
    void*       key;

  private:

    ~DmaEvent() {}
    DmaEvent() {}

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {   
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(type);
        ar & BOOST_SERIALIZATION_NVP(addr);
        ar & BOOST_SERIALIZATION_NVP(buf);
        ar & BOOST_SERIALIZATION_NVP(size);
    }
};

}
}

#endif
