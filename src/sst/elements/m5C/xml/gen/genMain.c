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
#include "./gen.h"
#include <stdlib.h>
#include <assert.h>


static void foo( FILE *fp, int numCores, int argc, char *argv[] );

int main( int argc, char* argv[] )
{
    int numCores;
    numCores = atoi( argv[1] );

    foo( stdout, numCores, argc - 1, argv + 1 );

    return 0;
}

static void foo( FILE *fp, int numCores, int argc, char *argv[] )
{
    int L2_cacheSize = (256 * 1024)/1;
    int L1_iCacheSize = 32 * 1024;
    int L1_dCacheSize = (32 * 1024)/1; 
    int numExe = 1;
    int L1_busWidth = 64;
    int L2_busWidth = 64;
    float L1_busClock = 1;
    float L2_busClock = 1;
    int L1_lat = 2000; //ps
    int L1_assoc = 2;
    int L1_mshrs = 32;
    int L1_prefetch_lat=4000;
    int L1_prefetcher_size=64;
    int L1_tgts_per_mshr=64;

    int memory_lat = 100;
    int L2_lat = 20000; //ps
    int L2_assoc = 16;
    int L2_mshrs = 64;
    int L2_prefetch_lat=12000;
    int L2_prefetcher_size=512;
    int L2_tgts_per_mshr=64;
    int numMC = 1;
    int i;
    int linkNum;
    //char *dramConfig = "dramsim_configs/ini/DDR3_micron_32M_8B_x4_sg125.ini";
    char *dramConfig = "dramsim_configs/ini/GDDR5_hynix_1Gb_16B_x64ish_sg0677.ini";
    int dramTXQ = 64; 
    addr_t physStart = 0x00000000;
    addr_t physEnd = 0x7fffffff;
    addr_t exeStart = 0x00000000;
    addr_t exeSize = 0x80000000 ;
    int nid = 0, core, sstRank = 0;

    fprintf(stderr, "numCores %d\n",numCores);
    fprintf(stderr, "iCache %d \ndCache %d\nL2 %d\n",
                    L1_iCacheSize, L1_dCacheSize, L2_cacheSize);
    fprintf(stderr, "L1 latency %d cycles\nL2 latency %d cycles\n",
                                        L1_lat, L2_lat );
    fprintf(stderr, "L1 bus clock %f Ghz\nL2 bus clock %f Ghz\n",
                                        L1_busClock,L2_busClock); 
    // but called functions want 0 based argv but don't used argv[0]
    // so cheat

    fprintf( fp, "<?xml version=\"1.0\"?>\n\n" );

    fprintf(fp,"<sdl version=\"2.0\"/>\n\n");

    fprintf(fp,"<variables>\n");
    fprintf(fp,"    <lat> 10ns </lat>\n");
    fprintf(fp,"</variables>\n\n");

    fprintf(fp,"<param_include>\n");

    genCpuParams( fp );
    fprintf(fp,"\n");
    genCacheParams( fp );
    fprintf(fp,"\n");
    
    genBusParams( fp );
    fprintf(fp,"\n");

    fprintf( fp, "</param_include>\n");

    fprintf( fp, "<sst>\n");

    char name[1024];
    char cpuSide[1024],memSide[1024],dcachePort[1024],icachePort[1024];
    for ( core = 0; core < numCores; core++ ) { 
        addr_t tmp = exeStart + (exeSize * core);
        sprintf(name,"nid%d.cpu%d.cpu", nid, core );
        sprintf(dcachePort,"nid%d.cpu%d.cpu-l1d",nid,core);
        sprintf(icachePort,"nid%d.cpu%d.cpu-l1i",nid,core);
        genCpu( fp, name, core, sstRank, tmp, tmp + (exeSize - 1), 
                    argv, argc, dcachePort, icachePort, dramConfig, dramTXQ );
        fprintf( fp, "\n");

        sprintf(name,"nid%d.cpu%d.l1d", nid, core );
        sprintf(cpuSide,"nid%d.cpu%d.cpu-l1d",nid,core);
        sprintf(memSide,"nid%d.cpu%d.l2bus-l1d",nid,core);
        genCache( fp, name, sstRank, cpuSide, memSide, L1_dCacheSize, 1, L1_lat, L1_assoc, L1_mshrs, L1_prefetch_lat, L1_prefetcher_size, L1_tgts_per_mshr );
        fprintf( fp, "\n");

        sprintf(name,"nid%d.cpu%d.l1i", nid, core );
        sprintf(cpuSide,"nid%d.cpu%d.cpu-l1i",nid,core);
        sprintf(memSide,"nid%d.cpu%d.l2bus-l1i",nid,core);
        genCache( fp, name, sstRank, cpuSide, memSide, L1_iCacheSize, 1, L1_lat, L1_assoc, L1_mshrs, L1_prefetch_lat, L1_prefetcher_size,L1_tgts_per_mshr );
        fprintf( fp, "\n");

        linkName_t links[4];

        sprintf(links[0],"nid%d.cpu%d.l2bus-l2", nid, core );
        sprintf(links[1],"nid%d.cpu%d.l2bus-l1d", nid, core );
        sprintf(links[2],"nid%d.cpu%d.l2bus-l1i", nid, core );
        memcpy(links[3],"",0);

        sprintf(name,"nid%d.cpu%d.l2bus", nid, core );
        genBus( fp, name, sstRank, links, L1_busWidth, L1_busClock );

        fprintf( fp, "\n");

        sprintf(name,"nid%d.cpu%d.l2", nid, core );
        sprintf(cpuSide,"nid%d.cpu%d.l2bus-l2",nid,core);
        sprintf(memSide,"nid%d.membus-l2%d",nid,core);
        genCache( fp, name, sstRank, cpuSide, memSide, L2_cacheSize, 0, L2_lat, L2_assoc, L2_mshrs, L2_prefetch_lat, L2_prefetcher_size, L2_tgts_per_mshr );

        fprintf( fp, "\n");

    }

    fprintf( fp, "\n");

    linkName_t *links = malloc( sizeof(linkName_t) * numCores + numMC );

    linkNum = 0;
    for ( core = 0; core < numCores; core++ ) {
        sprintf(links[linkNum],"nid%d.membus-l2%d", nid, core );
        ++linkNum;
    }
    
    for ( i = 0; i < numMC; i++ ) { 
        sprintf(links[linkNum],"nid%d.membus-memory%d", nid, i );
        ++linkNum;
    }
    memcpy(links[linkNum],"",0);

    sprintf(name,"nid%d.memBus", nid );
    genBus( fp, name, sstRank, links, L2_busWidth, L2_busClock );

    fprintf( fp, "\n");

    for ( i = 0; i < numMC; i++ ) { 
        linkName_t link;
        sprintf(name,"nid%d.memory%d", nid, i );
        sprintf(link,"nid%d.membus-memory%d", nid, i );

        genPhysmem( fp, name, sstRank, numExe, physStart, physEnd, exeStart, 
                   exeSize, argv, argc, link, dramConfig, dramTXQ, memory_lat );

        fprintf( fp, "\n");
    }

    fprintf( fp, "</sst>\n");
}
