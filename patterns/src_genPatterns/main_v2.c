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

/*
**
** Rolf Riesen, July 2010, Sandia National Laboratories
**
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "main.h"
#include "sst_gen_v2.h"
#include "machine.h"
#include "farlink.h"
#include "pattern.h"
#include "topo_mesh2D.h"



/*
** Local functions
*/
static void usage(char *argv[]);



/*
** Long options
*/
static struct option long_options[]=   {
    /* name, has arg, flag, val */
    {"help", 0, NULL, 'h'},
    {"verbose", 0, NULL, 'v'},
    {"sstfilename", 1, NULL, 'o'},
    {"machine", 1, NULL, 'm'},
    {"farlinks", 1, NULL, 'f'},
    {"pattern", 1, NULL, 'p'},
    {"IO_nodes", 1, NULL, 'i'},
    {"partition", 1, NULL, 1002},
    {"power", 1, NULL, 1003},
    {NULL, 0, NULL, 0}
};



/*
** Main
*/
int
main(int argc, char *argv[])
{

int option_index= 0;
int ch, error;
int verbose;
char *sstFname;
char *machineFname;
char *farlinkFname;
char *patternFname;
FILE *fp_sst;
FILE *fp_machine;
FILE *fp_farlink;
FILE *fp_pattern;

int IO_nodes;		/* Should be divisible by the number of nodes */
char *power_model;	/* Which, if any, power model to use */
int wormhole;
pwr_method_t power_method;

int nvram_lat;		/* In ns */
int nvram_read_bw;	/* In bytes per second */
int nvram_write_bw;	/* In bytes per second */
int ssd_read_bw;	/* In bytes per second */
int ssd_write_bw;	/* In bytes per second */

int partition;



    /* Defaults */
    error= FALSE;
    verbose= 0;
    sstFname= "";
    machineFname= "";
    farlinkFname= "";
    patternFname= "";

    /* Assume a SATA3 drive (Crucial C200) "somehow" connected to an I/O network */
    ssd_read_bw= 200000000;
    ssd_write_bw= 350000000;

    /* Let's say we have a Fusion-io PCIe card on each node */
    nvram_read_bw= 490000000;
    nvram_write_bw= 700000000;
    nvram_lat= 50000;

    IO_nodes= 1;

    power_model= "none";
    power_method= pwrNone;

    partition= 0;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "vo:f:hp:i:m:", long_options, &option_index);
	if (ch == -1)   {
	    break;
	}

	switch (ch)   {
	    case 'i':
		IO_nodes= strtol(optarg, (char **)NULL, 0);
		if (IO_nodes < 1)   {
		    fprintf(stderr, "Need at least one I/O node\n");
		    error= TRUE;
		}
		break;
	    case 'm':
		machineFname= optarg;
		break;
	    case 'o':
		sstFname= optarg;
		break;
	    case 'p':
		patternFname= optarg;
		break;
	    case 'f':
		farlinkFname= optarg;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'h':
		error= TRUE;
		break;
	    case 1002:
		partition= strtol(optarg, (char **)NULL, 0);
		if (partition < 1)   {
		    fprintf(stderr, "Need to specify for how many ranks to partition for: --partition num\n");
		    error= TRUE;
		}
		break;
	    case 1003:
		power_model= optarg;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (optind < argc)   {
	error= TRUE;
	fprintf(stderr, "No additional arguments expected!\n");
    }

    /* Open the machine description file for input */
    if (strcmp(machineFname, "") == 0)   {
	fp_machine= NULL;
	error= TRUE;
	fprintf(stderr, "Machine file must be specified with -m option!\n");
    } else   {
	fp_machine= fopen(machineFname, "r");
	if (fp_machine == NULL)   {
	    fprintf(stderr, "Could not open the machine input file \"%s\": %s\n",
		machineFname, strerror(errno));
	    error= TRUE;
	}
    }

    /* Open the pattern description file for input */
    if (strcmp(patternFname, "") == 0)   {
	fp_pattern= NULL;
	error= TRUE;
	fprintf(stderr, "Pattern file must be specified with -p option!\n");
    } else   {
	fp_pattern= fopen(patternFname, "r");
	if (fp_pattern == NULL)   {
	    fprintf(stderr, "Could not open the pattern input file \"%s\": %s\n",
		patternFname, strerror(errno));
	    error= TRUE;
	}
    }

    /* Open the (optional) far link configuration file for input */
    if (strcmp(farlinkFname, "") == 0)   {
	fp_farlink= NULL;
    } else   {
	fp_farlink= fopen(farlinkFname, "r");
	if (fp_farlink == NULL)   {
	    fprintf(stderr, "Could not open the far link configuration file \"%s\": %s\n",
		farlinkFname, strerror(errno));
	    error= TRUE;
	}
    }

    fp_sst= NULL;
    if (!error)   {
	/* Open the SST xml file for output */
	printf("# *** Writing output to \"%s\"\n", sstFname);
	if (strcmp(sstFname, "") == 0)   {
	    error= TRUE;
	    fprintf(stderr, "Output file must be specified with -o option!\n");
	} else if (strcmp(sstFname, "-") == 0)   {
	    fp_sst= stdout;
	} else   {
	    fp_sst= fopen(sstFname, "w");
	    if (fp_sst == NULL)   {
		fprintf(stderr, "Could not open the SST xml output file \"%s\": %s\n",
		    sstFname, strerror(errno));
		error= TRUE;
	    }
	}
    }


    /* Read the machine file */
    if (!error)   {
	printf("# *** Reading machine parameters from \"%s\"\n", machineFname);
	if (read_machine_file(fp_machine, verbose) == FALSE)   {
	    error= TRUE;
	}
    }

    /* Read the farlink file */
    if (!error && fp_farlink)   {
	printf("# *** Reading far link configuration from \"%s\"\n", farlinkFname);
	if (read_farlink_file(fp_farlink, verbose) == FALSE)   {
	    error= TRUE;
	}
    }

    /* Read the pattern file */
    if (!error)   {
	printf("# *** Reading pattern parameters from \"%s\"\n", patternFname);
	if (read_pattern_file(fp_pattern, verbose) == FALSE)   {
	    error= TRUE;
	}
    }

    /* A node is the endpoint of a network router */
    if (!error)   {
	if (num_nodes() % IO_nodes)   {
	    fprintf(stderr, "Number of nodes (%d) should be evenly divisible by number "
		"of I/O nodes (%d)!\n", num_nodes(), IO_nodes);
	    error= TRUE;
	} else   {
	    printf("# *** Number of I/O nodes is %d, one per %d compute nodes\n", IO_nodes,
		num_nodes() / IO_nodes);
	}
    }

    if (!error)   {
	disp_machine_params();
	disp_pattern_params();
	disp_farlink_params();
	if (partition)   {
	    printf("# *** Creating a self-partition for %d ranks\n", partition);
	}
    }

    if (!error)   {
	if ((strcasestr("none", power_model) == NULL) &&
	    (strcasestr("McPAT", power_model) == NULL) &&
	    (strcasestr("ORION", power_model) == NULL))   {
	    error= TRUE;
	    fprintf(stderr, "Unknown power model!\n");
	    fprintf(stderr, "Must be one of \"McPAT\" or \"ORION\"\n");
	} else   {
	    if (strcasestr("none", power_model) != NULL)   {
		printf("# *** Power model is \"none\"\n");
		power_method= pwrNone;
	    } else if (strcasestr("McPAT", power_model) != NULL)   {
		printf("# *** Power model is \"McPAT\"\n");
		power_method= pwrMcPAT;
	    } else if (strcasestr("ORION", power_model) != NULL)   {
		printf("# *** Power model is \"ORION\"\n");
		power_method= pwrORION;
	    }

	}
    }

    if (error)   {
	usage(argv);
	exit(1);
    }


    if (partition < 1)   {
	/* Set this to the default */
	partition= 1;
    }
    GenMesh3D(IO_nodes, partition);


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst, partition);
    sst_variables(fp_sst, IOLinkLatency());
    sst_param_start(fp_sst); /* start the parameter_include section */
    sst_gen_param_start(fp_sst, 0);
    sst_gen_param_entries(fp_sst);
    sst_pwr_param_entries(fp_sst, power_method);
    sst_gen_param_end(fp_sst);
    sst_nvram_param_entries(fp_sst, nvram_read_bw, nvram_write_bw, ssd_read_bw, ssd_write_bw);

    /* We assume the router bandwidth is the same as the link bandwidth */
    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NETWORK, 6 + num_router_nodes(),
	num_router_cores(), wormhole, power_method);
    sst_router_param_end(fp_sst, RNAME_NETWORK);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NoC, 6 + num_router_cores(),
	num_router_cores(), wormhole, power_method);
    sst_router_param_end(fp_sst, RNAME_NoC);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NET_ACCESS, 1 + num_cores(),
	num_router_cores(), wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NET_ACCESS);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NVRAM, 1 + num_cores(),
	num_router_cores(), wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NVRAM);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_STORAGE, 1 + num_cores(),
	num_router_cores(), wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_STORAGE);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_IO, 1 + num_nodes() / IO_nodes,
	num_router_cores(), wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_IO);
    sst_param_end(fp_sst); 

    sst_body_start(fp_sst);
    sst_pwr_component(fp_sst, power_method);
    sst_pattern_generators(fp_sst);
    sst_nvram(fp_sst);
    sst_routers(fp_sst, nvram_lat, power_method);
    sst_body_end(fp_sst);
    sst_footer(fp_sst);

    if (fp_sst)   {
	fclose(fp_sst);
    }

    if (fp_machine)   {
	fclose(fp_machine);
    }

    if (fp_pattern)   {
	fclose(fp_pattern);
    }

    if (fp_farlink)   {
	fclose(fp_farlink);
    }

    return 0;

}  /* end of main() */



static void
usage(char *argv[])
{

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s -o out -m machine -p pname [-f farlink] [--power model] [-i IO] [--partition num] [-h]\n",
	argv[0]);
    fprintf(stderr, "   --sstfilename, -o     Name of the SST xml output file\n");
    fprintf(stderr, "   --machine, -m         Name of machine description file\n");
    fprintf(stderr, "   --farlink, -f         Name of far link configuration file\n");
    fprintf(stderr, "   --IO_nodes, -i        Number of I/O nodes (Default 1)\n");
    fprintf(stderr, "   --help, -h            Print this message\n");
    fprintf(stderr, "   --pattern, -p         Name of pattern description file\n");
    fprintf(stderr, "   --power <model>       Enable power modeling using McPAT or ORION\n");
    fprintf(stderr, "   --partition num       Create a self partitioned XML file\n");

}  /* end of usage() */
