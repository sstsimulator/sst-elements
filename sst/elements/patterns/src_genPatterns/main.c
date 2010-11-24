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
#include "topo_mesh2D.h"



/*
** Local functions
*/
static void usage(char *argv[]);
static int is_pow2(int num);



/*
** Long options
*/
static struct option long_options[]=   {
    /* name, has arg, flag, val */
    {"help", 0, NULL, 'h'},
    {"cores", 1, NULL, 'c'},
    {"sstfilename", 1, NULL, 's'},
    {"pattern", 1, NULL, 'p'},
    {"method", 1, NULL, 'm'},
    {"IO_nodes", 1, NULL, 'i'},
    {"chckpt_size", 1, NULL, 1000},
    {"chckpt_interval", 1, NULL, 1001},
    {"envelope_size", 1, NULL, 1002},
    {"power", 1, NULL, 1003},
    {"msg_len", 1, NULL, 1004},
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
int net_x_dim, net_y_dim;
int NoC_x_dim, NoC_y_dim;
char *sstFname;
FILE *fp_sst;
char *pattern_name;
uint64_t net_lat;	/* In ns */
uint64_t net_bw;	/* In bytes per second */
uint64_t node_lat;	/* In ns */
uint64_t node_bw;	/* In bytes per second */
uint64_t compute;	/* In ns */

uint64_t app_time;	/* Application compute time per node in ns */
int exchange_msg_len;
int num_cores;		/* How many pattern generators per router */
uint64_t core_memory;	/* How much memory per core in bytes */
int num_nodes;
int IO_nodes;		/* Should be divisible by the number of nodes */
char *method;		/* Checkpointing method to use */
int chckpt_interval;	/* How long between checkpoints in ns */
int chckpt_size;	/* How many bytes for a full checkpoint of a core */
int envelope_size;	/* How many bytes is the message envelope? */
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
    net_x_dim= 1;
    net_y_dim= 1;
    NoC_x_dim= -1;
    NoC_y_dim= -1;
    net_lat= 4900;
    net_bw= 1900000000;
    node_lat= 150;
    node_bw= 12600000000;

    /* Assume a SATA3 drive (Crucial C200) "somehow" connected to an I/O network */
    ssd_read_bw= 200000000;
    ssd_write_bw= 350000000;

    /* Let's say we have a Fusion-io PCIe card on each node */
    nvram_read_bw= 490000000;
    nvram_write_bw= 700000000;
    nvram_lat= 50000;

    compute= 150000;
    app_time= 20 * compute; /* 20 time steps */
    core_memory= (uint64_t)512 * 1024 * 1024;
    num_cores= 1;
    IO_nodes= 1;
    method= "none";
    chckpt_size= core_memory / 5;
    chckpt_interval= compute * 100; /* Every 100 time steps */
    chckpt_size= core_memory / 5; /* 20% of core memory to checkpoint */
    envelope_size= 64;

    exchange_msg_len= -1;
    pattern_name= NULL;

