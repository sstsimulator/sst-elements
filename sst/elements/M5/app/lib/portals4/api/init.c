/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#include <stdlib.h>

#include <portals4.h>
#include <ptl_internal_commpad.h>
#include <ptl_internal_debug.h>

int _ptl_dbg_rank = -1;

static __attribute__ ((constructor)) void init(void)
{
    char* tmp = getenv("PTL_DBG_ID");
    if ( tmp ) {
        _ptl_dbg_rank = atoi( tmp );
    }
}

size_t proc_number;
size_t num_siblings;

int PtlInit(void)
{
    return PTL_OK;
}

void PtlFini(void)
{
}
