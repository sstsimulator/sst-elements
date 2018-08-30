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





typedef struct params {
    uint32_t N;  /* # of hosts */
    uint32_t p;  /* # of hosts / router */
    uint32_t a;  /* # of routers / group */
    uint32_t k;  /* Router Radix */
    uint32_t h;  /* # of ports / router to connect to other groups */
    uint32_t g;  /* # of Groups */
    char link_bw[64];
    char link_lat[64];
    char xbar_bw[64];
} params_t;

static params_t params;


int collect_parameters(FILE *f)
{
    fprintf(stderr, "Enter the switch size/radix:  ");
    fscanf(f, "%u", &params.k);

    fprintf(stderr, "Enter the number of ports per router for Hosts:  ");
    fscanf(f, "%u", &params.p);

    fprintf(stderr, "Enter the number of ports per router for connecting to other groups:  ");
    fscanf(f, "%u", &params.h);

    fprintf(stderr, "Enter the number of routers per group:  ");
    fscanf(f, "%u", &params.a);

    fprintf(stderr, "Enter the number of groups:  ");
    fscanf(f, "%u", &params.g);

    params.N = params.p * params.a * params.g;

    if ( (params.a-1 + params.p + params.h) > params.k ) {
        fprintf(stderr, "ERROR:  # of ports per router is only %u.\n", params.k);
        return 1;
    }

    if ( (params.a < 2*params.h) || (2*params.p < 2*params.h) ) {
        fprintf(stderr, "WARN:  Network balance is out of wack.\n");
    }

    fprintf(stderr, "Enter Link Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.link_bw);
    fprintf(stderr, "Enter Link Latency  (ie:  10ns):  ");
    fscanf(f, "%s", params.link_lat);
    fprintf(stderr, "Enter Xbar Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.xbar_bw);

    return 0;
}



int main(int argc, char **argv)
{
    FILE *output = stdout;
    FILE *input = stdin;

    if ( collect_parameters(input) ) {
        fprintf(stderr, "Parameter collection failed.\n");
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
    fprintf(output, "    <num_vcs> 3 </num_vcs>\n"); /* 3 VCs are sufficient */
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "    <xbar_bw> %s </xbar_bw>\n", params.xbar_bw);
    fprintf(output, "    <topology> dragonfly </topology>\n");
    fprintf(output, "    <dragonfly:hosts_per_router> %u </dragonfly:hosts_per_router>\n", params.p);
    fprintf(output, "    <dragonfly:routers_per_group> %u </dragonfly:routers_per_group>\n", params.a);
    fprintf(output, "    <dragonfly:intergroup_per_router> %u </dragonfly:intergroup_per_router>\n", params.h);
    fprintf(output, "    <dragonfly:num_groups> %u </dragonfly:num_groups>\n", params.g);
    fprintf(output, "  </rtr_params>\n");
    fprintf(output, "\n");
    fprintf(output, "  <nic_params>\n");
    fprintf(output, "    <topology> dragonfly </topology>\n");
    fprintf(output, "    <num_peers> %d </num_peers>\n", params.N);
    fprintf(output, "    <num_vcs> 3 </num_vcs>\n"); /* 3 VCs are sufficient */
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "  </nic_params>\n");
    fprintf(output, "\n");
    fprintf(output, "</param_include>\n");
    fprintf(output, "\n");

    fprintf(output, "<sst>\n\n");



    uint32_t router_num = 0;
    uint32_t nic_num = 0;
    for ( uint32_t g = 0 ; g < params.g ; g++ ) {
        fprintf(output, "  <!-- GROUP %u -->\n", g);
        uint32_t tgt_grp = 0;

        for ( uint32_t r = 0 ; r < params.a ; r++ ) {
            fprintf(output, "  <!-- GROUP %u, ROUTER %u -->\n", g, r);

            fprintf(output, "  <component name=rtr:G%uR%u type=merlin.hr_router>\n", g, r);
            fprintf(output, "    <params include=rtr_params>\n");
            fprintf(output, "      <id> %d </id>\n", router_num);
            fprintf(output, "    </params>\n");

            uint32_t port = 0;
            for ( uint32_t p = 0 ; p < params.p ; p++ ) {
                fprintf(output, "    <link name=link:g%ur%uh%u port=port%u latency=%s />\n",
                        g, r, p, port++, params.link_lat);

            }
            for ( uint32_t p = 0 ; p < params.a ; p++ ) {
                if ( p != r ) {
                    uint32_t src = (p < r) ? p : r;
                    uint32_t dst = (p < r) ? r : p;
                    fprintf(output, "    <link name=link:g%ur%ur%u port=port%u latency=%s />\n",
                            g, src, dst, port++, params.link_lat);
                }
            }
            for ( uint32_t p = 0 ; p < params.h ; p++ ) {
                if ( (tgt_grp%params.g) == g ) {
                    tgt_grp++;
                }
                uint32_t src_g = (g < (tgt_grp%params.g)) ? g : (tgt_grp%params.g);
                uint32_t dst_g = (g < (tgt_grp%params.g)) ? (tgt_grp%params.g) : g;

                fprintf(output, "    <link name=link:g%ug%u:%u port=port%u latency=%s />\n",
                        src_g, dst_g, tgt_grp / params.g, port++, params.link_lat);
                tgt_grp++;
            }

            fprintf(output, "  </component>\n");
            fprintf(output, "\n");

            for ( uint32_t h = 0 ; h < params.p ; h++ ) {
                fprintf(output, "  <!-- GROUP %u, ROUTER %u, HOST %u -->\n", g, r, h);
                fprintf(output, "  <component name=nic:G%uR%uH%u type=merlin.test_nic>\n", g, r, h);
                fprintf(output, "    <params include=nic_params>\n");
                fprintf(output, "      <!-- node number %d -->\n", nic_num);
                fprintf(output, "      <id> %d </id>\n", nic_num);
                fprintf(output, "    </params>\n");
                fprintf(output, "    <link name=link:g%ur%uh%u port=rtr latency=%s />\n",
                        g, r, h, params.link_lat);
                fprintf(output, "  </component>\n");

                nic_num++;
            }
            fprintf(output, "\n");
            router_num++;
        }
        fprintf(output, "\n");
    }



    fprintf(output, "\n</sst>\n");


    return 0;
}
