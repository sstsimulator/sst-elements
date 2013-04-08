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

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>
#include <ptl_internal_netIf.h>
int PtlGetUid(ptl_handle_ni_t   ni_handle,
              ptl_uid_t*        uid)
{

    return PTL_FAIL;
}

int PtlGetId(ptl_handle_ni_t    ni_handle,
             ptl_process_t*     id)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };

    struct PtlAPI* api = GetPtlAPI( ni );
    int retval = api->PtlGetId( api, id );

    if ( retval < 0 ) return -retval; 

    return retval;
}
