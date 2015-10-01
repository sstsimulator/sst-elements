/*
 * =====================================================================================
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
 * =====================================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>

static void
print_usage(char *argv0)
{
    char *app = basename(argv0);

    fprintf(stderr, "Usage: %s --application=<application> [OPTION]...\n", app);
    fprintf(stderr, "Generate SDL file for triggered CPU/NIC\n\n");
    fprintf(stderr, "Mandatory arguments to long options are mandatory for short options too.\n");
    fprintf(stderr, "  -x, --xdim=COUNT       Size of x dimension (default: 8)\n");
    fprintf(stderr, "  -y, --ydim=COUNT       Size of y dimension (default: 8)\n");
    fprintf(stderr, "  -z, --zdim=COUNT       Size of z dimension (default: 8)\n");
    fprintf(stderr, "  -r, --radix=COUNT      Radix of tree based algorithms (default: 4)\n");
    fprintf(stderr, "  -t, --timing_set=COUNT Timing set to use (default: 2)\n");
    fprintf(stderr, "      --noise_runs=COUNT Number of iterations when noise enabled (default: 0)\n");
    fprintf(stderr, "      --noise_interval=STRING Interval for noise when enabled\n");
    fprintf(stderr, "      --noise_duration=STRING Duration for noise when enabled\n");
    fprintf(stderr, "      --msg_rate=STRING  Message rate\n");
    fprintf(stderr, "      --latency=COUNT   Latency (in ns)\n");
    fprintf(stderr, "      --message_size=SIZE Size in bytes of message\n");
    fprintf(stderr, "      --chunk_size=SIZE Size in bytes of pipeline chunk\n");
    fprintf(stderr, "      --application=STRING Application to run (required)\n");
    fprintf(stderr, "      --coalesce=0/1 Enable/Disable coalescing of portals commands (default: 1/true)\n");
    fprintf(stderr, "      --enable_putv=0/1 Enable/Disable coalescing of portals commands (default: 0/false)\n");
    fprintf(stderr, "      --output=FILENAME  Output should be sent to FILENAME (default: stdout)\n");
    fprintf(stderr, "      --ranks=COUNT If >1, will pre-partition for COUNT ranks (default: 1)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "NOTE: If noise_runs is non-zero, noise_interval and noise_duration\n");
    fprintf(stderr, "must be specified\n");
}


static struct option longopts[] = {
    { "xdim",              required_argument, NULL, 'x' },
    { "ydim",              required_argument, NULL, 'y' },
    { "zdim",              required_argument, NULL, 'z' },
    { "radix",             required_argument, NULL, 'r' },
    { "timing_set",        required_argument, NULL, 't' },
    { "noise_runs",        required_argument, NULL, 'n' },
    { "noise_interval",    required_argument, NULL, 'i' },
    { "noise_duration",    required_argument, NULL, 'd' },
    { "msg_rate",          required_argument, NULL, 'm' },
    { "latency",           required_argument, NULL, 'l' },
    { "message_size",      required_argument, NULL, 'q' },
    { "chunk_size",        required_argument, NULL, 'w' },
    { "output",            required_argument, NULL, 'o' },
    { "help",              no_argument,       NULL, 'h' },
    { "application",       required_argument, NULL, 'a' },
    { "coalesce",          required_argument, NULL, 'c' },
    { "enable_putv",       required_argument, NULL, 'p' },
    { "ranks",             required_argument, NULL, 'k' },
    { "new_format",        no_argument,       NULL, 'f' },
    { NULL,                0,                 NULL, 0   }
};


int
main(int argc, char **argv)
{
    int i, ch, size;
    int x_count = 8, y_count = 8, z_count = 8;
    int radix = 4, timing_set = 2, noise_runs = 0;
    int coalesce = 1, enable_putv = 0;
    char *noise_interval = NULL, *noise_duration = NULL;
    char *msg_rate = "5MHz";
    int latency = 500;
    int msg_size = 1 * 1024 * 1024;
    int chunk_size = 16 * 1024;
    char *application = NULL;
    char * nic_link_latency = "150ns";
    FILE *output = stdout;

    int argc_org = argc;
    char **argv_org = argv;
    int ranks = 1;
    int new_format = 1;
    
    while ((ch = getopt_long(argc, argv, "hx:y:z:r:", longopts, NULL)) != -1) {
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
        case 'r':
            radix = atoi(optarg);
            break;
        case 't':
            timing_set = atoi(optarg);
            break;
        case 'n':
            noise_runs = atoi(optarg);
            break;
        case 'i':
            noise_interval = optarg;
            break;
        case 'd':
            noise_duration = optarg;
            break;
        case 'l':
            latency = atoi(optarg);
            break;
        case 'q':
            msg_size = atoi(optarg);
            break;
        case 'w':
            chunk_size = atoi(optarg);
            break;
        case 'm':
            msg_rate = optarg;
            break;
        case 'o':
            output = fopen(optarg, "w");
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case 'a':
            application = optarg;
            break;
        case 'c':
            coalesce = atoi(optarg);
            break;
        case 'p':
            enable_putv = atoi(optarg);
            break;
        case 'k':
            ranks = atoi(optarg);
            break;
        case 'f':
            new_format = 1;
            break;
        default:
            print_usage(argv[0]);
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    /* backward compatibility */
    if (argc == 3 || argc == 4) {
        x_count = atoi(argv[0]);
        y_count = atoi(argv[1]);
        z_count = atoi(argv[2]);
        if (argc == 4) radix = atoi(argv[3]);
    }

    if (0 != noise_runs && (NULL == noise_interval || NULL == noise_duration)) {
        print_usage(argv[0]);
        exit(1);
    }

    if (NULL == application) {
        print_usage(argv[0]);
        exit(1);
    }

    /* clean up so SDL file looks nice */
    if (NULL == noise_interval) noise_interval = "1kHz";
    if (NULL == noise_duration) noise_duration = "25us";

    if ( timing_set == 1 ) nic_link_latency = "100ns";
    if ( timing_set == 2 ) nic_link_latency = "200ns";
    if ( timing_set == 3 ) nic_link_latency = "250ns";
    
    size = x_count * y_count * z_count;

    if ( new_format ) fprintf(output, "<sdl version=\"2.0\"/>\n");
    else fprintf(output, "<?xml version=\"1.0\"?>\n");
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
    if ( ranks > 1 || new_format ) fprintf(output, "    partitioner=self\n");
    fprintf(output, "</config>\n");
    fprintf(output, "\n");
    char* s_indent = "    ";
    char* s_noindent = "";

    char* indent = s_noindent;
    if ( new_format ) {
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
    fprintf(output, "%s<credits> 30 </credits>\n",indent);
    fprintf(output, "%s<ports> 7 </ports>\n",indent);
    fprintf(output, "%s<vcs> 4 </vcs>\n",indent);
    fprintf(output, "%s<no_nodes> %d </no_nodes>\n",indent,x_count*y_count);
    fprintf(output, "%s<grid_size> %d </grid_size>\n",indent,x_count);
    fprintf(output, "%s</rtr_params>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<cpu_params>\n",indent);
    fprintf(output, "%s    <radix> %d </radix>\n",indent,radix);
    fprintf(output, "%s    <timing_set> %d </timing_set>\n",indent,timing_set);
    fprintf(output, "%s    <nodes> %d </nodes>\n",indent,size);
    fprintf(output, "%s    <msgrate> %s </msgrate>\n",indent, msg_rate);
    fprintf(output, "%s    <xDimSize> %d </xDimSize>\n",indent,x_count);
    fprintf(output, "%s    <yDimSize> %d </yDimSize>\n",indent,y_count);
    fprintf(output, "%s    <zDimSize> %d </zDimSize>\n",indent,z_count);
    fprintf(output, "%s    <noiseRuns> %d </noiseRuns>\n",indent, noise_runs);
    fprintf(output, "%s    <noiseInterval> %s </noiseInterval>\n",indent, noise_interval);
    fprintf(output, "%s    <noiseDuration> %s </noiseDuration>\n",indent, noise_duration);
    fprintf(output, "%s    <application> %s </application>\n",indent, application);
    fprintf(output, "%s    <latency> %d </latency>\n",indent, latency);    
    fprintf(output, "%s    <msg_size> %d </msg_size>\n",indent, msg_size);    
    fprintf(output, "%s    <chunk_size> %d </chunk_size>\n",indent, chunk_size);    
    fprintf(output, "%s    <coalesce> %d </coalesce>\n",indent, coalesce);
    fprintf(output, "%s    <enable_putv> %d </enable_putv>\n",indent, enable_putv);
    fprintf(output, "%s</cpu_params>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params1>\n",indent);
    fprintf(output, "%s    <clock>500Mhz</clock>\n",indent);
    fprintf(output, "%s    <timing_set> %d </timing_set>\n",indent,timing_set);
    fprintf(output, "%s</nic_params1>\n",indent);
    fprintf(output, "\n");
    fprintf(output, "%s<nic_params2>\n",indent);
    fprintf(output, "%s    <info>no</info>\n",indent);
    fprintf(output, "%s    <debug>no</debug>\n",indent);
    fprintf(output, "%s    <dummyDebug> no </dummyDebug>\n",indent);
    fprintf(output, "%s    <latency> %d </latency>\n",indent, latency);
    fprintf(output, "%s</nic_params2>\n",indent);
    fprintf(output, "\n");

    if ( new_format ) {
        fprintf(output, "</param_include>\n");
        fprintf(output, "\n");
    }

    fprintf(output, "<sst>\n");

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

        if ( new_format ) {
            fprintf(output, "    <component name=%d.cpu type=portals4_sm.trig_cpu rank=%d >\n",i,rank);
            fprintf(output, "        <params include=cpu_params>\n");
            fprintf(output, "            <id> %d </id>\n",i);
            fprintf(output, "        </params>\n");
            fprintf(output, "        <link name=%d.cpu2nic port=nic latency=$nic_link_lat/>\n",i);
            fprintf(output, "    </component>\n");
            fprintf(output, "\n");
            fprintf(output, "    <component name=%d.nic type=iris.trig_nic rank=%d >\n",i,rank);
            fprintf(output, "        <params include=nic_params1,nic_params2>\n");
            fprintf(output, "            <id> %d </id>\n",i);
            fprintf(output, "        </params>\n");
            fprintf(output, "        <link name=%d.cpu2nic port=cpu latency=$nic_link_lat/>\n",i);
            fprintf(output, "        <link name=%d.nic2rtr port=rtr latency=$nic_link_lat/>\n",i);
            fprintf(output, "    </component>\n");
            fprintf(output, "\n");
            fprintf(output, "    <component name=%d.rtr type=iris.router rank=%d >\n",i,rank);
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
        }
        else {
            if ( ranks > 1 ) {
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
            fprintf(output, "        <iris.trig_nic>\n");
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
            fprintf(output, "        </trig_nic.trig_nic>\n");
            fprintf(output, "    </component>\n");
            fprintf(output, "\n");
            /*  XML for dummy component
                fprintf(output, "    <component id=\"%d.rtr\" rank=%d >\n",i,rank);
                fprintf(output, "        <simpleComponent.simpleComponent>\n");
                fprintf(output, "            <params include=rtr_params>\n");
                fprintf(output, "                <id> %d </id>\n",i);
                fprintf(output, "            </params>\n");
                fprintf(output, "            <links>\n");
                fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
                fprintf(output, "                    <params include=nicLink>\n");
                fprintf(output, "                        <name> nic </name>\n");
                fprintf(output, "                    </params>\n");
                fprintf(output, "                </link>\n");
                fprintf(output, "            </links>\n");
                fprintf(output, "        </simpleComponent.simpleComponent>\n");
                fprintf(output, "    </component>\n");
             *  */
            /* XML for the SS_router 
             * */
            fprintf(output, "    <component id=\"%d.rtr\" rank=%d >\n",i,rank);
            fprintf(output, "        <iris.router>\n");
            fprintf(output, "            <params include=rtr_params>\n");
            fprintf(output, "                <id> %d </id>\n",i);
            fprintf(output, "                <node_id> %d </node_id>\n",i);
            fprintf(output, "            </params>\n");
            fprintf(output, "            <links>\n");

            fprintf(output, "                <link id=\"%d.nic2rtr\">\n",i);
            fprintf(output, "                    <params include=nicLink>\n");
            fprintf(output, "                        <name> nic </name>\n");
            fprintf(output, "                    </params>\n");
            fprintf(output, "                </link>\n");

            if ( x_count > 1 ) {
                fprintf(output, "                <link id=\"xr2r.%d.%d.%d\">\n",y,z,(x+1)%x_count);
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

            if ( y_count > 1 ) {
                fprintf(output, "                <link id=\"yr2r.%d.%d.%d\">\n",x,z,(y+1)%y_count);
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
            /* 
               if ( z_count > 1 ) {
               fprintf(output, "                <link id=\"zr2r.%d.%d.%d\">\n",x,y,(z+1)%z_count);
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
             * */

            fprintf(output, "            </links>\n");
            fprintf(output, "        </iris.router>\n");
            fprintf(output, "    </component>\n");
        }
        fprintf(output, "\n");
        fprintf(output, "\n");
    }
    fprintf(output, "</sst>\n");

    return 0;
}
