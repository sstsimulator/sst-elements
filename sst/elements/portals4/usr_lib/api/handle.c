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

#include <portals4.h>
#include <ptl_internal_debug.h>

int PtlHandleIsEqual(ptl_handle_any_t handle1,
                              ptl_handle_any_t handle2)
{
    PTL_DBG( "%lx %lx\n", handle1, handle2 );
    if ((uint32_t)handle1 == (uint32_t)handle2) {
        return PTL_OK;
    } else {
        return PTL_FAIL;
    }
}
