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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>		/* For PRId64 */
#include "main.h"
#include "machine.h"



#define MAX_LINE	1024
#define MAX_INFLECTIONS	256
#define MAX_NICSTAT	256



/* 
** Parameter variables. Locally global so I don't have to keep them
** passing back and forth (too many of them)
*/
static int _Net_x_dim;
static int _Net_y_dim;
static int _Net_z_dim;
static int _NoC_x_dim;
static int _NoC_y_dim;
static int _NoC_z_dim;
static int _Net_x_wrap;
static int _Net_y_wrap;
static int _Net_z_wrap;
static int _NoC_x_wrap;
static int _NoC_y_wrap;
static int _NoC_z_wrap;
static int _num_router_nodes;
static int _num_router_cores;
static int _NetNICgap;
static int _NoCNICgap;
static int _NetNICinflections;
static int _NoCNICinflections;
static int _NetRtrinflections;
static int _NoCRtrinflections;
static int _NICstat_num;
static int _NICstat[MAX_NICSTAT];
static int64_t _NetLinkBandwidth;
static int64_t _NoCLinkBandwidth;
static int64_t _NetLinkLatency;
static int64_t _NoCLinkLatency;
static int64_t _IOLinkBandwidth;
static int64_t _IOLinkLatency;

typedef struct NICparams_t   {
    int inflectionpoint;
    int64_t latency;
} NICparams_t;

static NICparams_t *_NetNICparams= NULL;
static NICparams_t *_NoCNICparams= NULL;
static NICparams_t *_NetRtrparams= NULL;
static NICparams_t *_NoCRtrparams= NULL;



/* Local functions */
static void set_defaults(void);
static int check_inflections(NICparams_t *params, int num, char *name);
static int error_check(void);



static void
set_defaults(void)
{

int i;


    _Net_x_dim= NO_DEFAULT;
    _Net_y_dim= NO_DEFAULT;
    _Net_z_dim= 1;
    _NoC_x_dim= NO_DEFAULT;
    _NoC_y_dim= NO_DEFAULT;
    _NoC_z_dim= 1;
    _Net_x_wrap= 0;
    _Net_y_wrap= 0;
    _Net_z_wrap= 0;
    _NoC_x_wrap= 0;
    _NoC_y_wrap= 0;
    _NoC_z_wrap= 0;
    _num_router_nodes= NO_DEFAULT;
    _num_router_cores= NO_DEFAULT;
    _NetNICinflections= 0;
    _NoCNICinflections= 0;
    _NetRtrinflections= 0;
    _NoCRtrinflections= 0;
    _NetLinkBandwidth= NO_DEFAULT;
    _NoCLinkBandwidth= NO_DEFAULT;
    _NetLinkLatency= 0;
    _NoCLinkLatency= 0;
    _IOLinkBandwidth= NO_DEFAULT;
    _IOLinkLatency= NO_DEFAULT;

    if (_NetNICparams != NULL)   {
	free(_NetNICparams);
    }
    if (_NoCNICparams != NULL)   {
	free(_NoCNICparams);
    }
    if (_NetRtrparams != NULL)   {
	free(_NetRtrparams);
    }
    if (_NoCRtrparams != NULL)   {
	free(_NoCRtrparams);
    }
    _NetNICparams= (NICparams_t *)malloc(sizeof(NICparams_t) * MAX_INFLECTIONS);
    if (_NetNICparams == NULL)   {
	fprintf(stderr, "Out of memory for _NetNICparams!\n");
	exit(-1);
    }
    _NoCNICparams= (NICparams_t *)malloc(sizeof(NICparams_t) * MAX_INFLECTIONS);
    if (_NoCNICparams == NULL)   {
	fprintf(stderr, "Out of memory for _NoCNICparams!\n");
	exit(-1);
    }
    _NetRtrparams= (NICparams_t *)malloc(sizeof(NICparams_t) * MAX_INFLECTIONS);
    if (_NetRtrparams == NULL)   {
	fprintf(stderr, "Out of memory for _NetRtrparams!\n");
	exit(-1);
    }
    _NoCRtrparams= (NICparams_t *)malloc(sizeof(NICparams_t) * MAX_INFLECTIONS);
    if (_NoCRtrparams == NULL)   {
	fprintf(stderr, "Out of memory for _NoCRtrparams!\n");
	exit(-1);
    }

    for (i= 0; i < MAX_INFLECTIONS; i++)   {
	_NetNICparams[i].inflectionpoint= -1;
	_NetNICparams[i].latency= -1;
	_NoCNICparams[i].inflectionpoint= -1;
	_NoCNICparams[i].latency= -1;
	_NetRtrparams[i].inflectionpoint= -1;
	_NetRtrparams[i].latency= -1;
	_NoCRtrparams[i].inflectionpoint= -1;
	_NoCRtrparams[i].latency= -1;
    }

    for (i= 0; i < MAX_NICSTAT; i++)   {
	_NICstat[i]= NO_DEFAULT;
    }
    _NICstat_num= 0;

}  /* end of set_defaults() */




