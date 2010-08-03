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
    {"sstfilename", 1, NULL, 's'},
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



    /* Defaults */
    error= FALSE;
    verbose= 0;
    sstFname= "";
    x_dim= -1;
    y_dim= -1;
    pattern_name= NULL;


    /* check command line args */
    while (1)   {
	ch= getopt_long(argc, argv, "s:hx:y:p:", long_options, &option_index);
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
	    case 's':
		sstFname= optarg;
		break;
	    case 'p':
		pattern_name= optarg;
		break;
	    case 'h':
		error= TRUE;
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

    if (strcmp(pattern_name, "ghost_pattern") != 0)   {
	error= TRUE;
	fprintf(stderr, "Unknown pattern name!\n");
	fprintf(stderr, "Must be one of \"ghost_pattern\", or \"\"\n");
    }

    if ((x_dim < 0) || (y_dim < 0))   {
	error= TRUE;
	fprintf(stderr, "-x and -y must be specified!\n");
    }

    if (!is_pow2(x_dim * y_dim))   {
	error= TRUE;
	fprintf(stderr, "x * y must be power of two!\n");
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
	    fprintf(stderr, "Could not open the SST xml output file \"%s\": %s\n", sstFname, strerror(errno));
	    exit(2);
	}
    }

    if (error)   {
	usage(argv);
	exit(1);
    }


    num_ports= 5;
    GenMesh2D(x_dim, y_dim, TRUE, TRUE);


    /*
    ** Start the sst xml file
    */
    sst_header(fp_sst);
    sst_gen_param_start(fp_sst, 0);
    sst_gen_param_entries(fp_sst);
    sst_gen_param_end(fp_sst);

    sst_router_param_start(fp_sst, num_ports);
    sst_router_param_end(fp_sst);
    sst_body_start(fp_sst);


    sst_pattern_generators(pattern_name, fp_sst);
    sst_routers(fp_sst);
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

    fprintf(stderr, "Usage: %s -x dimX -y dimY -s sname -p pname [-h]\n", argv[0]);
    fprintf(stderr, "   --sstfilename, -s     Name of the SST xml output file\n");
    fprintf(stderr, "   dimX                  X dimesnion of 2-D tours\n");
    fprintf(stderr, "   dimY                  Y dimesnion of 2-D tours\n");
    fprintf(stderr, "   --help, -h            Print this message\n");
    fprintf(stderr, "   --pattern, -p         Name of pattern; e.g., ghost_pattern\n");

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
