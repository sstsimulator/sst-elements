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


typedef struct params {
    int ndim;
    int *dims;
    int peers;
    char link_bw[64];
    char link_lat[64];
} params_t;


static params_t params;

int collect_parameters(FILE *f)
{
    fprintf(stderr, "Enter the number of dimensions:  ");
    fscanf(f, "%d", &params.ndim);
    params.dims = (int*)calloc(params.ndim, sizeof(int));
    params.peers = 1;
    for ( int i = 0 ; i < params.ndim ; i++ ) {
        fprintf(stderr, "Enter size of dimension %d:  ", i);
        fscanf(f, "%d", &params.dims[i]);
        params.peers *= params.dims[i];
    }
    fprintf(stderr, "Enter Link Bandwidth  (ie:  1GHz):  ");
    fscanf(f, "%s", params.link_bw);
    fprintf(stderr, "Enter Link Latency  (ie:  10ns):  ");
    fscanf(f, "%s", params.link_lat);

    return 0;
}


void idToLoc(int id, int *loc)
{
	for ( int i = params.ndim - 1; i > 0; i-- ) {
		int div = 1;
		for ( int j = 0; j < i; j++ ) {
			div *= params.dims[j];
		}
		int value = (id / div);
		loc[i] = value;
		id -= (value * div);
	}
	loc[0] = id;
}



void format_id(int id, int port, int isSrc, char **P)
{
    static int *loc = NULL;
    if ( loc == NULL ) loc = (int*)calloc(params.ndim, sizeof(int));
    idToLoc(id, loc);

    int posDirection = !( port & 1 );
    int dim = port / 2;

    if ( posDirection ) {
        if ( !isSrc ) {
            loc[dim] = (loc[dim] + 1) % params.dims[dim];
        }
    } else {
        if ( isSrc ) {
            loc[dim] = (loc[dim] - 1 + params.dims[dim]) % params.dims[dim];
        }
    }

    char *p = *P;
    p += sprintf(p, "%d", loc[0]);
    for ( int i = 1 ; i < params.ndim ; i++ )
        p += sprintf(p, "x%d", loc[i]);

    *P = p;
}


char *portName(int id, int port)
{
    static char pName[256];
    memset(pName, '\0', 256);
    char *p = pName;
    *p++ = 'l';
    *p++ = 'i';
    *p++ = 'n';
    *p++ = 'k';
    *p++ = '_';
    format_id(id, port, 1, &p);
    *p++ = '_';
    format_id(id, port, 0, &p);

    return pName;
}

char *routerName(int id)
{
    static char rName[256];
    memset(rName, '\0', 256);
    char *p = rName;
    *p++ = 'r';
    *p++ = 't';
    *p++ = 'r';
    *p++ = '.';
    format_id(id, 0, 0, &p);

    return rName;
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
    fprintf(output, "    <num_ports> %d </num_ports>\n", 2*params.ndim +1);
    fprintf(output, "    <num_vcs> 2 </num_vcs>\n");
    fprintf(output, "    <link_bw> %s </link_bw>\n", params.link_bw);
    fprintf(output, "    <topology> torus </topology>\n");

    fprintf(output, "    <torus:shape> %d",  params.dims[0]);
    for ( int i = 1 ; i < params.ndim ; i++ )
        fprintf(output, "x%d", params.dims[i]);
    fprintf(output, " </torus:shape>\n");

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


    for ( int i = 0 ; i < params.peers ; i++ ) {
        fprintf(output, "  <component name=%s type=merlin.hr_router>\n", routerName(i));
        fprintf(output, "    <params include=rtr_params>\n");
        fprintf(output, "      <id> %d </id>\n", i);
        fprintf(output, "    </params>\n");
        for ( int p = 0 ; p < (2*params.ndim) ; p++ ) {
            fprintf(output, "    <link name=%s port=port%d latency=%s />\n",
                    portName(i, p), p, params.link_lat);
        }
        fprintf(output, "    <link name=nic%d port=port%d latency=%s />\n",
                i, 2*params.ndim, params.link_lat);
        fprintf(output, "  </component>\n");
        fprintf(output, "\n");
        fprintf(output, "  <component name=%d.nic type=merlin.test_nic>\n", i);
        fprintf(output, "    <params include=nic_params>\n");
        fprintf(output, "      <id> %d </id>\n", i);
        fprintf(output, "    </params>\n");
        fprintf(output, "    <link name=nic%d port=rtr latency=%s />\n",
                i, params.link_lat);
        fprintf(output, "  </component>\n");
        fprintf(output, "\n");
    }

    fprintf(output, "\n");
    fprintf(output, "</sst>\n");

    return 0;
}
