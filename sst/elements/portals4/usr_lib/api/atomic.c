/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>

int PtlAtomic(ptl_handle_md_t   md_handle,
              ptl_size_t        local_offset,
              ptl_size_t        length,
              ptl_ack_req_t     ack_req,
              ptl_process_t     target_id,
              ptl_pt_index_t    pt_index,
              ptl_match_bits_t  match_bits,
              ptl_size_t        remote_offset,
              void *            user_ptr,
              ptl_hdr_data_t    hdr_data,
              ptl_op_t          operation,
              ptl_datatype_t    datatype)
{
    return PTL_FAIL;
}

int PtlFetchAtomic(ptl_handle_md_t  get_md_handle,
                   ptl_size_t       local_get_offset,
                   ptl_handle_md_t  put_md_handle,
                   ptl_size_t       local_put_offset,
                   ptl_size_t       length,
                   ptl_process_t    target_id,
                   ptl_pt_index_t   pt_index,
                   ptl_match_bits_t match_bits,
                   ptl_size_t       remote_offset,
                   void *           user_ptr,
                   ptl_hdr_data_t   hdr_data,
                   ptl_op_t         operation,
                   ptl_datatype_t   datatype)
{
    return PTL_FAIL;
}

int PtlSwap(ptl_handle_md_t     get_md_handle,
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
            const void *              operand,
            ptl_op_t            operation,
            ptl_datatype_t      datatype)
{
    return PTL_FAIL;
}

