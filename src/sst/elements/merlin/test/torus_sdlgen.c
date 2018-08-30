/*
// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct params {
    int ndim;
    int *dims;
    int *dimwidth;
    int numnodes;
    int peers;
    char link_bw[64];
    char link_lat[64];
    char xbar_bw[64];
} params_t;


static params_t params;

int collect_parameters(FILE *f)
{
    fprintf(stderr, "Enter the number of dimensions:  ");
    fscanf(f, "%d", &params.ndim);
    params.dims = (int*)calloc(params.ndim, sizeof(int));
    params.dimwidth = (int*)calloc(params.ndim, sizeof(int));
    params.peers = 1;
    for ( int i = 0 ; i < params.ndim ; i++ ) {
        fprintf(stderr, "Enter size of dimension %d:  ", i);
        fscanf(f, "%d", &params.dims[i]);
        fprintf(stderr, "Enter number of links in dimension %d:  ", i);
        fscanf(f, "%d", &params.dimwidth[i]);
        params.peers *= params.dims[i];
    }
    fprintf(stderr, "Enter number of nodes attached to each router:  ");
    fscanf(f, "%d", &params.numnodes);
    params.peers *= params.numnodes;
    fprintf(stderr, "Enter Link Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.link_bw);
    fprintf(stderr, "Enter Link Latency  (ie:  10ns):  ");
    fscanf(f, "%s", params.link_lat);
    fprintf(stderr, "Enter Xbar Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.xbar_bw);

    return 0;
}


void idToLoc(int router_id, int *loc)
{
	for ( int i = params.ndim - 1; i > 0; i-- ) {
		int div = 1;
		for ( int j = 0; j < i; j++ ) {
			div *= params.dims[j];
		}
		int value = (router_id / div);
		loc[i] = value;
		router_id -= (value * div);
	}
	loc[0] = router_id;
}



void formatLoc(int *dims, char *buf)
{
    char *p = buf;
    p += sprintf(buf, "%d", dims[0]);
    for ( int i = 1 ; i < params.ndim ; i++ ) {
        p += sprintf(p, "x%d", dims[i]);
    }
}


int
main(int argc, char **argv)
{
    char * nic_link_latency = "10ns";
    FILE *output = stdout;
    FILE *input = stdin;

    if ( collect_parameters(input) ) {
        fprintf(stderr, "Parameter collection failed!\n");
        return 1;
    }


    fprintf(output, "<?xml version=\"1.0\"?>\n");
    fprintf(output, "\n<sdl version=\"2.0\"/>\n");
    fprintf(output, "\n");

    fprintf(output, "<config>\n");
    fprintf(output, "    run-mode=both\n");
    fprintf(output, "</config>\n");
    fprintf(output, "\n");

    fprintf(output, "<param_include>\n");

    fprintf(output, "  <rtr_params>\n");
    fprintf(output, "    <debug> 0 </debug>\n");
    int num_rtr_ports = 0;
    for ( int d = 0 ; d < params.ndim ; d++ ) {
        num_rtr_ports += 2 * params.dimwidth[d];
    }
    fprintf(output, "    <num_ports> %d </num_ports>\n", num_rtr_ports + params.numnodes);
    fprintf(output, "    <num_vcs> 2 </num_vcs>\n");
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "    <xbar_bw> %s </xbar_bw>\n", params.xbar_bw);
    fprintf(output, "    <topology> torus </topology>\n");

    fprintf(output, "    <torus:shape> %d",  params.dims[0]);
    for ( int i = 1 ; i < params.ndim ; i++ )
        fprintf(output, "x%d", params.dims[i]);
    fprintf(output, " </torus:shape>\n");

    fprintf(output, "    <torus:width> %d",  params.dimwidth[0]);
    for ( int i = 1 ; i < params.ndim ; i++ )
        fprintf(output, "x%d", params.dimwidth[i]);
    fprintf(output, " </torus:width>\n");

    fprintf(output, "    <torus:local_ports> %d </torus:local_ports>\n", params.numnodes);

    fprintf(output, "  </rtr_params>\n");
    fprintf(output, "\n");
    fprintf(output, "  <nic_params>\n");
    fprintf(output, "    <num_peers> %d </num_peers>\n", params.peers);
    fprintf(output, "    <num_vcs> 2 </num_vcs>\n");
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "  </nic_params>\n");
    fprintf(output, "\n");
    fprintf(output, "</param_include>\n");
    fprintf(output, "\n");

    fprintf(output, "<sst>\n");


    int num_routers = params.peers / params.numnodes;
    int *mydims = (int*)calloc(params.ndim, sizeof(int));
    int *theirdims = (int*)calloc(params.ndim, sizeof(int));

    for ( int i = 0 ; i < num_routers ; i++ ) {
        idToLoc(i, mydims);

        char mylocstr[256], otherlocstr[256];
        formatLoc(mydims, mylocstr);

        fprintf(output, "  <component name=rtr.%s type=merlin.hr_router>\n", mylocstr);
        fprintf(output, "    <params include=rtr_params>\n");
        fprintf(output, "      <id> %d </id>\n", i);
        fprintf(output, "    </params>\n");

        int port = 0;
        for ( int dim = 0 ; dim < params.ndim ; dim++ ) {
            memcpy(theirdims, mydims, sizeof(int) * params.ndim);

            /* Positive direction */
            theirdims[dim] = (mydims[dim] + 1) % params.dims[dim];
            formatLoc(theirdims, otherlocstr);
            for ( int num = 0 ; num < params.dimwidth[dim] ; num++ ) {
                fprintf(output, "    <link name=link.%s:%s:%d port=port%d latency=%s />\n",
                        mylocstr, otherlocstr, num, port++, params.link_lat);
            }

            /* Negative direction */
            theirdims[dim] = ((mydims[dim] - 1) + params.dims[dim]) % params.dims[dim];
            formatLoc(theirdims, otherlocstr);
            for ( int num = 0 ; num < params.dimwidth[dim] ; num++ ) {
                fprintf(output, "    <link name=link.%s:%s:%d port=port%d latency=%s />\n",
                        otherlocstr, mylocstr, num, port++, params.link_lat);
            }
        }

        for ( int n = 0 ; n < params.numnodes ; n++ ) {
            fprintf(output, "    <link name=nic.%d:%d port=port%d latency=%s />\n",
                    i, n, port++, params.link_lat);
        }
        fprintf(output, "  </component>\n");
        fprintf(output, "\n");

        for ( int n = 0 ; n < params.numnodes ; n++ ) {
            int nodeID = params.numnodes*i + n;

            fprintf(output, "  <component name=nic.%s-%d type=merlin.test_nic>\n", mylocstr, n);
            fprintf(output, "    <params include=nic_params>\n");
            fprintf(output, "      <id> %d </id>\n", nodeID);
            fprintf(output, "    </params>\n");
            fprintf(output, "    <link name=nic.%d:%d port=rtr latency=%s />\n",
                    i, n, params.link_lat);
            fprintf(output, "  </component>\n");
            fprintf(output, "\n");
        }
    }

    fprintf(output, "\n");
    fprintf(output, "</sst>\n");

    free(mydims);
    free(theirdims);
    return 0;
}






