/*
** Some commonly used functions by the benchmarks
*/

#include <stdio.h>
#include "util.h"


void
disp_cmd_line(int argc, char **argv)
{

int i;


    printf("# Command line \"");
    for (i= 0; i < argc; i++)   {
	printf("%s", argv[i]);
	if (i < (argc - 1))   {
	    printf(" ");
	}
    }
    printf("\"\n");

}  /* end of disp_cmd_line() */
