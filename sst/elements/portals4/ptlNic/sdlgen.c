/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

/*
 * Copyright 2009-2010 Sandia Corporation. Under the terms
 * of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 * Government retains certain rights in this software.
 *
 * Copyright (c) 2009-2010, Sandia Corporation
 * All rights reserved.
 *
 * This file is part of the SST software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <assert.h>

static void
print_usage(char *argv0)
{
    char *app = basename(argv0);

    fprintf(stderr, "Usage: %s [OPTION]...\n", app);
    fprintf(stderr, "Generate SDL file for P4 sim\n\n");
    fprintf(stderr, "Mandatory arguments to long options are mandatory for short options too.\n");
    fprintf(stderr, "  -x, --xdim=COUNT       Size of x dimension (default: 8)\n");
    fprintf(stderr, "  -y, --ydim=COUNT       Size of y dimension (default: 8)\n");
    fprintf(stderr, "  -z, --zdim=COUNT       Size of z dimension (default: 8)\n");
    fprintf(stderr, "      --latency=COUNT   Latency (in ns)\n");
    fprintf(stderr, "      --output=NAME  Output should be sent to NAME.xml and NAME-M5.xml (default: config)\n");
    fprintf(stderr, "      --ranks=COUNT If >1, will pre-partition for COUNT ranks (default: 1)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "must be specified\n");
}


static struct option longopts[] = {
    { "xdim",              required_argument, NULL, 'x' },
    { "ydim",              required_argument, NULL, 'y' },
    { "zdim",              required_argument, NULL, 'z' },
    { "latency",           required_argument, NULL, 'l' },
    { "output",            required_argument, NULL, 'o' },
    { "help",              no_argument,       NULL, 'h' },
    { "ranks",             required_argument, NULL, 'k' },
    { NULL,                0,                 NULL, 0   }
};


int
main(int argc, char **argv)
{
    int i, ch, size;
    int x_count = 8, y_count = 8, z_count = 8;
    int latency = 500;
    char * nic_link_latency = "150ns";
    FILE *output = stdout;

    int argc_org = argc;
    char **argv_org = argv;
    int ranks = 1;
    char *filePrefix = NULL;

    char fileName[256];
    
    while ((ch = getopt_long(argc, argv, "hx:y:z:r:e:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'x':
            x_count = atoi(optarg);
            break;
        case 'y':
            y_count = atoi(optarg);
            break;
        case 'z':
            z_count = atoi(optarg);
            break;
            latency = atoi(optarg);
            break;
        case 'o':
            filePrefix = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case 'k':
            ranks = atoi(optarg);
            break;
        default:
            print_usage(argv[0]);
            exit(1);
        }
    }

    if ( !filePrefix ) {
        print_usage(argv[0]); 
        exit(-1);
    }
    sprintf(fileName,"%s.xml",filePrefix);

    output = fopen( fileName ,"w"); 

    assert( output );

    size = x_count * y_count * z_count;

    extern void sdlgenM5( const char* file, int numM5Nids );

    sprintf(fileName,"%s-M5.xml",filePrefix);
    fprintf( stderr, "%d:%d:%d size=%d ranks=%d\n",
                    x_count, y_count, z_count, size, ranks );
    sdlgenM5( fileName,  size / ranks ); 

    fprintf(output, "<?xml version=\"1.0\"?>\n");
    fprintf(output, "<sdl version=\"2.0\"/>\n");
    fprintf(output, "\n");

    fprintf(output,"<|-- Command Line: -->\n ");
    fprintf(output,"<|-- ");
    for (i= 0; i < argc_org; i++)   {
        fprintf(output,"%s", argv_org[i]);
        if (i < (argc_org - 1))   {
            fprintf(output," ");
        }
    }

    fprintf(output," -->\n\n");

    fprintf(output, "<config>\n");
    fprintf(output, "    run-mode=both\n");
    fprintf(output, "    partitioner=self\n");
    fprintf(output, "</config>\n");
    fprintf(output, "\n");
    char* s_indent = "    ";
    char* s_noindent = "";

    char* indent = s_noindent;
	indent = s_indent;	
	fprintf(output, "<variables>\n");
	fprintf(output, "    <nic_link_lat> %s </nic_link_lat>\n",nic_link_latency);
	fprintf(output, "    <foo_link_lat> 1ps </foo_link_lat>\n");
	fprintf(output, "    <rtr_link_lat> 10ns </rtr_link_lat>\n");
	fprintf(output, "</variables>\n");
	fprintf(output, "\n");
	
	fprintf(output, "<param_include>\n");
    
    fprintf(output, "%s<rtr_params>\n",indent);
    fprintf(output, "%s    <clock>         500Mhz </clock>\n",indent);
    fprintf(output, "%s    <debug>         no     </debug>\n",indent);
    fprintf(output, "%s    <info>          no     </info>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s    <iLCBLat>       13     </iLCBLat>\n",indent);
    fprintf(output, "%s    <oLCBLat>       7      </oLCBLat>\n",indent);
    fprintf(output, "%s    <routingLat>    3      </routingLat>\n",indent);
    fprintf(output, "%s    <iQLat>         2      </iQLat>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s    <OutputQSize_flits>       16  </OutputQSize_flits>\n",indent);
    fprintf(output, "%s    <InputQSize_flits>        96  </InputQSize_flits>\n",indent);
    fprintf(output, "%s    <Router2NodeQSize_flits>  512 </Router2NodeQSize_flits>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s    <network.xDimSize> %d </network.xDimSize>\n",indent,x_count);
    fprintf(output, "%s    <network.yDimSize> %d </network.yDimSize>\n",indent,y_count);
    fprintf(output, "%s    <network.zDimSize> %d </network.zDimSize>\n",indent,z_count);
    fprintf(output, "\n");
    fprintf(output, "%s    <routing.xDateline> 0 </routing.xDateline>\n",indent);
    fprintf(output, "%s    <routing.yDateline> 0 </routing.yDateline>\n",indent);
    fprintf(output, "%s    <routing.zDateline> 0 </routing.zDateline>\n",indent);
    fprintf(output, "%s</rtr_params>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params1>\n",indent);
    fprintf(output, "%s    <clock>500Mhz</clock>\n",indent);
    fprintf(output, "%s</nic_params1>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params2>\n",indent);
    fprintf(output, "%s    <info>no</info>\n",indent);
    fprintf(output, "%s    <debug>no</debug>\n",indent);
    fprintf(output, "%s    <dummyDebug> no </dummyDebug>\n",indent);
    fprintf(output, "%s    <latency> %d </latency>\n",indent, latency);
    fprintf(output, "%s</nic_params2>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<cpu_params>\n",indent);
    fprintf(output, "%s    <debug> 0 </debug>\n",indent);
    fprintf(output, "%s    <M5debug> none </M5debug>\n",indent);
    fprintf(output, "%s    <info> no </info>\n",indent);
    fprintf(output, "%s    <registerExit> yes </registerExit>\n",indent);
    fprintf(output, "%s    <configFile> %s </configFile>\n",indent,fileName);
    fprintf(output, "%s</cpu_params>\n",indent);
    fprintf(output, "\n");


	fprintf(output, "</param_include>\n");
	fprintf(output, "\n");
    
    fprintf(output, "<sst>\n");

    int* nidMap = malloc( sizeof(int) * size ); 
    for ( i = 0; i < size; ++i) {
        int x, y, z;

	    z = i / (x_count * y_count);
	    y = (i / x_count) % y_count;
	    x = i % x_count;

	    /* Partition logic */
        int rank = i % ranks;
        if ( ranks == 2 ) {
            if ( x < x_count/2 ) rank = 0;
            else rank = 1;
        }
        if ( ranks == 4 ) {
            rank = 0;
            if ( x >= x_count/2 ) rank = rank | 1;
            if ( y >= y_count/2 ) rank = rank | (1 << 1);
        }
        if ( ranks == 8 ) {
            rank = 0;
            if ( x >= x_count/2 ) rank = rank | 1;
            if ( y >= y_count/2 ) rank = rank | (1 << 1);
            if ( z >= z_count/2 ) rank = rank | (1 << 2);
        }

        if ( ranks == 16 ) {
            rank = 0;
	    if ( x >= 3*(x_count/4) ) rank = rank | 3;
	    else if ( x >= x_count/2 && x < 3*(x_count/4) ) rank = rank | 2;
	    else if ( x >= x_count/4 && x < x_count/2 ) rank = rank | 1;
            if ( y >= y_count/2 ) rank = rank | (1 << 2);
            if ( z >= z_count/2 ) rank = rank | (1 << 3);
        }

        nidMap[i] = rank; 
        //printf("nidMap[%d]=%d\n",i,rank);
	    fprintf(output, "    <component name=%d.nic type=portals4.PtlNic rank=%d >\n",i,rank);
	    fprintf(output, "        <params include=nic_params1,nic_params2>\n");
	    fprintf(output, "            <id> %d </id>\n",i);
	    fprintf(output, "            <nid> %d </nid>\n",i);
	    fprintf(output, "        </params>\n");
	    fprintf(output, "        <link name=%d.cpu2nic port=mmif latency=$foo_link_lat/>\n",i);
	    fprintf(output, "        <link name=%d.dmaLink port=dma latency=$foo_link_lat/>\n",i);
	    fprintf(output, "        <link name=%d.nic2rtr port=rtr latency=$nic_link_lat/>\n",i);
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    fprintf(output, "    <component name=%d.rtr type=SS_router.SS_router rank=%d >\n",i,rank);
	    fprintf(output, "        <params include=rtr_params>\n");
	    fprintf(output, "            <id> %d </id>\n",i);
	    fprintf(output, "        </params>\n");
	    fprintf(output, "        <link name=%d.nic2rtr port=nic latency=$nic_link_lat/>\n",i);

	    if ( x_count > 1 ) {
		    fprintf(output, "        <link name=xr2r.%d.%d.%d port=xPos latency=$rtr_link_lat/>\n",y,z,(x+1)%x_count);
		    fprintf(output, "        <link name=xr2r.%d.%d.%d port=xNeg latency=$rtr_link_lat/>\n",y,z,x);
	    }

	    if ( y_count > 1 ) {
		    fprintf(output, "        <link name=yr2r.%d.%d.%d port=yPos latency=$rtr_link_lat/>\n",x,z,(y+1)%y_count);
		    fprintf(output, "        <link name=yr2r.%d.%d.%d port=yNeg latency=$rtr_link_lat/>\n",x,z,y);
	    }

	    if ( z_count > 1 ) {
		    fprintf(output, "        <link name=zr2r.%d.%d.%d port=zPos latency=$rtr_link_lat/>\n",x,y,(z+1)%z_count);
		    fprintf(output, "        <link name=zr2r.%d.%d.%d port=zNeg latency=$rtr_link_lat/>\n",x,y,z);
	    }
	
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    fprintf(output, "\n");
    }

    // this is really brain dead but...
    int rank;
    int* foo = malloc( sizeof( int ) * rank );


    for ( rank = 0; rank < ranks; rank++) {

	    fprintf(output, "    <component name=%d.cpu type=m5C.M5 rank=%d >\n", rank, rank );
	    fprintf(output, "        <params include=cpu_params>\n");

        memset( foo, 0, sizeof(int)*ranks); 
        int j;
        for ( j = 0; j < size; j++ ) {

            if ( nidMap[j] == rank ) {
    	        fprintf(output, "            <nid%d.cpu0.base.process.nid> %d </nid%d.cpu0.base.process.nid>\n",foo[rank],j,foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.cmd.0> ${M5_EXE} </nid%d.cpu0.base.process.cmd.0>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.cmd.1> ${ARG1} </nid%d.cpu0.base.process.cmd.1>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.cmd.2> ${ARG2} </nid%d.cpu0.base.process.cmd.2>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.cmd.3> ${ARG3} </nid%d.cpu0.base.process.cmd.3>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.env.0> RT_RANK=%d </nid%d.cpu0.base.process.env.0>\n",foo[rank],j,foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.env.1> RT_SIZE=%d </nid%d.cpu0.base.process.env.1>\n",foo[rank],size,foo[rank]);
    	        fprintf(output, "            <nid%d.cpu0.base.process.env.2> PTLNIC_CMD_QUEUE_ADDR=0x2000 </nid%d.cpu0.base.process.env.2>\n",foo[rank],foo[rank]);

    	        fprintf(output, "            <nid%d.physmem.exe0.process.cmd.0> ${M5_EXE} </nid%d.physmem.exe0.process.cmd.0>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.cmd.1> ${ARG1} </nid%d.physmem.exe0.process.cmd.1>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.cmd.2> ${ARG2} </nid%d.physmem.exe0.process.cmd.2>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.cmd.3> ${ARG3} </nid%d.physmem.exe0.process.cmd.3>\n",foo[rank],foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.env.0> RT_RANK=%d </nid%d.physmem.exe0.process.env.0>\n",foo[rank],j,foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.env.1> RT_SIZE=%d </nid%d.physmem.exe0.process.env.1>\n",foo[rank],size,foo[rank]);
    	        fprintf(output, "            <nid%d.physmem.exe0.process.env.2> PTLNIC_CMD_QUEUE_ADDR=0x2000 </nid%d.physmem.exe0.process.env.2>\n",foo[rank],foo[rank]);
                ++foo[rank];
            }
        }

	    fprintf(output, "        </params>\n");

        memset( foo, 0, sizeof(int)*ranks); 
        for ( j = 0; j < size; j++ ) {
            if ( nidMap[j] == rank ) {
	            fprintf(output, "        <link name=%d.cpu2nic port=nic%d latency=$foo_link_lat/>\n",j,foo[rank]);
	            fprintf(output, "        <link name=%d.dmaLink port=dma%d latency=$foo_link_lat/>\n",j,foo[rank]);
                ++foo[rank];
            }
        }
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
    }
    fprintf(output, "</sst>\n");

    return 0;
}
