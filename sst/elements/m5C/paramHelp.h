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


#ifndef _paramHelp_h
#define _paramHelp_h

#include <debug.h>
#include <util.h>

namespace SST {
namespace M5 {

#define INIT_CLOCK( c, p, member )\
    c.member = freq_to_ticks( p.find_string(#member) );\
    F_INT(c,member)

#define INIT_LATENCY( c, p, member )\
    c.member = latency_to_ticks( p.find_string(#member) );\
    F_INT(c,member)

#define INIT_INT( c, p, member )\
    c.member = p.find_integer(#member);\
    F_INT(c,member)

#define INIT_HEX( c, p, member )\
    c.member = p.find_integer(#member);\
    F_HEX(c,member)

#define INIT_STR( c, p, member )\
    c.member = p.find_string(#member);\
    F_STR(c,member)

static inline bool find_bool( std::string val ) {
    if ( val.compare("true") == 0 )
        return true;
    else if ( val.compare("false") == 0 )
        return false;
    else if ( val.compare("") == 0 )
        return false;
    else
        assert(0);
}

#define INIT_BOOL( c, p, member )\
    c.member = find_bool( p.find_string(#member) );\
    F_BOOL(c,member)

#define F_INT( x, member )\
    DBGC(1,"%s.%s %d\n", x.name.c_str(), #member, x.member )

#define F_HEX( x, member )\
    DBGC(1,"%s.%s %#x\n", x.name.c_str(), #member, x.member )

#define F_STR( x, member )\
    DBGC(1,"%s.%s `%s`\n", x.name.c_str(), #member, x.member.c_str() )

#define F_BOOL( x, member )\
    DBGC(1,"%s.%s `%s`\n", x.name.c_str(), #member, x.member ? "true" : "false")

}
}

#endif
