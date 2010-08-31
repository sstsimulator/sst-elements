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
    {"chckpt_delay", 1, NULL, 1000},
    {"chckpt_interval", 1, NULL, 1001},
    {"envelope_write_time", 1, NULL, 1002},
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
int x_dim, y_dim;
int num_ports;
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
char *method;		/* Checkpointing method to use */
uint64_t chckpt_delay;	/* How long to write a checkpoint in ns */
uint64_t chckpt_interval;	/* How long between checkpoints in ns */
uint64_t envelope_write_time;	/* How long to write receive envelope info */
char *power_model;	/* Which, if any, power model to use */
pwr_method_t power_method;



    /* Defaults */
    error= FALSE;
    verbose= 0;
    sstFname= "";
    x_dim= -1;
    y_dim= -1;
    net_lat= 4900;
    net_bw= 1900000000;
    node_lat= 150;
    node_bw= 12600000000;
    compute= 150000;
    app_time= 1000 * compute; /* 1000 time steps */
    num_cores= 1;
    method= "none";
    chckpt_delay= compute / 2;	/* This number is completly aritrary */
    chckpt_interval= compute * 100; /* Every 100 time steps */
    envelope_write_time= 250;	/* Assume we have very fast stable storage */


    /* Assume 2GB of memory per MPI rank. The aquare root of that is */
    /* the amount of data each node sends to each neighbor per ghost */
    /* cell (halo) exchange. */
    exchange_msg_len= 46340;
    pattern_name= NULL;

    power_model= "none";
    power_method= pwrNone;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "c:s:hx:y:p:m:", long_options, &option_index);
	if (ch == -1)   {
	    break;
	}

	switch (ch)   {
	    case 'x':
		x_dim= strtol(optarg, (char **)NULL, 0);
		if (x_dim < 1)   {
		    fprintf(stderr, "X dimension must be > 0\n");
		    error= TRUE;
		}
		break;
	    case 'y':
		y_dim= strtol(optarg, (char **)NULL, 0);
		if (y_dim < 1)   {
		    fprintf(stderr, "Y dimension must be > 0\n");
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
		chckpt_delay= strtol(optarg, (char **)NULL, 0);
		if (chckpt_delay < 1)   {
		    fprintf(stderr, "chckpt_delay must be > 0\n");
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
		envelope_write_time= strtol(optarg, (char **)NULL, 0);
		if (envelope_write_time < 1)   {
		    fprintf(stderr, "envelope_write_time must be > 0\n");
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

    if (pattern_name == NULL)   {
	error= TRUE;
	fprintf(stderr, "Need a pattern name!\n");
    }

    if (pattern_name && strstr("ghost_pattern", pattern_name) == NULL)   {
	error= TRUE;
	fprintf(stderr, "Unknown pattern name!\n");
	fprintf(stderr, "Must be one of \"ghost_pattern\", or \"\"\n");
    } else   {
	pattern_name= "ghost_pattern";
	if (!error)   {
	    printf("*** Communication pattern is \"%s\"\n", pattern_name);
	}
    }

    if ((strstr("none", method) == NULL) &&
        (strstr("coordinated", method) == NULL) &&
        (strstr("uncoordinated", method) == NULL) &&
        (strstr("distributed", method) == NULL))   {
	error= TRUE;
	fprintf(stderr, "Unknown checkpointing method!\n");
	fprintf(stderr, "Must be one of \"none\", \"coordinated\", \"uncoordinated\", "
	    "or \"distributed\"\n");
    } else   {

	if (strstr("none", method) != NULL)   {
	    method= "none";
	} else if (strstr("coordinated", method) != NULL)   {
	    method= "coordinated";
	} else if (strstr("uncoordinated", method) != NULL)   {
	    method= "uncoordinated";
	} else if (strstr("distributed", method) != NULL)   {
	    method= "distributed";
	} else   {
	    error= TRUE;
	    fprintf(stderr, "Internal error!\n");
	}
	if (!error)   {
	    printf("*** Checkpoint method is \"%s\"\n", method);
	}
    }

    if ((strstr("none", power_model) == NULL) &&
	(strstr("McPAT", power_model) == NULL) &&
        (strstr("ORION", power_model) == NULL))   {
	error= TRUE;
	fprintf(stderr, "Unknown power model!\n");
	fprintf(stderr, "Must be one of \"McPAT\" or \"ORION\"\n");
    } else   {
	if (strstr("none", power_model) != NULL)   {
	    printf("*** Power model is \"none\"\n");
	    power_method= pwrNone;
	} else if (strstr("McPAT", power_model) != NULL)   {
	    printf("*** Power model is \"McPAT\"\n");
	    power_method= pwrMcPAT;
	} else if (strstr("ORION", power_model) != NULL)   {
	    printf("*** Power model is \"ORION\"\n");
	    power_method= pwrORION;
	}

    }

    if ((x_dim < 0) || (y_dim < 0))   {
	error= TRUE;
	fprintf(stderr, "-x and -y must be specified!\n");
    }

    if (!is_pow2(x_dim * y_dim))   {
	error= TRUE;
	fprintf(stderr, "x * y must be power of two!\n");
    }

    if (!is_pow2(num_cores))   {
	error= TRUE;
	fprintf(stderr, "Number of cores must be power of two!\n");
    }

    if (!error)   {
	printf("*** Torus is x * y = %d * %d, with %d core(s) per router\n",
	    x_dim, y_dim, num_cores);
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

    if (error)   {
	usage(argv);
	exit(1);
    }

    printf("*** Writing output to \"%s\"\n", sstFname);


    num_ports= 4 + num_cores;
    GenMesh2D(x_dim, y_dim, TRUE, TRUE, num_cores);


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst);
    sst_gen_param_start(fp_sst, 0);
    sst_gen_param_entries(fp_sst, x_dim, y_dim, num_cores, net_lat, net_bw, node_lat, node_bw,
	compute, app_time, exchange_msg_len, method, chckpt_delay, chckpt_interval,
	envelope_write_time);
    sst_gen_param_end(fp_sst, node_lat);
    sst_pwr_param_entries(fp_sst, power_method);

    /* We assume the router bandwidth is the same as the link bandwidth */
    sst_router_param_start(fp_sst, num_ports, net_bw, num_cores, power_method);
    sst_router_param_end(fp_sst);
    sst_body_start(fp_sst);

    sst_pwr_component(fp_sst, power_method);
    sst_pattern_generators(pattern_name, fp_sst);
    sst_routers(fp_sst, node_lat, net_lat, power_method);
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
    fprintf(stderr, "Usage: %s -x dimX -y dimY -s sname -p pname [-c num_cores] [--power model] [-h]\n", argv[0]);
    fprintf(stderr, "   --sstfilename, -s     Name of the SST xml output file\n");
    fprintf(stderr, "   --cores, -c           Number of cores per router\n");
    fprintf(stderr, "   dimX                  X dimesnion of 2-D tours\n");
    fprintf(stderr, "   dimY                  Y dimesnion of 2-D tours\n");
    fprintf(stderr, "   --help, -h            Print this message\n");
    fprintf(stderr, "   --pattern, -p         Name of pattern; e.g., ghost_pattern\n");
    fprintf(stderr, "   --method, -m          Checkpointing method: none (default), coordinated,\n");
    fprintf(stderr, "                         uncoordinated, distributed\n");
    fprintf(stderr, "   --chckpt_delay        Time to write a checkpoint (in nano seconds)\n");
    fprintf(stderr, "   --chckpt_interval     How often to write a (coordinated) checkpoint (in nano seconds)\n");
    fprintf(stderr, "   --envelope_write_time Time needed to write receive envelope info (in nano seconds)\n");
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
