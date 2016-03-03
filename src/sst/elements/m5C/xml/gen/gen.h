// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <stdio.h>
#include <string.h>

typedef unsigned long addr_t;

typedef char  linkName_t[1024];

static inline char *tab( FILE* fp, int num )
{
    int offset = num * 4;
    static char *tmp = "                                        ";
    return tmp + (strlen(tmp) - offset);
}


void genCpuParams( FILE* fp );
void genCacheParams( FILE* fp );
void genBusParams( FILE* fp );

void genCpu( FILE *fp, char *name, int core, int sstRank, 
                       addr_t start, addr_t end, char* argv[], int argc, char*, char*,
                        char *dramConfig, int dramTxQ );
void genCache( FILE* fp, char *name, int sstRank, 
                       char* cpuSide,  char* memSide, int size, int top, int lat,
    int assoc, int mshrs, int prefetch_lat, int prefetcher_size, int tgts_per_mshr );
void genBus( FILE* fp, char *name, int sstRank, linkName_t linkNames[], int busWidth, float busClock );

void genPhysmem( FILE* fp, char *name, int sstRank, int numExe, 
    addr_t physStart, addr_t physEend, addr_t exeStart, addr_t exeSize, 
                        char* argv[], int argc, linkName_t link,
                        char *dramConfig, int dramTxQ, int lat );
