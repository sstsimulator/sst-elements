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

#ifndef _util_h
#define _util_h

#include <cstdio>
#include <dll/gem5dll.hh>

namespace SST {
namespace M5 {

class M5;

typedef std::map< std::string, Gem5Object_t* > objectMap_t;

extern objectMap_t buildConfig( SST::M5::M5*, std::string, std::string, SST::Params& );

extern unsigned freq_to_ticks( std::string val );
extern unsigned latency_to_ticks( std::string val );

}
}

#endif
