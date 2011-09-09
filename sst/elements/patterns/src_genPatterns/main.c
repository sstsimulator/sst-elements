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
#include "sst_gen.h"
#include "machine.h"
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
    {"pattern", 1, NULL, 'p'},
    {"IO_nodes", 1, NULL, 'i'},
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
FILE *fp_sst;
FILE *fp_machine;
char *pattern_name;
uint64_t net_lat;	/* In ns */
uint64_t net_bw;	/* In bytes per second */
uint64_t node_lat;	/* In ns */
uint64_t node_bw;	/* In bytes per second */

int IO_nodes;		/* Should be divisible by the number of nodes */
char *power_model;	/* Which, if any, power model to use */
int wormhole;
pwr_method_t power_method;

int nvram_lat;		/* In ns */
int nvram_read_bw;	/* In bytes per second */
int nvram_write_bw;	/* In bytes per second */
int ssd_read_bw;	/* In bytes per second */
int ssd_write_bw;	/* In bytes per second */



    /* Defaults */
    error= FALSE;
    verbose= 0;
    sstFname= "";
    machineFname= "";
    net_lat= 30;
    net_bw= 1900000000;
    node_lat= 30;
    node_bw= 126000000;

    /* Assume a SATA3 drive (Crucial C200) "somehow" connected to an I/O network */
    ssd_read_bw= 200000000;
    ssd_write_bw= 350000000;

    /* Let's say we have a Fusion-io PCIe card on each node */
    nvram_read_bw= 490000000;
    nvram_write_bw= 700000000;
    nvram_lat= 50000;

    IO_nodes= 1;
    pattern_name= NULL;

    power_model= "none";
    power_method= pwrNone;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "vo:hp:i:m:", long_options, &option_index);
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
		pattern_name= optarg;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'h':
		error= TRUE;
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

    if (pattern_name == NULL)   {
	error= TRUE;
	fprintf(stderr, "Need a pattern name!\n");
    } else   {
	if ((strcasestr("ghost_pattern", pattern_name) == NULL)   &&
	    (strcasestr("pingpong_pattern", pattern_name) == NULL)   &&
	    (strcasestr("msgrate_pattern", pattern_name) == NULL)   &&
	    (strcasestr("allreduce_pattern", pattern_name) == NULL))   {
	    error= TRUE;
	    fprintf(stderr, "Unknown pattern name!\n");
	    fprintf(stderr, "Must be one of \"ghost_pattern\", \"msgrate_pattern\", \"allreduce_pattern\","
		" or \"pingpong_pattern\n");

	} else   {

	    if (strcasestr("ghost_pattern", pattern_name) != NULL)   {
		pattern_name= "ghost_pattern";

	    } else if (strcasestr("msgrate_pattern", pattern_name) != NULL)   {
		pattern_name= "msgrate_pattern";

	    } else if (strcasestr("allreduce_pattern", pattern_name) != NULL)   {
		pattern_name= "allreduce_pattern";

	    } else if (strcasestr("pingpong_pattern", pattern_name) != NULL)   {
		pattern_name= "pingpong_pattern";

	    } else   {
		error= TRUE;
	    }

	    if (!error)   {
		printf("*** Communication pattern is \"%s\"\n", pattern_name);
	    }
	}
    }

    if ((strcasestr("none", power_model) == NULL) &&
	(strcasestr("McPAT", power_model) == NULL) &&
        (strcasestr("ORION", power_model) == NULL))   {
	error= TRUE;
	fprintf(stderr, "Unknown power model!\n");
	fprintf(stderr, "Must be one of \"McPAT\" or \"ORION\"\n");
    } else   {
	if (strcasestr("none", power_model) != NULL)   {
	    printf("*** Power model is \"none\"\n");
	    power_method= pwrNone;
	} else if (strcasestr("McPAT", power_model) != NULL)   {
	    printf("*** Power model is \"McPAT\"\n");
	    power_method= pwrMcPAT;
	} else if (strcasestr("ORION", power_model) != NULL)   {
	    printf("*** Power model is \"ORION\"\n");
	    power_method= pwrORION;
	}

    }

    /* Open the machine description file for input */
    if (strcmp(machineFname, "") == 0)   {
	/* Default */
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

    /* Open the SST xml file for output */
    if (strcmp(sstFname, "") == 0)   {
	/* Default */
	fp_sst= NULL;
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

    /* Read the machine file */
    if (read_machine_file(fp_machine, verbose) == FALSE)   {
	error= TRUE;
    }

    /* A node is the endpoint of a network router */
    if (!error)   {
	if (num_nodes() % IO_nodes)   {
	    fprintf(stderr, "Number of nodes (%d) should be evenly divisible by number "
		"of I/O nodes (%d)!\n", num_nodes(), IO_nodes);
	    error= TRUE;
	} else   {
	    printf("*** Number of I/O nodes is %d, one per %d compute nodes\n", IO_nodes,
		num_nodes() / IO_nodes);
	}
    }

    if (error)   {
	usage(argv);
	exit(1);
    }

    printf("*** Reading machine parameters from \"%s\"\n", machineFname);
    printf("*** Writing output to \"%s\"\n", sstFname);
    disp_machine_params();


    GenMesh2D(Net_x_dim(), Net_y_dim(), NoC_x_dim(), NoC_y_dim(), num_cores(), IO_nodes, num_router_nodes());


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst);
    sst_gen_param_start(fp_sst, 0);
    sst_gen_param_entries(fp_sst, fp_machine, pattern_name);
    sst_gen_param_end(fp_sst, node_lat, net_lat);
    sst_pwr_param_entries(fp_sst, power_method);
    sst_nvram_param_entries(fp_sst, nvram_read_bw, nvram_write_bw, ssd_read_bw, ssd_write_bw);

    /* We assume the router bandwidth is the same as the link bandwidth */
    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NETWORK, 4 + num_router_nodes(), net_bw,
	num_cores(), 25, wormhole, power_method);
    sst_router_param_end(fp_sst, RNAME_NETWORK);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NoC, 4 + num_cores(), node_bw, num_cores(), 25, wormhole,
	power_method);
    sst_router_param_end(fp_sst, RNAME_NoC);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NET_ACCESS, 1 + (num_cores() * NoC_x_dim() * NoC_y_dim()),
	node_bw, num_cores(), 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NET_ACCESS);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NVRAM, 1 + (num_cores() * NoC_x_dim() * NoC_y_dim()), node_bw,
	num_cores(), 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NVRAM);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_STORAGE, 1 + (num_cores() * NoC_x_dim() * NoC_y_dim()), node_bw,
	num_cores(), 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_STORAGE);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_IO, 1 + num_nodes() / IO_nodes, net_bw, num_cores(), 25,
	wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_IO);

    sst_body_start(fp_sst);
    sst_pwr_component(fp_sst, power_method);
    sst_pattern_generators(pattern_name, fp_sst);
    sst_nvram(fp_sst);
    sst_routers(fp_sst, node_lat, net_lat, nvram_lat, power_method);
    sst_body_end(fp_sst);
    sst_footer(fp_sst);

    if (fp_sst)   {
	fclose(fp_sst);
    }

    if (fp_machine)   {
	fclose(fp_machine);
    }

    return 0;

}  /* end of main() */



static void
usage(char *argv[])
{

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s -o out -m machine -p pname [--power model] [-i IO] [-h]\n", argv[0]);
    fprintf(stderr, "   --sstfilename, -o     Name of the SST xml output file\n");
    fprintf(stderr, "   --machine, -m         Name of machine description file\n");
    fprintf(stderr, "   --IO_nodes, -i        Number of I/O nodes (Default 1)\n");
    fprintf(stderr, "   --help, -h            Print this message\n");
    fprintf(stderr, "   --pattern, -p         Name of pattern; e.g., ghost, msgrate, allreduce, pingpong\n");
    fprintf(stderr, "   --power <model>       Enable power modeling using McPAT or ORION\n");

}  /* end of usage() */
