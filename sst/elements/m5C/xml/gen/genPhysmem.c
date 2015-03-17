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

void genPhysmem( FILE *fp, char *name, int sstRank, int numExe, addr_t physStart,
    addr_t physEnd, addr_t exeStart, addr_t exeSize, char* argv[], int argc, linkName_t link, char *dramConfig, int dramTXQ, int lat ) 
{
    fprintf(fp,"%s<component name=%s type=PhysicalMemory rank=%d >\n",tab(fp,1),name,sstRank);
    fprintf(fp,"%s    <params>\n",tab(fp,1));
    fprintf(fp,"%s        <range.start> %0#18lx </range.start>\n",tab(fp,1),physStart);
    fprintf(fp,"%s        <range.end> %0#18lx </range.end>\n",tab(fp,1),physEnd);
    fprintf(fp,"%s        <latency> %dns </latency>\n",tab(fp,1),lat);
    fprintf(fp,"%s        <latency_var> 0ns </latency_var>\n",tab(fp,1));
    fprintf(fp,"%s        <null> false </null>\n",tab(fp,1));
    fprintf(fp,"%s        <zero> false </zero>\n",tab(fp,1));
    fprintf(fp,"%s        <tx_q> %d </tx_q>\n",tab(fp,1), dramTXQ );
    fprintf(fp,"%s        <ddrConfig> %s </ddrConfig>\n",tab(fp,1), dramConfig );

    int  exe;
    for ( exe = 0; exe < numExe; exe++ ) {
        addr_t tmp = exeStart + (exe*exeSize);
        fprintf(fp,"%s        <exe%d.process.executable> ${M5_EXE} </exe%d.process.executable>\n",tab(fp,1),exe,exe);
        fprintf(fp,"%s        <exe%d.process.cmd.0> ${M5_EXE}  </exe%d.process.cmd.0>\n",tab(fp,1),exe,exe);
        int i;
        // skip argv[0]
        for ( i = 1; i < argc; i++ ) { 
            fprintf(fp,"%s        <exe%d.process.cmd.%d> %s </exe%d.process.cmd.%d>\n",tab(fp,1),exe,i,argv[i],exe,i);
        }
        fprintf(fp,"%s        <exe%d.process.env.0> RT_RANK=0 </exe%d.process.env.0>\n",tab(fp,1),exe,exe);
        fprintf(fp,"%s        <exe%d.physicalMemory.start> %0#18lx </exe%d.physicalMemory.start>\n", tab(fp,1),exe,tmp,exe);
        fprintf(fp,"%s        <exe%d.physicalMemory.end>   %0#18lx </exe%d.physicalMemory.end>\n",tab(fp,1),exe,tmp+(exeSize-1),exe);
    }

    fprintf(fp,"%s    </params>\n",tab(fp,1));
    fprintf(fp,"%s    <link name=%s port=port latency=$lat />\n",tab(fp,1),link); 
    fprintf(fp,"%s</component>\n",tab(fp,1));
}