    power_model= "none";
    power_method= pwrNone;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "c:s:hX:Y:x:y:p:m:i:", long_options, &option_index);
	if (ch == -1)   {
	    break;
	}

	switch (ch)   {
	    case 'X':
		net_x_dim= strtol(optarg, (char **)NULL, 0);
		if (net_x_dim < 1)   {
		    fprintf(stderr, "X dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'Y':
		net_y_dim= strtol(optarg, (char **)NULL, 0);
		if (net_y_dim < 1)   {
		    fprintf(stderr, "Y dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'x':
		NoC_x_dim= strtol(optarg, (char **)NULL, 0);
		if (NoC_x_dim < 1)   {
		    fprintf(stderr, "x dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'y':
		NoC_y_dim= strtol(optarg, (char **)NULL, 0);
		if (NoC_y_dim < 1)   {
		    fprintf(stderr, "y dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'c':
		num_cores= strtol(optarg, (char **)NULL, 0);
		if (num_cores < 1)   {
		    fprintf(stderr, "Number of cores per router must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'i':
		IO_nodes= strtol(optarg, (char **)NULL, 0);
		if (IO_nodes < 1)   {
		    fprintf(stderr, "Need at least one I/O node\n");
		    error= TRUE;
		}
		break;
	    case 's':
		sstFname= optarg;
		break;
	    case 'p':
		pattern_name= optarg;
		break;
	    case 'm':
		method= optarg;
		break;
	    case 'h':
		error= TRUE;
		break;
	    case 1000:
		chckpt_size= strtol(optarg, (char **)NULL, 0);
		if (chckpt_size < 1)   {
		    fprintf(stderr, "chckpt_size must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 1001:
		chckpt_interval= strtol(optarg, (char **)NULL, 0);
		if (chckpt_interval < 1)   {
		    fprintf(stderr, "chckpt_interval must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 1002:
		envelope_size= strtol(optarg, (char **)NULL, 0);
		if (envelope_size < 1)   {
		    fprintf(stderr, "envelope_size must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 1003:
		power_model= optarg;
		break;
	    case 1004:
		exchange_msg_len= strtol(optarg, (char **)NULL, 0);
		if (exchange_msg_len < 1)   {
		    fprintf(stderr, "--msg_len must be > 0\n");
		    error= TRUE;
		}
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
	    (strcasestr("msgrate_pattern", pattern_name) == NULL))   {
	    error= TRUE;
	    fprintf(stderr, "Unknown pattern name!\n");
	    fprintf(stderr, "Must be one of \"ghost_pattern\", or \"msgrate_pattern\"\n");

	} else   {

	    if (strcasestr("ghost_pattern", pattern_name) != NULL)   {
		pattern_name= "ghost_pattern";
		/* Assume 2GB of memory per MPI rank. The square root of that is */
		/* the amount of data each node sends to each neighbor per ghost */
		/* cell (halo) exchange. */
		if (exchange_msg_len < 0)   {
		    /* Use the default */
		    exchange_msg_len= 46340;
		}

	    } else if (strcasestr("msgrate_pattern", pattern_name) != NULL)   {
		pattern_name= "msgrate_pattern";
		if (exchange_msg_len < 0)   {
		    /* Use the default */
		    exchange_msg_len= 0;
		}

	    } else   {
		error= TRUE;
	    }

	    if (!error)   {
		printf("*** Communication pattern is \"%s\"\n", pattern_name);
	    }
	}
    }

    if ((strcasestr("none", method) == NULL) &&
        (strcasestr("coordinated", method) == NULL) &&
        (strcasestr("uncoordinated", method) == NULL) &&
        (strcasestr("distributed", method) == NULL))   {
	error= TRUE;
	fprintf(stderr, "Unknown checkpointing method!\n");
	fprintf(stderr, "Must be one of \"none\", \"coordinated\", \"uncoordinated\", "
	    "or \"distributed\"\n");
    } else   {

	if (strcasestr("none", method) != NULL)   {
	    method= "none";
	} else if (strcasestr("coordinated", method) != NULL)   {
	    method= "coordinated";
	} else if (strcasestr("uncoordinated", method) != NULL)   {
	    method= "uncoordinated";
	} else if (strcasestr("distributed", method) != NULL)   {
	    method= "distributed";
	} else   {
	    error= TRUE;
	    fprintf(stderr, "Internal error!\n");
	}
	if (!error)   {
	    printf("*** Checkpoint method is \"%s\"\n", method);
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

    if ((NoC_x_dim < 0) || (NoC_y_dim < 0))   {
	error= TRUE;
	fprintf(stderr, "-x and -y must be specified!\n");
    }

    if (((net_x_dim  == 1) && (net_y_dim > 1)) ||
	((net_x_dim  > 1) && (net_y_dim == 1)))   {
	error= TRUE;
	fprintf(stderr, "Can't handle network rings yet! Increase x or y dimension.\n");
    }

    if (((NoC_x_dim  == 1) && (NoC_y_dim > 1)) ||
	((NoC_x_dim  > 1) && (NoC_y_dim == 1)))   {
	error= TRUE;
	fprintf(stderr, "Can't handle NoC rings yet! Increase x or y dimension.\n");
    }

    if ((NoC_x_dim * NoC_y_dim == 1) && (num_cores > 1))   {
	error= TRUE;
	fprintf(stderr, "Can't handle single router NoC (1x1) with more than one core each!\n");
    }

    if (!is_pow2(num_cores * NoC_x_dim * NoC_y_dim * net_x_dim * net_y_dim))   {
	error= TRUE;
	fprintf(stderr, "Total number of cores must be power of two!\n");
    }

    if (!error)   {
	if (net_x_dim * net_y_dim > 1)   {
	    printf("*** Network torus is X * Y = %d * %d\n", net_x_dim, net_y_dim);
	} else   {
	    printf("*** Single node, no network\n");
	}
	if (NoC_x_dim * NoC_y_dim > 1)   {
	    printf("*** Each node has a x * y = %d * %d NoC torus, with %d core(s) per router\n",
		NoC_x_dim, NoC_y_dim, num_cores);
	} else   {
	    if (num_cores > 1)   {
		printf("*** Each node has a router with %d core(s)\n", num_cores);
	    } else   {
		printf("*** Each node consists of a single core\n");
	    }
	}
	printf("*** Total number of cores is %d\n",
	    num_cores * NoC_x_dim * NoC_y_dim * net_x_dim * net_y_dim);
    }

    /* Open the SST xml file for output */
    if (strcmp(sstFname, "") == 0)   {
	/* Default */
	fp_sst= NULL;
	error= TRUE;
	fprintf(stderr, "Output file must be specified with -s option!\n");
    } else if (strcmp(sstFname, "-") == 0)   {
	fp_sst= stdout;
    } else   {
	fp_sst= fopen(sstFname, "w");
	if (fp_sst == NULL)   {
	    fprintf(stderr, "Could not open the SST xml output file \"%s\": %s\n",
		sstFname, strerror(errno));
	    exit(2);
	}
    }

    /* A node is the endpoint of a network router */
    num_nodes= net_x_dim * net_y_dim;
    if (num_nodes % IO_nodes)   {
	fprintf(stderr, "Number of nodes (%d) should be evenly divisible by number "
	    "of I/O nodes (%d)!\n", num_nodes, IO_nodes);
	error= TRUE;
    } else   {
	printf("*** Number of I/O nodes is %d, one per %d compute nodes\n", IO_nodes,
	    num_nodes / IO_nodes);
    }

    if (error)   {
	usage(argv);
	exit(1);
    }

    printf("*** Writing output to \"%s\"\n", sstFname);


    GenMesh2D(net_x_dim, net_y_dim, NoC_x_dim, NoC_y_dim, num_cores, IO_nodes);


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst);
    sst_gen_param_start(fp_sst, 0);
    sst_gen_param_entries(fp_sst, net_x_dim, net_y_dim, NoC_x_dim, NoC_y_dim, num_cores,
	net_lat, net_bw, node_lat, node_bw,
	compute, app_time, exchange_msg_len, method, chckpt_interval,
	envelope_size, chckpt_size, pattern_name);
    sst_gen_param_end(fp_sst, node_lat, net_lat);
    sst_pwr_param_entries(fp_sst, power_method);
    sst_nvram_param_entries(fp_sst, nvram_read_bw, nvram_write_bw, ssd_read_bw, ssd_write_bw);

    /*
    ** We have several types of routers:
    ** - Network routers always have five ports, hop_delay 25
    ** - NoC routers have four + num_cores ports, hop_delay 25
    ** - Network aggregators have one + (num_cores * NoC_x_dim * NoC_y_dim) ports, hop_delay 25
    ** - NVRAM aggregators have one + (num_cores * NoC_x_dim * NoC_y_dim) ports, hop_delay 25
    ** - Storage aggregators have have one + (num_cores * NoC_x_dim * NoC_y_dim) ports, hop_delay 25
    ** - Stable storage (I/O) aggregators have one + num_nodes / I/O nodes, hop_delay 25
    */

    /* We assume the router bandwidth is the same as the link bandwidth */
    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NETWORK, 5, net_bw, num_cores, 25, wormhole, power_method);
    sst_router_param_end(fp_sst, RNAME_NETWORK);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NoC, 4 + num_cores, node_bw, num_cores, 25, wormhole,
	power_method);
    sst_router_param_end(fp_sst, RNAME_NoC);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_NET_ACCESS, 1 + (num_cores * NoC_x_dim * NoC_y_dim),
	node_bw, num_cores, 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NET_ACCESS);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_NVRAM, 1 + (num_cores * NoC_x_dim * NoC_y_dim), node_bw,
	num_cores, 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_NVRAM);

    wormhole= TRUE;
    sst_router_param_start(fp_sst, RNAME_STORAGE, 1 + (num_cores * NoC_x_dim * NoC_y_dim), node_bw,
	num_cores, 25, wormhole, pwrNone);
    sst_router_param_end(fp_sst, RNAME_STORAGE);

    wormhole= FALSE;
    sst_router_param_start(fp_sst, RNAME_IO, 1 + num_nodes / IO_nodes, net_bw, num_cores, 25,
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

    return 0;

}  /* end of main() */



static void
usage(char *argv[])
{

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s -x dimX -y dimY -X dimX -Y dimY -s sname -p pname [-c num_cores] [--power model] [-i IO] [-h]\n", argv[0]);
    fprintf(stderr, "   --sstfilename, -s     Name of the SST xml output file\n");
    fprintf(stderr, "   --cores, -c           Number of cores per NoC router (Default 1)\n");
    fprintf(stderr, "   -X dimX               X dimension of tours network\n");
    fprintf(stderr, "   -Y dimY               Y dimension of tours network\n");
    fprintf(stderr, "   -x dimX               X dimension of NoC tours\n");
    fprintf(stderr, "   -y dimY               Y dimension of NoC tours\n");
    fprintf(stderr, "   --IO_nodes, -i        Number of I/O nodes (Default 1)\n");
    fprintf(stderr, "   --help, -h            Print this message\n");
    fprintf(stderr, "   --pattern, -p         Name of pattern; e.g., ghost, msgrate\n");
    fprintf(stderr, "   --msg_len             Message size. Default is pattern dependent.\n");
    fprintf(stderr, "   --method, -m          Checkpointing method: none (default), coordinated,\n");
    fprintf(stderr, "                         uncoordinated, distributed\n");
    fprintf(stderr, "   --chckpt_size         Size of a full checkpoint (in bytes)\n");
    fprintf(stderr, "   --chckpt_interval     How often to write a (coordinated) checkpoint (in nano seconds)\n");
    fprintf(stderr, "   --envelope_size       Size of message envelope in bytes\n");
    fprintf(stderr, "   --power <model>       Enable power modeling using McPAT or ORION\n");

}  /* end of usage() */



static int
is_pow2(int num)
{
    if (num < 1)   {
	return FALSE;
    }

    if ((num & (num - 1)) == 0)   {
	return TRUE;
    }

    return FALSE;

}  /* end of is_pow2() */
