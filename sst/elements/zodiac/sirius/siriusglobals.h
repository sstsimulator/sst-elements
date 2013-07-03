
#include "mpi.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "siriusconst.h"

int sirius_rank;
int sirius_npes;
struct timespec load_lib_time;
double load_library;

FILE* trace_dump;
