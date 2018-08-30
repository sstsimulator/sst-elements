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
#include <inttypes.h>

typedef union {
    uint8_t  x[4];
    int32_t  s;
    uint32_t u;
} addr;


typedef struct params {
    int k;  // Router size, # of pods
    int numnodes;

    int peers;
    char link_bw[64];
    char link_lat[64];
    char xbar_bw[64];
} params_t;


static params_t params;

int collect_parameters(FILE *f)
{
    fprintf(stderr, "Enter the switch size:  ");
    fscanf(f, "%d", &params.k);
    if ( params.k > 255 ) {
        fprintf(stderr, "Sorry, this sdl generator doesn't support so large a switch\n");
        return 1;
    }

    fprintf(stderr, "Enter number of nodes attached to each edge router:  ");
    fscanf(f, "%d", &params.numnodes);

    if ( params.numnodes <0 || params.numnodes > (params.k/2) ) {
        fprintf(stderr, "ERROR: number of nodes per edge router must be less than or equal to half the switch size.\n");
        return 1;
    }

    fprintf(stderr, "Enter Link Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.link_bw);
    fprintf(stderr, "Enter Link Latency  (ie:  10ns):  ");
    fscanf(f, "%s", params.link_lat);
    fprintf(stderr, "Enter Xbar Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.xbar_bw);


    // k Pods * (k/2) Edge Switches * numnodes  [numnodes <= k/2]
    params.peers = params.k * (params.k/2) * params.numnodes;

    return 0;
}


static inline void formatIP(addr myip, char *str) {
    sprintf(str, "%u.%u.%u.%u",
            myip.x[0], myip.x[1], myip.x[2], myip.x[3]);
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
    fprintf(output, "    <num_ports> %d </num_ports>\n", params.k);
    fprintf(output, "    <num_vcs> 2 </num_vcs>\n");
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "    <xbar_bw> %s </xbar_bw>\n", params.xbar_bw);
    fprintf(output, "    <topology> fattree </topology>\n");
    fprintf(output, "    <fattree:loading> %d </fattree:loading>\n",  params.numnodes);
    fprintf(output, "  </rtr_params>\n");
    fprintf(output, "\n");
    fprintf(output, "  <nic_params>\n");
    fprintf(output, "    <topology> fattree </topology>\n");
    fprintf(output, "    <fattree:loading> %d </fattree:loading>\n", params.numnodes);
    fprintf(output, "    <fattree:radix> %d </fattree:radix>\n", params.k);
    fprintf(output, "    <num_peers> %d </num_peers>\n", params.peers);
    fprintf(output, "    <num_vcs> 2 </num_vcs>\n");
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "  </nic_params>\n");
    fprintf(output, "\n");
    fprintf(output, "</param_include>\n");
    fprintf(output, "\n");

    fprintf(output, "<sst>\n");


    addr myip;
    myip.x[0] = 10;
    myip.x[1] = params.k;
    myip.x[2] = 1;
    myip.x[3] = 1;
    char myip_str[16] = {0};

    int router_num = 0;
    fprintf(output, "  <!-- CORE ROUTERS -->\n");
    int num_core = (params.k/2) * (params.k/2);
    for ( int i = 0 ; i < num_core ; i++ ) {
        myip.x[2] = 1 + i/(params.k/2);
        myip.x[3] = 1 + i%(params.k/2);
        formatIP(myip, myip_str);
        fprintf(output, "  <component name=core:%s type=merlin.hr_router>\n", myip_str);
        fprintf(output, "    <params include=rtr_params>\n");
        fprintf(output, "      <id> %d </id>\n", router_num++);
        fprintf(output, "      <fattree:addr> %d </fattree:addr>\n", myip.s);
        fprintf(output, "      <fattree:level> 3 </fattree:level>\n");
        fprintf(output, "    </params>\n");

        for ( int l = 0 ; l < params.k ; l++ ) {
            fprintf(output, "    <link name=link:pod%d_core%d port=port%d latency=%s />\n",
                    l, i, l, params.link_lat);
        }

        fprintf(output, "  </component>\n");
        fprintf(output, "\n");

    }

    for ( int pod = 0 ; pod < params.k ; pod++ ) {
        fprintf(output, "\n\n\n");
        fprintf(output, "  <!-- POD %d -->\n", pod);
        myip.x[1] = pod;
        fprintf(output, "  <!-- AGGREGATION ROUTERS -->\n");
        for ( int r = 0 ; r < params.k/2 ; r++ ) {
            int router = params.k/2 + r;
            myip.x[2] = router;
            myip.x[3] = 1;
            formatIP(myip, myip_str);
            fprintf(output, "  <component name=agg:%s type=merlin.hr_router>\n", myip_str);
            fprintf(output, "    <params include=rtr_params>\n");
            fprintf(output, "      <id> %d </id>\n", router_num++);
            fprintf(output, "      <fattree:addr> %d </fattree:addr>\n", myip.s);
            fprintf(output, "      <fattree:level> 2 </fattree:level>\n");
            fprintf(output, "    </params>\n");

            for ( int l = 0 ; l < params.k/2 ; l++ ) {
                fprintf(output, "    <link name=link:pod%d_aggr%d_edge%d port=port%d latency=%s />\n",
                        pod, r, l, l, params.link_lat);
            }
            for ( int l = 0 ; l < params.k/2 ; l++ ) {
                int core = (params.k/2) * (r) + l;
                fprintf(output, "    <link name=link:pod%d_core%d port=port%d latency=%s />\n",
                        pod, core, l + params.k/2, params.link_lat);
            }
            fprintf(output, "  </component>\n");
            fprintf(output, "\n");

        }

        fprintf(output, "\n");
        fprintf(output, "  <!-- EDGE ROUTERS -->\n");
        for ( int r = 0 ; r < params.k/2 ; r++ ) {
            myip.x[2] = r;
            myip.x[3] = 1;
            formatIP(myip, myip_str);
            fprintf(output, "  <component name=edge:%s type=merlin.hr_router>\n", myip_str);
            fprintf(output, "    <params include=rtr_params>\n");
            fprintf(output, "      <id> %d </id>\n", router_num++);
            fprintf(output, "      <fattree:addr> %d </fattree:addr>\n", myip.s);
            fprintf(output, "      <fattree:level> 1 </fattree:level>\n");
            fprintf(output, "    </params>\n");

            for ( int l = 0 ; l < params.numnodes ; l++ ) {
                int node_id = pod * (params.k/2) * params.numnodes;
                node_id += r * params.numnodes;
                node_id += l;
                fprintf(output, "    <link name=link:pod%d_edge%d_node%d port=port%d latency=%s />\n",
                        pod, r, node_id, l, params.link_lat);
            }

            for ( int l = 0 ; l < params.k/2 ; l++ ) {
                fprintf(output, "    <link name=link:pod%d_aggr%d_edge%d port=port%d latency=%s />\n",
                        pod, l, r, l+params.k/2, params.link_lat);
            }

            fprintf(output, "  </component>\n");
            fprintf(output, "\n");
            fprintf(output, "  <!-- NODES -->\n");
            for ( int n = 0 ; n < params.numnodes ; n++ ) {
                int node_id = pod * (params.k/2) * params.numnodes;
                node_id += r * params.numnodes;
                node_id += n;
                myip.x[3] = n+2;
                formatIP(myip, myip_str);
                fprintf(output, "  <component name=nic:%s type=merlin.test_nic>\n", myip_str);
                fprintf(output, "    <params include=nic_params>\n");
                fprintf(output, "      <!-- node number %d -->\n", node_id);
                fprintf(output, "      <id> %d </id>\n", node_id);
                fprintf(output, "      <fattree:addr> %d </fattree:addr>\n", myip.s);
                fprintf(output, "    </params>\n");
                fprintf(output, "    <link name=link:pod%d_edge%d_node%d port=rtr latency=%s />\n",
                        pod, r, node_id, params.link_lat);
                fprintf(output, "  </component>\n");
                fprintf(output, "\n");
            }
        }


    }





    fprintf(output, "\n");
    fprintf(output, "</sst>\n");

    return 0;
}






