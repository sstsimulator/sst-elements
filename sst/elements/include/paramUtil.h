// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef _PARAMUTIL_H
#define _PARAMUTIL_H

#warning paramUtil.h has been deprecated.  Please use equivalent functionality contained in the Params class

#include <errno.h>
#include <sst/core/component.h>

inline long str2long( std::string str )
{
    long value = -1;
    value = strtol( str.c_str(), NULL, 0 );
    if ( errno == EINVAL ) {
        _abort(XbarV2,"strtol( %s, NULL, 0: %ld\n", str.c_str(),value);
    }
    return value;
}
#endif
