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

#include <debug.h>
#include <dll/gem5dll.hh>

namespace SST {
namespace M5 {

bool SST_M5_debug = false;
Log<1> _dbg( std::cerr, "M5:", false );
Log<1> _info( std::cout, "M5:", false );


void enableDebug( std::string name )
{
    bool all = false;
    if ( name.find( "All") != std::string::npos) {
        all = true;
    }

    if ( all || name.find( "SST") != std::string::npos) {
        SST_M5_debug = true;
    }
    libgem5::EnableDebugFlags( name );
}

}
}