static int
check_inflections(NICparams_t *params, int num, char *name)
{

int found_zero;
int i;


    found_zero= FALSE;
    for (i= 0; i < num; i++)   {
	if (params[i].inflectionpoint < 0)   {
	    fprintf(stderr, "%sinflection%d in machine file must be >= 0!\n", name, i);
	    return FALSE;
	} else if (params[i].inflectionpoint == 0)   {
	    found_zero= TRUE;
	}
	if (params[i].latency < 0)   {
	    fprintf(stderr, "%slatency%d in machine file must be >= 0!\n", name, i);
	    return FALSE;
	}
    }
    if (!found_zero)   {
	fprintf(stderr, "Need a %sparams entry for inflection point 0!\n", name);
	return FALSE;
    }

    return TRUE;

}  /* end of check_inflection() */



static int
error_check(void)
{

    if (_Net_x_dim * _Net_y_dim * _Net_z_dim < 1)   {
	fprintf(stderr, "Net_x_dim, Net_y_dim, and Net_z_dim in machine file must be > 0!\n");
	return FALSE;
    }

    if (_NoC_x_dim * _NoC_y_dim * _NoC_z_dim < 1)   {
	fprintf(stderr, "NoC_x_dim, NoC_y_dim, and NoC_z_dim in machine file must be > 0!\n");
	return FALSE;
    }

    if (_num_router_nodes < 1)   {
	fprintf(stderr, "num_router_nodes in machine file must be > 0!\n");
	return FALSE;
    }

    if (_num_router_cores < 1)   {
	fprintf(stderr, "num_router_cores in machine file must be > 0!\n");
	return FALSE;
    }

    if (_NetNICgap < 0)   {
	fprintf(stderr, "NetNICgap in machine file must be >= 0!\n");
	return FALSE;
    }

    if (_NoCNICgap < 0)   {
	fprintf(stderr, "NetNICgap in machine file must be >= 0!\n");
	return FALSE;
    }

    if (_NetNICinflections < 1)   {
	fprintf(stderr, "Need at least one NetNICparams!\n");
	return FALSE;
    }

    if (_NoCNICinflections < 1)   {
	fprintf(stderr, "Need at least one NoCNICparams!\n");
	return FALSE;
    }

    if (_NetRtrinflections < 1)   {
	fprintf(stderr, "Need at least one NetRtrparams!\n");
	return FALSE;
    }

    if (_NoCRtrinflections < 1)   {
	fprintf(stderr, "Need at least one NoCRtrparams!\n");
	return FALSE;
    }

    if (_NetLinkBandwidth < 0)   {
	fprintf(stderr, "Need to set NetLinkBandwidth in machine file!\n");
	return FALSE;
    }
    if (_NoCLinkBandwidth < 0)   {
	fprintf(stderr, "Need to set NoCLinkBandwidth in machine file!\n");
	return FALSE;
    }
    if (_IOLinkBandwidth < 0)   {
	fprintf(stderr, "Need to set IOLinkBandwidth in machine file!\n");
	return FALSE;
    }
    if (_IOLinkLatency < 0)   {
	fprintf(stderr, "Need to set IOLinkLatency in machine file!\n");
	return FALSE;
    }



    if (!check_inflections(_NetNICparams, _NetNICinflections, "NetCNIC"))   {
	return FALSE;
    }

    if (!check_inflections(_NoCNICparams, _NoCNICinflections, "NoCNIC"))   {
	return FALSE;
    }

    if (!check_inflections(_NetRtrparams, _NetRtrinflections, "NetCRtr"))   {
	return FALSE;
    }

    if (!check_inflections(_NoCRtrparams, _NoCRtrinflections, "NoCCRtr"))   {
	return FALSE;
    }

    /* No errors found */
    return TRUE;

}  /* end of error_check() */



