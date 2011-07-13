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
