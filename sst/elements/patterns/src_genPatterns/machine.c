#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "machine.h"



static int _Net_x_dim;
static int _Net_y_dim;
static int _NoC_x_dim;
static int _NoC_y_dim;
static int _num_router_nodes;
static int _num_cores;
static int _envelope_size;

#define MAX_LINE	1024



int
read_machine_file(FILE *fp_machine)
{

int error;
char line[MAX_LINE];
char *pos;
char key[MAX_LINE], value[MAX_LINE];


    /* Defaults */
    error= FALSE;
    _Net_x_dim= NO_DEFAULT;
    _Net_y_dim= NO_DEFAULT;
    _NoC_x_dim= NO_DEFAULT;
    _NoC_y_dim= NO_DEFAULT;
    _num_router_nodes= NO_DEFAULT;
    _num_cores= NO_DEFAULT;
    _envelope_size= DEFAULT_ENVELOPE;

    /* Process the input file */
    while (fgets(line, MAX_LINE, fp_machine) != NULL)   {
	/* Get rid of comments */
	pos= strchr(line, '#');
	if (pos)   {
	    *pos= '\0';
	}

	/* Now scan it */
	if (sscanf(line, "%s = %s", key, value) != 2)   {
	    continue;
	}

	/* printf("Found %s = %s\n", key, value); */

	/* Parameter matching */
	if (strcmp("Net_x_dim", key) == 0)   {
	    _Net_x_dim= strtol(value, (char **)NULL, 0);
	} else if (strcmp("Net_y_dim", key) == 0)   {
	    _Net_y_dim= strtol(value, (char **)NULL, 0);
	} else if (strcmp("NoC_x_dim", key) == 0)   {
	    _NoC_x_dim= strtol(value, (char **)NULL, 0);
	} else if (strcmp("NoC_y_dim", key) == 0)   {
	    _NoC_y_dim= strtol(value, (char **)NULL, 0);
	} else if (strcmp("num_router_nodes", key) == 0)   {
	    _num_router_nodes= strtol(value, (char **)NULL, 0);
	} else if (strcmp("num_cores", key) == 0)   {
	    _num_cores= strtol(value, (char **)NULL, 0);
	} else if (strcmp("envelope_size", key) == 0)   {
	    _envelope_size= strtol(value, (char **)NULL, 0);
	} else   {
	    fprintf(stderr, "Unknown parameter \"%s\" in machine file!\n", key);
	    error= TRUE;
	}
    }


    /* Error checking */
    if (_Net_x_dim * _Net_y_dim < 1)   {
	fprintf(stderr, "Both Net_x_dim and Net_y_dim in machine file must be > 0!\n");
	error= TRUE;
    }

    if (_NoC_x_dim * _NoC_y_dim < 1)   {
	fprintf(stderr, "Both NoC_x_dim and NoC_y_dim in machine file must be > 0!\n");
	error= TRUE;
    }

    if (_num_router_nodes < 1)   {
	fprintf(stderr, "num_router_nodes in machine file must be > 0!\n");
	error= TRUE;
    }

    if (_num_cores < 1)   {
	fprintf(stderr, "num_cores in machine file must be > 0!\n");
	error= TRUE;
    }

    if (_envelope_size < 0)   {
	fprintf(stderr, "envelope_size in machine file must be >= 0!\n");
	error= TRUE;
    }


    if (!error)   {
	if (_Net_x_dim * _Net_y_dim > 1)   {
	    printf("*** Network torus is X * Y = %d * %d with %d node(s) per router\n",
		_Net_x_dim, _Net_y_dim, _num_router_nodes);
	} else   {
	    if (_num_router_nodes == 1)   {
		printf("*** Single node, no network\n");
	    } else   {
		printf("*** %d nodes, single router, no network\n", _num_router_nodes);
	    }
	}
	if (_NoC_x_dim * _NoC_y_dim > 1)   {
	    printf("*** Each node has a x * y = %d * %d NoC torus, with %d core(s) per router\n",
		_NoC_x_dim, _NoC_y_dim, _num_cores);
	} else   {
	    if (_num_cores > 1)   {
		printf("*** Each node has a router with %d core(s)\n", _num_cores);
	    } else   {
		printf("*** Each node consists of a single core\n");
	    }
	}
	printf("*** Total number of nodes is %d\n", num_nodes());
	printf("*** Total number of cores is %d\n", _num_cores *
	    _NoC_x_dim * _NoC_y_dim * num_nodes());
    }

    return error;

}  /* end of read_machine_file() */



int
num_nodes(void)
{
    return _Net_x_dim * _Net_y_dim * _num_router_nodes;
}

int
Net_x_dim(void)
{
    return _Net_x_dim;
}  /* end of Net_x_dim() */



int
Net_y_dim(void)
{
    return _Net_y_dim;
}  /* end of Net_y_dim() */



int
NoC_x_dim(void)
{
    return _NoC_x_dim;
}  /* end of NoC_x_dim() */



int
NoC_y_dim(void)
{
    return _NoC_y_dim;
}  /* end of NoC_y_dim() */



int
num_cores(void)
{
    return _num_cores;
}  /* end of num_cores() */



int
num_router_nodes(void)
{
    return _num_router_nodes;
}  /* end of num_router_nodes() */



int
envelope_size(void)
{
    return _envelope_size;
}  /* end of envelope_size() */
