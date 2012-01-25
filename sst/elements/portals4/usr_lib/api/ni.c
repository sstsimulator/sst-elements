/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#include <stdio.h>
#include <stdlib.h>
#include <portals4.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include "ptl_internal_handles.h"
#include "ptl_internal_netIf.h"
#include "ptl_internal_debug.h"

const ptl_interface_t  PTL_IFACE_DEFAULT = "PtlNic";

const ptl_handle_any_t PTL_INVALID_HANDLE = { UINT_MAX };

#define NUM_IFACE ( 1 << HANDLE_IFACE_BITS )

NetIFEntry ifTable[ NUM_IFACE ];

void ptl_internal_register_netIF( const char* const name, struct NetIF* netIF )
{
    int i;
    assert( strlen( name ) < NET_IF_NAME_LEN );

    //PTL_DBG("name=`%s`\n",name);

    for ( i = 0; i < NUM_IFACE; i++ ) {
        assert ( strncmp( ifTable[i].name, name, NET_IF_NAME_LEN ) );
    }
    for ( i = 0; i < NUM_IFACE; i++ ) {
        if ( strlen( ifTable[i].name ) == 0 ) {
            strncpy( ifTable[i].name, name, NET_IF_NAME_LEN ); 
            ifTable[i].netIF = netIF;
            return;
        }
    }
    abort();
}

static int lookup_iface( const char* const name )
{
    int i;
    //PTL_DBG("name=`%s`\n",name);
    for ( i = 0; i < NUM_IFACE; i++ ) {
        if ( strncmp( ifTable[i].name, name, NET_IF_NAME_LEN ) == 0 ) {
            return i;
        }
    }
    return -1; 
}

int PtlNIInit(ptl_interface_t   ifaceName,
              unsigned int      options,
              ptl_pid_t         pid,
              ptl_ni_limits_t   *desired,
              ptl_ni_limits_t   *actual,
              ptl_size_t        map_size,
              ptl_process_t     *desired_mapping,
              ptl_process_t     *actual_mapping,
              ptl_handle_ni_t   *ni_handle)
{
    ptl_internal_handle_converter_t ni = { .s.selector = HANDLE_NI_CODE };

    switch (options) {
        case (PTL_NI_MATCHING | PTL_NI_LOGICAL):
            ni.s.ni = 0;
            break;
        case PTL_NI_NO_MATCHING | PTL_NI_LOGICAL:
            ni.s.ni = 1;
            break;
        case (PTL_NI_MATCHING | PTL_NI_PHYSICAL):
            ni.s.ni = 2;
            break;
        case PTL_NI_NO_MATCHING | PTL_NI_PHYSICAL:
            ni.s.ni = 3;
            break;
        default:
            return PTL_ARG_INVALID;
    }   

    PTL_DBG( "ifaceName=`%s` ni=%d\n", ifaceName,ni.s.ni);

    ni.s.iface = lookup_iface( ifaceName ); 

    if ( ni.s.iface == -1 ) {
        return PTL_ARG_INVALID;
    }

    NetIFEntry* iface = &ifTable[ni.s.iface]; 
    assert( iface );
    
    if ( ! iface->ptlIF ) {
        assert( iface->netIF );
        iface->ptlIF = iface->netIF->init();
        assert( iface->ptlIF );
    }

    // don't allow the same NI to be opened more than once
    assert ( ! iface->ptlAPI[ni.s.ni] );

    iface->ptlAPI[ni.s.ni] = iface->ptlIF->init( iface->ptlIF );
    assert( iface->ptlAPI[ni.s.ni] );

    *ni_handle = ni.a; 
    int retval = iface->ptlAPI[ni.s.ni]->PtlNIInit( 
                        iface->ptlAPI[ni.s.ni],
                        options, pid, desired, actual, 
                        map_size, desired_mapping, actual_mapping ); 
    if ( retval < 0 ) return -retval;

    return PTL_OK;
}

int PtlNIFini(ptl_handle_ni_t ni_handle)  
{
    const ptl_internal_handle_converter_t ni = { ni_handle };

    PTL_DBG("\n");

    NetIFEntry* iface = &ifTable[ni.s.iface]; 
    int retval = iface->ptlAPI[ni.s.ni]->PtlNIFini( iface->ptlAPI[ni.s.ni] ); 

    iface->ptlIF->fini( iface->ptlAPI[ni.s.ni] );
    iface->ptlAPI[ni.s.ni] = NULL;

    int i;
    for ( i = 0; i < NUM_PTL_NI; i++ ) {
        if ( iface->ptlAPI[ni.s.ni] ) {
            break;
        }  
    }
    if ( i == NUM_PTL_NI ) {
        iface->netIF->fini( iface->ptlIF );
        iface->netIF = NULL;
    }
    return retval;
}

int PtlNIHandle(ptl_handle_any_t    handle,
                ptl_handle_ni_t*    ni_handle)
{
    ptl_internal_handle_converter_t ehandle;

    PTL_DBG("handle=%#x ni_handle=%p\n",handle,ni_handle);
    ehandle.a = handle;

    switch (ehandle.s.selector) {
        case HANDLE_NI_CODE:
            *ni_handle = ehandle.i;
            break;
        case HANDLE_EQ_CODE:
        case HANDLE_CT_CODE:
        case HANDLE_MD_CODE:
        case HANDLE_LE_CODE:
        case HANDLE_ME_CODE:   
            ehandle.s.code     = 0;
            ehandle.s.selector = HANDLE_NI_CODE;
            ehandle.s.iface = ehandle.s.iface;
            *ni_handle         = ehandle.i;
            break;
        default:
            return PTL_ARG_INVALID;
    }
    return PTL_OK;
}
