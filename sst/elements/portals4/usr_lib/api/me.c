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

int PtlMEAppend(ptl_handle_ni_t     ni_handle,
                ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void*               user_ptr,
                ptl_handle_me_t*    me_handle)
{
    const ptl_internal_handle_converter_t ct = { me->ct_handle };

    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_internal_handle_converter_t meh = ni;
    PTL_DBG("\n");

    struct PtlAPI* api = GetPtlAPI( ni );

    meh.s.selector = HANDLE_MD_CODE;

    ptl_me_t tmp_me = *me;
    tmp_me.ct_handle = me->ct_handle == PTL_CT_NONE ? -1 : ct.s.code;

    int retval = api->PtlMEAppend( api, pt_index, &tmp_me, ptl_list, user_ptr );
    if ( retval < 0 )  return -retval;

    meh.s.code = retval;

    *me_handle = meh.a;

    return PTL_OK;
}

int PtlMEUnlink(ptl_handle_me_t me_handle)
{
    PTL_DBG("me_handle=%#x\n",me_handle);
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( me_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t me = { me_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlMEUnlink( api, me.s.code );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlMESearch(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index,
                const ptl_me_t       *me,
                ptl_search_op_t ptl_search_op,
                void           *user_ptr)
{
    return PTL_FAIL;
}

