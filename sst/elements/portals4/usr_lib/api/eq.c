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
#include <ptl_internal_debug.h>
#include <assert.h>
#include <stdlib.h>

int PtlEQAlloc(ptl_handle_ni_t      ni_handle,
               ptl_size_t           count,
               ptl_handle_eq_t *    eq_handle)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_internal_handle_converter_t eqh = ni;

    struct PtlAPI* api = GetPtlAPI( ni );

    eqh.s.selector = HANDLE_EQ_CODE;

    int retval = api->PtlEQAlloc( api, count );
    if ( retval < 0 )  return -retval;

    eqh.s.code = retval;

    *eq_handle = eqh.a;

    return PTL_OK;
}

int PtlEQFree(ptl_handle_eq_t eq_handle)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( eq_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t eq = { eq_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQFree( api, eq.s.code );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlEQGet(ptl_handle_eq_t    eq_handle,
             ptl_event_t *      event)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( eq_handle, &ni_handle );

    PTL_DBG("\n");

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t eq = { eq_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQGet( api, eq.s.code, event );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlEQWait(ptl_handle_eq_t   eq_handle,
              ptl_event_t *     event)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( eq_handle, &ni_handle );

    PTL_DBG("\n");

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t eq = { eq_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQWait( api, eq.s.code, event );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlEQPoll(const ptl_handle_eq_t *eq_handles,
              unsigned int          size,
              ptl_time_t            timeout,
              ptl_event_t *         event,
              unsigned int *        which)
{
    ptl_handle_ni_t ni_handle;
    int i;

    PTL_DBG("\n");

    if ( size <= 0 ) return PTL_ARG_INVALID;

    PtlNIHandle( eq_handles[0], &ni_handle );

    ptl_handle_eq_t* xx = (ptl_handle_eq_t*)
                        malloc( sizeof(ptl_handle_eq_t ) * size );

    for ( i = 0; i < size; i++ ) {
        ptl_handle_ni_t tmp;
        PtlNIHandle( eq_handles[i], &tmp );
        assert ( tmp == ni_handle );

        const ptl_internal_handle_converter_t eq = { eq_handles[i] };
        xx[i] = eq.s.code; 
    }

    const ptl_internal_handle_converter_t ni = { ni_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQPoll( api, xx, size, timeout, event, which );
    free( xx );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}
