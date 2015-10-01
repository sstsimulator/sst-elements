/*
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
 */


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "portals_args.h"



int
main(int argc, char **argv)
{
    struct portals_args args;

    char * nic_link_latency = "150ns";
    FILE *output = stdout;
    
    parse_partals_args(argc, argv, &args);

    int i;

    output = fopen(args.output, "w");
    
    if (0 != args.noise_runs && (NULL == args.noise_interval || NULL == args.noise_duration)) {
        print_usage(argv[0]);
        exit(1);
    }

    if (NULL == args.application) {
        print_usage(argv[0]);
        exit(1);
    }

    /* clean up so SDL file looks nice */
    if (NULL == args.noise_interval) args.noise_interval = "1kHz";
    if (NULL == args.noise_duration) args.noise_duration = "25us";

    if ( args.timing_set == 1 ) nic_link_latency = "100ns";
    if ( args.timing_set == 2 ) nic_link_latency = "200ns";
    if ( args.timing_set == 3 ) nic_link_latency = "250ns";
    
    int size = args.x * args.y * args.z;

    fprintf(output, "<?xml version=\"1.0\"?>\n");

    if ( args.new_format ) fprintf(output, "\n<sdl version=\"2.0\"/>\n");
    fprintf(output, "\n");

    fprintf(output,"<!-- Command Line: -->\n ");
    fprintf(output,"<!-- ");
    for (i= 0; i < argc; i++)   {
        fprintf(output,"%s", argv[i]);
        if (i < (argc - 1))   {
            fprintf(output," ");
        }
    }

    fprintf(output," -->\n\n");
    

    fprintf(output, "<config>\n");
    fprintf(output, "    run-mode=both\n");
    if ( !args.self_partition ) fprintf(output, "    partitioner=portals4_sm.partitioner\n");
    else if ( args.ranks > 1 || args.new_format ) fprintf(output, "    partitioner=self\n");
    fprintf(output, "</config>\n");
    fprintf(output, "\n");
    char* s_indent = "    ";
    char* s_noindent = "";

    char* indent = s_noindent;
    if ( args.new_format ) {
	indent = s_indent;	
	fprintf(output, "<variables>\n");
	fprintf(output, "    <nic_link_lat> %s </nic_link_lat>\n",nic_link_latency);
	fprintf(output, "    <rtr_link_lat> 10ns </rtr_link_lat>\n");
	fprintf(output, "</variables>\n");
	fprintf(output, "\n");
	
	fprintf(output, "<param_include>\n");
    }
    else {
	fprintf(output, "<nicLink>\n");
	fprintf(output, "    <lat> %s </lat>\n",nic_link_latency);
	fprintf(output, "</nicLink>\n");
	fprintf(output, "\n");
	fprintf(output, "<rtrLink>\n");
	fprintf(output, "    <lat>10ns</lat>\n");
	fprintf(output, "</rtrLink>\n");
	fprintf(output, "\n");
    }
    
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
    fprintf(output, "%s    <network.xDimSize> %d </network.xDimSize>\n",indent,args.x);
    fprintf(output, "%s    <network.yDimSize> %d </network.yDimSize>\n",indent,args.y);
    fprintf(output, "%s    <network.zDimSize> %d </network.zDimSize>\n",indent,args.z);
    fprintf(output, "\n");
    fprintf(output, "%s    <routing.xDateline> 0 </routing.xDateline>\n",indent);
    fprintf(output, "%s    <routing.yDateline> 0 </routing.yDateline>\n",indent);
    fprintf(output, "%s    <routing.zDateline> 0 </routing.zDateline>\n",indent);
    fprintf(output, "%s</rtr_params>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<cpu_params>\n",indent);
    fprintf(output, "%s    <radix> %d </radix>\n",indent,args.radix);
    fprintf(output, "%s    <timing_set> %d </timing_set>\n",indent,args.timing_set);
    fprintf(output, "%s    <nodes> %d </nodes>\n",indent,size);
    fprintf(output, "%s    <msgrate> %s </msgrate>\n",indent, args.msg_rate);
    fprintf(output, "%s    <xDimSize> %d </xDimSize>\n",indent,args.x);
    fprintf(output, "%s    <yDimSize> %d </yDimSize>\n",indent,args.y);
    fprintf(output, "%s    <zDimSize> %d </zDimSize>\n",indent,args.z);
    fprintf(output, "%s    <noiseRuns> %d </noiseRuns>\n",indent, args.noise_runs);
    fprintf(output, "%s    <noiseInterval> %s </noiseInterval>\n",indent, args.noise_interval);
    fprintf(output, "%s    <noiseDuration> %s </noiseDuration>\n",indent, args.noise_duration);
    fprintf(output, "%s    <application> %s </application>\n",indent, args.application);
    fprintf(output, "%s    <latency> %d </latency>\n",indent, args.latency);    
    fprintf(output, "%s    <msg_size> %d </msg_size>\n",indent, args.msg_size);    
    fprintf(output, "%s    <chunk_size> %d </chunk_size>\n",indent, args.chunk_size);    
    fprintf(output, "%s    <coalesce> %d </coalesce>\n",indent, args.coalesce);
    fprintf(output, "%s    <enable_putv> %d </enable_putv>\n",indent, args.enable_putv);
    fprintf(output, "%s</cpu_params>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params1>\n",indent);
    fprintf(output, "%s    <clock>500Mhz</clock>\n",indent);
    fprintf(output, "%s    <timing_set> %d </timing_set>\n",indent,args.timing_set);
    fprintf(output, "%s</nic_params1>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params2>\n",indent);
    fprintf(output, "%s    <info>no</info>\n",indent);
    fprintf(output, "%s    <debug>no</debug>\n",indent);
    fprintf(output, "%s    <dummyDebug> no </dummyDebug>\n",indent);
    fprintf(output, "%s    <latency> %d </latency>\n",indent, args.latency);
    fprintf(output, "%s</nic_params2>\n",indent);
    fprintf(output, "\n");

    if ( args.new_format ) {
	fprintf(output, "</param_include>\n");
	fprintf(output, "\n");
    }
    
    fprintf(output, "<sst>\n");

    for ( i = 0; i < size; ++i) {
        int x, y, z;

	z = i / (args.x * args.y);
	y = (i / args.x) % args.y;
	x = i % args.x;

	/* Partition logic */
        int rank = i % args.ranks;
        if ( args.ranks == 2 ) {
            if ( x < args.x/2 ) rank = 0;
            else rank = 1;
        }
        if ( args.ranks == 4 ) {
            rank = 0;
            if ( x >= args.x/2 ) rank = rank | 1;
            if ( y >= args.y/2 ) rank = rank | (1 << 1);
        }
        if ( args.ranks == 8 ) {
            rank = 0;
            if ( x >= args.x/2 ) rank = rank | 1;
            if ( y >= args.y/2 ) rank = rank | (1 << 1);
            if ( z >= args.z/2 ) rank = rank | (1 << 2);
        }

        if ( args.ranks == 16 ) {
            rank = 0;
	    if ( x >= 3*(args.x/4) ) rank = rank | 3;
	    else if ( x >= args.x/2 && x < 3*(args.x/4) ) rank = rank | 2;
	    else if ( x >= args.x/4 && x < args.x/2 ) rank = rank | 1;
            if ( y >= args.y/2 ) rank = rank | (1 << 2);
            if ( z >= args.z/2 ) rank = rank | (1 << 3);
        }

	if ( args.new_format ) {
	    if ( args.self_partition ) fprintf(output, "    <component name=%d.cpu type=portals4_sm.trig_cpu rank=%d >\n",i,rank);
	    else                  fprintf(output, "    <component name=%d.cpu type=portals4_sm.trig_cpu >\n",i);
	    fprintf(output, "        <params include=cpu_params>\n");
	    fprintf(output, "            <id> %d </id>\n",i);
	    fprintf(output, "        </params>\n");
	    fprintf(output, "        <link name=%d.cpu2nic port=nic latency=$nic_link_lat/>\n",i);
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    if ( args.self_partition ) fprintf(output, "    <component name=%d.nic type=portals4_sm.trig_nic rank=%d >\n",i,rank);
	    else                  fprintf(output, "    <component name=%d.nic type=portals4_sm.trig_nic >\n",i);
	    fprintf(output, "        <params include=nic_params1,nic_params2>\n");
	    fprintf(output, "            <id> %d </id>\n",i);
	    fprintf(output, "        </params>\n");
	    fprintf(output, "        <link name=%d.cpu2nic port=cpu latency=$nic_link_lat/>\n",i);
	    fprintf(output, "        <link name=%d.nic2rtr port=rtr latency=$nic_link_lat/>\n",i);
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    if ( args.self_partition ) fprintf(output, "    <component name=%d.rtr type=SS_router.SS_router rank=%d >\n",i,rank);
	    else                  fprintf(output, "    <component name=%d.rtr type=SS_router.SS_router >\n",i);
	    fprintf(output, "        <params include=rtr_params>\n");
	    fprintf(output, "            <id> %d </id>\n",i);
	    fprintf(output, "        </params>\n");
	    fprintf(output, "        <link name=%d.nic2rtr port=nic latency=$nic_link_lat/>\n",i);

	    if ( args.x > 1 ) {
		fprintf(output, "        <link name=xr2r.%d.%d.%d port=xPos latency=$rtr_link_lat/>\n",y,z,(x+1)%args.x);
		fprintf(output, "        <link name=xr2r.%d.%d.%d port=xNeg latency=$rtr_link_lat/>\n",y,z,x);
	    }

	    if ( args.y > 1 ) {
		fprintf(output, "        <link name=yr2r.%d.%d.%d port=yPos latency=$rtr_link_lat/>\n",x,z,(y+1)%args.y);
		fprintf(output, "        <link name=yr2r.%d.%d.%d port=yNeg latency=$rtr_link_lat/>\n",x,z,y);
	    }

	    if ( args.z > 1 ) {
		fprintf(output, "        <link name=zr2r.%d.%d.%d port=zPos latency=$rtr_link_lat/>\n",x,y,(z+1)%args.z);
		fprintf(output, "        <link name=zr2r.%d.%d.%d port=zNeg latency=$rtr_link_lat/>\n",x,y,z);
	    }
	
	    fprintf(output, "    </component>\n");
	}
	else {
	    if ( args.ranks > 1 ) {
		fprintf(output, "    <component id=\"%d.cpu\" rank=%d >\n",i,rank);
	    }
	    else {
		fprintf(output, "    <component id=\"%d.cpu\" >\n",i);
	    }
	    fprintf(output, "        <portals4_sm.trig_cpu>\n");
	    fprintf(output, "            <params include1=cpu_params>\n");
	    fprintf(output, "                <id> %d </id>\n",i);
	    fprintf(output, "            </params>\n");
	    fprintf(output, "            <links>\n");
	    fprintf(output, "                <link id=\"%d.cpu2nic\">\n",i);
	    fprintf(output, "        	    <params include=nicLink>\n");
	    fprintf(output, "                        <name> nic </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    fprintf(output, "            </links>\n");
	    fprintf(output, "        </portals4_sm.trig_cpu>\n");
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    fprintf(output, "    <component id=\"%d.nic\" rank=%d >\n",i,rank);
	    fprintf(output, "        <portals4_sm.trig_nic>\n");
	    fprintf(output, "            <params include1=nic_params1 include2=nic_params2>\n");
	    fprintf(output, "                <id> %d </id>\n",i);
	    fprintf(output, "            </params>\n");
	    fprintf(output, "            <links>\n");
	    fprintf(output, "                <link id=\"%d.cpu2nic\">\n",i);
	    fprintf(output, "        	    <params include=nicLink>\n");
	    fprintf(output, "                        <name> cpu </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
	    fprintf(output, "        	    <params include=nicLink>\n");
	    fprintf(output, "                        <name> rtr </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");
	    fprintf(output, "            </links>\n");
	    fprintf(output, "        </portals4_sm.trig_nic>\n");
	    fprintf(output, "    </component>\n");
	    fprintf(output, "\n");
	    fprintf(output, "    <component id=\"%d.rtr\" rank=%d >\n",i,rank);
	    fprintf(output, "        <SS_router.SS_router>\n");
	    fprintf(output, "            <params include=rtr_params>\n");
	    fprintf(output, "                <id> %d </id>\n",i);
	    fprintf(output, "            </params>\n");
	    fprintf(output, "            <links>\n");
	    fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
	    fprintf(output, "                    <params include=nicLink>\n");
	    fprintf(output, "                        <name> nic </name>\n");
	    fprintf(output, "                    </params>\n");
	    fprintf(output, "                </link>\n");

	    if ( args.x > 1 ) {
		fprintf(output, "                <link id=\"xr2r.%d.%d.%d\">\n",y,z,(x+1)%args.x);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> xPos </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    
		fprintf(output, "                <link id=\"xr2r.%d.%d.%d\">\n",y,z,x);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> xNeg </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    }

	    if ( args.y > 1 ) {
		fprintf(output, "                <link id=\"yr2r.%d.%d.%d\">\n",x,z,(y+1)%args.y);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> yPos </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    
		fprintf(output, "                <link id=\"yr2r.%d.%d.%d\">\n",x,z,y);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> yNeg </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    }

	    if ( args.z > 1 ) {
		fprintf(output, "                <link id=\"zr2r.%d.%d.%d\">\n",x,y,(z+1)%args.z);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> zPos </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    
		fprintf(output, "                <link id=\"zr2r.%d.%d.%d\">\n",x,y,z);
		fprintf(output, "                    <params include=rtrLink>\n");
		fprintf(output, "                        <name> zNeg </name>\n");
		fprintf(output, "                    </params>\n");
		fprintf(output, "                </link>\n");
	    }
	
	    fprintf(output, "            </links>\n");
	    fprintf(output, "        </SS_router.SS_router>\n");
	    fprintf(output, "    </component>\n");
	}
	fprintf(output, "\n");
	fprintf(output, "\n");
    }
    fprintf(output, "</sst>\n");

    return 0;
}
