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

int PtlTriggeredPut(ptl_handle_md_t     md_handle,
                    ptl_size_t          local_offset,
                    ptl_size_t          length,
                    ptl_ack_req_t       ack_req,
                    ptl_process_t       target_id,
                    ptl_pt_index_t      pt_index,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_handle_ct_t     trig_ct_handle,
                    ptl_size_t          threshold)
{
    return PTL_FAIL;
}

int PtlTriggeredGet(ptl_handle_md_t     md_handle,
                    ptl_size_t          local_offset,
                    ptl_size_t          length,
                    ptl_process_t       target_id,
                    ptl_pt_index_t      pt_index,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_handle_ct_t     trig_ct_handle,
                    ptl_size_t          threshold)
{
   ptl_handle_ni_t ni_handle;
    PtlNIHandle( md_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t md = { md_handle };
    const ptl_internal_handle_converter_t ct = { trig_ct_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int _ct_handle = trig_ct_handle == PTL_CT_NONE ? -1 : ct.s.code;

    int retval = api->PtlTrigGet( api, md.s.code,
            local_offset,
            length,
            target_id,
            pt_index,
            match_bits,
            remote_offset,
            user_ptr,
            _ct_handle,
            threshold);

    if ( retval < 0 ) return -retval;

    return PTL_OK;
}

int PtlTriggeredAtomic(ptl_handle_md_t  md_handle,
                       ptl_size_t       local_offset,
                       ptl_size_t       length,
                       ptl_ack_req_t    ack_req,
                       ptl_process_t    target_id,
                       ptl_pt_index_t   pt_index,
                       ptl_match_bits_t match_bits,
                       ptl_size_t       remote_offset,
                       void *           user_ptr,
                       ptl_hdr_data_t   hdr_data,
                       ptl_op_t         operation,
                       ptl_datatype_t   datatype,
                       ptl_handle_ct_t  trig_ct_handle,
                       ptl_size_t       threshold)
{
    return PTL_FAIL;
}

int PtlTriggeredFetchAtomic(ptl_handle_md_t     get_md_handle,
                            ptl_size_t          local_get_offset,
                            ptl_handle_md_t     put_md_handle,
                            ptl_size_t          local_put_offset,
                            ptl_size_t          length,
                            ptl_process_t       target_id,
                            ptl_pt_index_t      pt_index,
                            ptl_match_bits_t    match_bits,
                            ptl_size_t          remote_offset,
                            void *              user_ptr,
                            ptl_hdr_data_t      hdr_data,
                            ptl_op_t            operation,
                            ptl_datatype_t      datatype,
                            ptl_handle_ct_t     trig_ct_handle,
                            ptl_size_t          threshold)
{
    return PTL_FAIL;
}

int PtlTriggeredSwap(ptl_handle_md_t    get_md_handle,
                     ptl_size_t         local_get_offset,
                     ptl_handle_md_t    put_md_handle,
                     ptl_size_t         local_put_offset,
                     ptl_size_t         length,
                     ptl_process_t      target_id,
                     ptl_pt_index_t     pt_index,
                     ptl_match_bits_t   match_bits,
                     ptl_size_t         remote_offset,
                     void *             user_ptr,
                     ptl_hdr_data_t     hdr_data,
                     const void *       operand,
                     ptl_op_t           operation,
                     ptl_datatype_t     datatype,
                     ptl_handle_ct_t    trig_ct_handle,
                     ptl_size_t         threshold)
{
    return PTL_FAIL;
}

int PtlTriggeredCTInc(ptl_handle_ct_t   ct_handle,
                      ptl_ct_event_t    increment,
                      ptl_handle_ct_t   trig_ct_handle,
                      ptl_size_t        threshold)
{
    return PTL_FAIL;
}

int PtlTriggeredCTSet(ptl_handle_ct_t   ct_handle,
                      ptl_ct_event_t    new_ct,
                      ptl_handle_ct_t   trig_ct_handle,
                      ptl_size_t        threshold)
{
    return PTL_FAIL;
}
