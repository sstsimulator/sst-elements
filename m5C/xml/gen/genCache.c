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

#include "./gen.h"

void genCache( FILE* fp, char* name, int sstRank, char *cpuSide, char* memSide, int size, int top, int latency, 
                int assoc, int mshrs, int prefetch_lat, int prefetcher_size, int tgts_per_mshr )
{
    fprintf(fp,"%s<component name=%s type=BaseCache rank=%d >\n", tab(fp,1), name, sstRank );
    fprintf(fp,"%s    <params include=cacheParams>\n",tab(fp,1));
    fprintf(fp,"%s        <size> %d </size>\n", tab(fp,1), size );
    fprintf(fp,"%s        <is_top_level> %s </is_top_level>\n", tab(fp,1), top ? "true" : "false" );
    fprintf(fp,"%s        <latency> %d </latency>\n", tab(fp,1), latency );
    fprintf(fp,"%s        <assoc> %d </assoc>\n", tab(fp,1), assoc );
    fprintf(fp,"%s        <mshrs> %d </mshrs>\n", tab(fp,1), mshrs );
    fprintf(fp,"%s        <prefetch_latency> %d </prefetch_latency>\n", tab(fp,1), prefetch_lat );
    fprintf(fp,"%s        <prefetcher_size> %d </prefetcher_size>\n", tab(fp,1), prefetcher_size );
    fprintf(fp,"%s        <tgts_per_mshr> %d </tgts_per_mshr>\n", tab(fp,1), tgts_per_mshr );
    fprintf(fp,"%s    </params>\n",tab(fp,1));
    fprintf(fp,"%s    <link name=%s port=cpu_side latency=$lat />\n", tab(fp,1), cpuSide );
    fprintf(fp,"%s    <link name=%s port=mem_side latency=$lat />\n", tab(fp,1), memSide );
    fprintf(fp,"%s</component>\n",tab(fp,1));
}