void
disp_machine_params(void)
{

int i;


    if ((_Net_x_dim * _Net_y_dim > 1) && (_Net_z_dim == 1))   {
	printf("# *** Network torus is X * Y = %d * %d with %d node(s) per router\n",
	    _Net_x_dim, _Net_y_dim, _num_router_nodes);
    } else if (_Net_x_dim * _Net_y_dim * _Net_z_dim > 1)   {
	printf("# *** Network torus is X * Y * Z = %d * %d * %d with %d node(s) per router\n",
	    _Net_x_dim, _Net_y_dim, _Net_z_dim, _num_router_nodes);
    } else   {
	if (_num_router_nodes == 1)   {
	    printf("#     Single node, no network\n");
	} else   {
	    printf("#     %d nodes, single router, no network\n", _num_router_nodes);
	}
    }
    printf("#     Net dimensions wrapped: ");
    if (_Net_x_wrap || _Net_y_wrap || _Net_z_wrap)   {
	if (_Net_x_wrap)   {
	    printf("X ");
	}
	if (_Net_y_wrap)   {
	    printf("Y ");
	}
	if (_Net_z_wrap)   {
	    printf("Z ");
	}
	printf("\n");
    } else   {
	printf("none\n");
    }

    if ((_NoC_x_dim * _NoC_y_dim > 1) && (_NoC_z_dim == 1))   {
	printf("# *** Each node has a x * y = %d * %d NoC torus, with %d core(s) per router\n",
	    _NoC_x_dim, _NoC_y_dim, _num_router_cores);
    } else if (_NoC_x_dim * _NoC_y_dim * _NoC_z_dim > 1)   {
	printf("# *** Each node has a x * y * z = %d * %d * %d NoC torus, with %d core(s) per router\n",
	    _NoC_x_dim, _NoC_y_dim, _NoC_z_dim, _num_router_cores);
    } else   {
	if (_num_router_cores > 1)   {
	    printf("# *** Each node has a router with %d core(s)\n", _num_router_cores);
	} else   {
	    printf("# *** Each node consists of a single core\n");
	}
    }
    printf("#     NoC dimensions wrapped: ");
    if (_NoC_x_wrap || _NoC_y_wrap || _NoC_z_wrap)   {
	if (_NoC_x_wrap)   {
	    printf("X ");
	}
	if (_NoC_y_wrap)   {
	    printf("Y ");
	}
	if (_NoC_z_wrap)   {
	    printf("Z ");
	}
	printf("\n");
    } else   {
	printf("none\n");
    }



    printf("# *** Number of nodes is %d\n", num_nodes());
    printf("# *** Number of cores per node is %d\n", num_cores());
    printf("# *** Total number of cores is %d\n", num_cores() * num_nodes());

    printf("# *** Network NIC has %d inflection points, gap is %d ns\n",
	NetNICinflections(), NetNICgap());
    printf("# *** NoC NIC has %d inflection points, gap is %d ns\n",
	NoCNICinflections(), NoCNICgap());

    printf("# *** Network router has %d inflection points\n", NetRtrinflections());
    printf("# *** NoC router has %d inflection points\n", NoCRtrinflections());

    printf("# *** Net link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_NetLinkBandwidth, _NetLinkLatency);
    printf("# *** NoC link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_NoCLinkBandwidth, _NoCLinkLatency);
    printf("# *** I/O link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_IOLinkBandwidth, _IOLinkLatency);

    printf("# *** Print NIC statistics for ranks: ");
    for (i= 0; i < _NICstat_num; i++)   {
	printf("%d, ", _NICstat[i]);
    }
    printf("\n");

}  /* end of disp_machine_params() */



int
read_machine_file(FILE *fp_machine, int verbose)
{

int error;
char line[MAX_LINE];
char *pos;
char key[MAX_LINE];
char value1[MAX_LINE];
char value2[MAX_LINE];
int rc;


    if (fp_machine == NULL)   {
	return FALSE;
    }

    /* Defaults */
    error= FALSE;
    set_defaults();


    /* Process the input file */
    while (fgets(line, MAX_LINE, fp_machine) != NULL)   {
	/* Get rid of comments */
	pos= strchr(line, '#');
	if (pos)   {
	    *pos= '\0';
	}

	/* Now scan it */
	rc= sscanf(line, "%s = %s %s", key, value1, value2);
	if ((rc != 2) && (rc != 3))   {
	    continue;
	}

	if (verbose > 1)   {
	    if (rc == 2)   {
		printf("Debug: Found %s = %s\n", key, value1);
	    } else   {
		printf("Debug: Found %s = %s %s\n", key, value1, value2);
	    }
	}

	/* Parameter matching */
	if (strcmp("Net_x_dim", key) == 0)   {
	    _Net_x_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("Net_y_dim", key) == 0)   {
	    _Net_y_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("Net_x_dim", key) == 0)   {
	    _Net_x_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("Net_z_dim", key) == 0)   {
	    _Net_z_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_x_dim", key) == 0)   {
	    _NoC_x_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_y_dim", key) == 0)   {
	    _NoC_y_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_z_dim", key) == 0)   {
	    _NoC_z_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("num_router_nodes", key) == 0)   {
	    _num_router_nodes= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("num_router_cores", key) == 0)   {
	    _num_router_cores= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NetNICgap", key) == 0)   {
	    _NetNICgap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoCNICgap", key) == 0)   {
	    _NoCNICgap= strtol(value1, (char **)NULL, 0);

	} else if (strcmp("Net_x_wrap", key) == 0)   {
	    _Net_x_wrap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("Net_y_wrap", key) == 0)   {
	    _Net_y_wrap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("Net_z_wrap", key) == 0)   {
	    _Net_z_wrap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_x_wrap", key) == 0)   {
	    _NoC_x_wrap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_y_wrap", key) == 0)   {
	    _NoC_y_wrap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_z_wrap", key) == 0)   {
	    _NoC_z_wrap= strtol(value1, (char **)NULL, 0);

	} else if (strcmp("NetLinkBandwidth", key) == 0)   {
	    _NetLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("NoCLinkBandwidth", key) == 0)   {
	    _NoCLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("IOLinkBandwidth", key) == 0)   {
	    _IOLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("IOLinkLatency", key) == 0)   {
	    _IOLinkLatency= strtoll(value1, (char **)NULL, 0);


	/* Net NIC parameters */
	} else if (strstr(key, "NetNICparams") == key)   {
	    if (_NetNICinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NetNICparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NetNICparams[_NetNICinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NetNICparams[_NetNICinflections].latency= strtoll(value2, (char **)NULL, 0);
	    _NetNICinflections++;
	} else if (strstr(key, "NoCNICparams") == key)   {
	    if (_NoCNICinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NoCNICparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NoCNICparams[_NoCNICinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NoCNICparams[_NoCNICinflections].latency= strtoll(value2, (char **)NULL, 0);
	    _NoCNICinflections++;
	} else if (strstr(key, "NetRtrparams") == key)   {
	    if (_NetRtrinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NetRtrparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NetRtrparams[_NetRtrinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NetRtrparams[_NetRtrinflections].latency= strtoll(value2, (char **)NULL, 0);
	    _NetRtrinflections++;
	} else if (strstr(key, "NoCRtrparams") == key)   {
	    if (_NoCRtrinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NoCRtrparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NoCRtrparams[_NoCRtrinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NoCRtrparams[_NoCRtrinflections].latency= strtoll(value2, (char **)NULL, 0);
	    _NoCRtrinflections++;

	} else if (strcmp("NICstat", key) == 0)   {
	    _NICstat[_NICstat_num++]= strtol(value1, (char **)NULL, 0);

	} else   {
	    fprintf(stderr, "Unknown parameter \"%s\" in machine file!\n", key);
	    error= TRUE;
	}
    }

    if (error)   {
	return FALSE;
    }

    if (error_check() == FALSE)   {
	return FALSE;
    }

    /* Seems OK */
    return TRUE;

}  /* end of read_machine_file() */



/* Per machine */
int num_net_routers(void) {return _Net_x_dim * _Net_y_dim * _Net_z_dim;}
int num_nodes(void) {return num_net_routers() * _num_router_nodes;}

/* Per node */
int num_NoC_routers(void) {return _NoC_x_dim * _NoC_y_dim * _NoC_z_dim;}
int num_cores(void) {return num_NoC_routers() * _num_router_cores;}

int Net_x_wrap(void) {return _Net_x_wrap;}
int Net_y_wrap(void) {return _Net_y_wrap;}
int Net_z_wrap(void) {return _Net_z_wrap;}
int NoC_x_wrap(void) {return _NoC_x_wrap;}
int NoC_y_wrap(void) {return _NoC_y_wrap;}
int NoC_z_wrap(void) {return _NoC_z_wrap;}
int Net_x_dim(void) {return _Net_x_dim;}
int Net_y_dim(void) {return _Net_y_dim;}
int Net_z_dim(void) {return _Net_z_dim;}
int NoC_x_dim(void) {return _NoC_x_dim;}
int NoC_y_dim(void) {return _NoC_y_dim;}
int NoC_z_dim(void) {return _NoC_z_dim;}



/* Cores attached to same router */
int num_router_cores(void) {return _num_router_cores;}

/* Nodes attached to the same router */
int num_router_nodes(void) {return _num_router_nodes;}



/* Parameters */
int NetNICgap(void) {return _NetNICgap;}
int NoCNICgap(void) {return _NoCNICgap;}
int NetNICinflections(void) {return _NetNICinflections;}
int NoCNICinflections(void) {return _NoCNICinflections;}
int NetRtrinflections(void) {return _NetRtrinflections;}
int NoCRtrinflections(void) {return _NoCRtrinflections;}
int64_t NetLinkBandwidth(void) {return _NetLinkBandwidth;}
int64_t NoCLinkBandwidth(void) {return _NoCLinkBandwidth;}
int64_t NetLinkLatency(void) {return _NetLinkLatency;}
int64_t NoCLinkLatency(void) {return _NoCLinkLatency;}
int64_t IOLinkBandwidth(void) {return _IOLinkBandwidth;}
int64_t IOLinkLatency(void) {return _IOLinkLatency;}



int
NetNICinflectionpoint(int index)
{
    if ((index >= 0) && (index < _NetNICinflections))   {
	return _NetNICparams[index].inflectionpoint;
    } else   {
	return -1;
    }
}  /* end of NetNICinflectionpoint() */



int
NoCNICinflectionpoint(int index)
{
    if ((index >= 0) && (index < _NoCNICinflections))   {
	return _NoCNICparams[index].inflectionpoint;
    } else   {
	return -1;
    }
}  /* end of NoCNICinflectionpoint() */



int
NetRtrinflectionpoint(int index)
{
    if ((index >= 0) && (index < _NetRtrinflections))   {
	return _NetRtrparams[index].inflectionpoint;
    } else   {
	return -1;
    }
}  /* end of NetRtrinflectionpoint() */



int
NoCRtrinflectionpoint(int index)
{
    if ((index >= 0) && (index < _NoCRtrinflections))   {
	return _NoCRtrparams[index].inflectionpoint;
    } else   {
	return -1;
    }
}  /* end of NoCRtrinflectionpoint() */



int64_t
NoCNIClatency(int index)
{
    if ((index >= 0) && (index < _NoCNICinflections))   {
	return _NoCNICparams[index].latency;
    } else   {
	return -1;
    }
}  /* end of NoCNIClatency() */



int64_t
NetNIClatency(int index)
{
    if ((index >= 0) && (index < _NetNICinflections))   {
	return _NetNICparams[index].latency;
    } else   {
	return -1;
    }
}  /* end of NetNIClatency() */



int64_t
NoCRtrlatency(int index)
{
    if ((index >= 0) && (index < _NoCRtrinflections))   {
	return _NoCRtrparams[index].latency;
    } else   {
	return -1;
    }
}  /* end of NoCRtrlatency() */



int64_t
NetRtrlatency(int index)
{
    if ((index >= 0) && (index < _NetRtrinflections))   {
	return _NetRtrparams[index].latency;
    } else   {
	return -1;
    }
}  /* end of NetRtrlatency() */



void
machine_params(FILE *out)
{

int i;


    for (i= 0; i < _NICstat_num; i++)   {
	fprintf(out, "\t\t<NICstat%d> %d </NICstat%d>\n", i, _NICstat[i], i);
    }

}  /* end of machine_params() */
