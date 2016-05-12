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

void genCpu( FILE *fp, char* name, int core, int sstRank, addr_t start, 
                addr_t end, char *argv[], int argc, char *dcache_port, char* icache_port, char *dramConfig, int dramTXQ )
{
    fprintf(fp,"%s<component name=%s type=O3Cpu rank=%d>\n", tab(fp,1), name, sstRank );
    fprintf(fp,"%s    <params include=cpuParams>\n", tab(fp,1));


    if ( core == 0 ) {
        fprintf(fp,"%s    <base.process.cmd.0> ${M5_EXE}  </base.process.cmd.0>\n", tab(fp,1));
        int i;
        // skip argv[0]
        for ( i = 1; i < argc; i++ ) {
            fprintf(fp,"%s    <base.process.cmd.%d> %s  </base.process.cmd.%d>\n",
                 tab(fp,1),i,argv[i],i);
        }
        fprintf(fp,"%s    <base.process.env.0> RT_RANK=0  </base.process.env.0>\n", tab(fp,1));
        fprintf(fp,"%s    <physicalMemory.start> %0#18lx </physicalMemory.start>\n",tab(fp,1),start);
        fprintf(fp,"%s    <physicalMemory.end>   %0#18lx </physicalMemory.end>\n",tab(fp,1),end);
        fprintf(fp,"%s    <ddrConfig> %s </ddrConfig>\n",tab(fp,1),dramConfig);
        fprintf(fp,"%s    <tx_q> %d </tx_q>\n",tab(fp,1),dramTXQ);
    }

    fprintf(fp,"%s    <base.cpu_id> %d </base.cpu_id>\n",tab(fp,1),core);

    fprintf(fp,"%s    </params>\n",tab(fp,1));
    fprintf(fp,"%s    <link name=%s port=dcache_port latency=$lat/>\n", tab(fp,1), dcache_port );
    fprintf(fp,"%s    <link name=%s port=icache_port latency=$lat/>\n", tab(fp,1), icache_port );
    fprintf(fp,"%s</component>\n",tab(fp,1));
}
