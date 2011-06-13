#ifndef PTL_INTERNAL_ERROR_H
#define PTL_INTERNAL_ERROR_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>                    /* for getenv() */

#include "ptl_internal_commpad.h"

#ifdef NO_ARG_VALIDATION
# define VERBOSE_ERROR(...)
#else
static int verbose_error_flag = 2;

static void VERBOSE_ERROR(const char *restrict format,
                          ...)
{
    switch (verbose_error_flag) {
        case 0:
            if (getenv("VERBOSE") != NULL) {
                verbose_error_flag = 2;
                goto print_error;
            } else {
                verbose_error_flag = 1;
                return;
            }
        case 2:
print_error:
            {
                va_list ap;
                printf("PORTALS4-> (%lu) ERROR: ", (unsigned long)proc_number);
                va_start(ap, format);
                vprintf(format, ap);
                fflush(stdout);
                va_end(ap);
                break;
            }
    }
}

#endif /* NO_ARG_VALIDATION */

#endif /* ifndef PTL_INTERNAL_ERROR_H */
/* vim:set expandtab: */
