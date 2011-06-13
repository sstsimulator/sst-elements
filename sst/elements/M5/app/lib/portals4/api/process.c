/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>
#include <ptl_internal_netIf.h>
int PtlGetUid(ptl_handle_ni_t   ni_handle,
              ptl_uid_t*        uid)
{

    return PTL_OK;
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

int PtlGetJid(ptl_handle_ni_t   ni_handle,
             ptl_jid_t*         jid)
{
    return PTL_OK;
}
