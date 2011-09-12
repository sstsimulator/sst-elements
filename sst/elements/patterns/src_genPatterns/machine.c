#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>		/* For PRId64 */
#include "main.h"
#include "machine.h"



#define MAX_LINE	1024
#define MAX_INFLECTIONS	256
#define MAX_NICSTAT	256


/* Local functions */
static void set_defaults(void);
static int error_check(void);

/* 
** Parameter variables. Locally global so I don't have to keep them
** passing back and forth (too many of them)
*/
static int _Net_x_dim;
static int _Net_y_dim;
static int _NoC_x_dim;
static int _NoC_y_dim;
static int _num_router_nodes;
static int _num_cores;
static int _NetNICgap;
static int _NoCNICgap;
static int _NetNICinflections;
static int _NoCNICinflections;
static int _NICstat_num;
static int _NICstat[MAX_NICSTAT];
static int64_t _NetLinkBandwidth;
static int64_t _NoCLinkBandwidth;
static int64_t _NetLinkLatency;
static int64_t _NoCLinkLatency;
static int64_t _IOLinkBandwidth;
static int64_t _IOLinkLatency;
static int64_t _NetIntraLatency;
static int64_t _NoCIntraLatency;

typedef struct NICparams_t   {
    int inflectionpoint;
    int64_t latency;
} NICparams_t;

static NICparams_t *_NetNICparams= NULL;
static NICparams_t *_NoCNICparams= NULL;



static void
set_defaults(void)
{

int i;


    _Net_x_dim= NO_DEFAULT;
    _Net_y_dim= NO_DEFAULT;
    _NoC_x_dim= NO_DEFAULT;
    _NoC_y_dim= NO_DEFAULT;
    _num_router_nodes= NO_DEFAULT;
    _num_cores= NO_DEFAULT;
    _NetNICinflections= 0;
    _NoCNICinflections= 0;
    _NetLinkBandwidth= NO_DEFAULT;
    _NoCLinkBandwidth= NO_DEFAULT;
    _NetLinkLatency= 999999999;
    _NoCLinkLatency= 999999999;
    _IOLinkBandwidth= NO_DEFAULT;
    _IOLinkLatency= NO_DEFAULT;
    _NetIntraLatency= NO_DEFAULT;
    _NoCIntraLatency= NO_DEFAULT;

    if (_NetNICparams != NULL)   {
	free(_NetNICparams);
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

    for (i= 0; i < MAX_INFLECTIONS; i++)   {
	_NetNICparams[i].inflectionpoint= -1;
	_NetNICparams[i].latency= -1;
	_NoCNICparams[i].inflectionpoint= -1;
	_NoCNICparams[i].latency= -1;
    }

    for (i= 0; i < MAX_NICSTAT; i++)   {
	_NICstat[i]= NO_DEFAULT;
    }
    _NICstat_num= 0;

}  /* end of set_defaults() */



static int
error_check(void)
{

int i;
int found_zero;


    if (_Net_x_dim * _Net_y_dim < 1)   {
	fprintf(stderr, "Both Net_x_dim and Net_y_dim in machine file must be > 0!\n");
	return FALSE;
    }

    if (_NoC_x_dim * _NoC_y_dim < 1)   {
	fprintf(stderr, "Both NoC_x_dim and NoC_y_dim in machine file must be > 0!\n");
	return FALSE;
    }

    if (_num_router_nodes < 1)   {
	fprintf(stderr, "num_router_nodes in machine file must be > 0!\n");
	return FALSE;
    }

    if (_num_cores < 1)   {
	fprintf(stderr, "num_cores in machine file must be > 0!\n");
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

    if (_NetLinkBandwidth < 0)   {
	fprintf(stderr, "Need to set NetLinkBandwidth in machine file!\n");
	return FALSE;
    }
    if (_NoCLinkBandwidth < 0)   {
	fprintf(stderr, "Need to set NoCLinkBandwidth in machine file!\n");
	return FALSE;
    }
    if (_NoCLinkLatency < 0)   {
	fprintf(stderr, "Need to set NoCLinkLatency in machine file!\n");
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

    if (_NetIntraLatency < 0)   {
	fprintf(stderr, "Need to set NetIntraLatency in machine file!\n");
	return FALSE;
    }
    if (_NoCIntraLatency < 0)   {
	fprintf(stderr, "Need to set NoCIntraLatency in machine file!\n");
	return FALSE;
    }



    found_zero= FALSE;
    for (i= 0; i < _NetNICinflections; i++)   {
	if (_NetNICparams[i].inflectionpoint < 0)   {
	    fprintf(stderr, "NetNICinflection%d in machine file must be >= 0!\n", i);
	    return FALSE;
	} else if (_NetNICparams[i].inflectionpoint == 0)   {
	    found_zero= TRUE;
	}

	if (_NetNICparams[i].latency < 0)   {
	    fprintf(stderr, "NetNIClatency%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
    }
    if (!found_zero)   {
	fprintf(stderr, "Need a NetNICparams entry for inflection point 0!\n");
	return FALSE;
    }

    found_zero= FALSE;
    for (i= 0; i < _NoCNICinflections; i++)   {
	if (_NoCNICparams[i].inflectionpoint < 0)   {
	    fprintf(stderr, "NoCNICinflection%d in machine file must be >= 0!\n", i);
	    return FALSE;
	} else if (_NoCNICparams[i].inflectionpoint == 0)   {
	    found_zero= TRUE;
	}
	if (_NoCNICparams[i].latency < 0)   {
	    fprintf(stderr, "NoCNIClatency%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
    }
    if (!found_zero)   {
	fprintf(stderr, "Need a NoCNICparams entry for inflection point 0!\n");
	return FALSE;
    }

    /* No errors found */
    return TRUE;

}  /* end of error_check() */



void
disp_machine_params(void)
{

int i;


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

    printf("*** Network NIC has %d inflection points, gap is %d ns\n",
	NetNICinflections(), NetNICgap());
    printf("*** NoC NIC has %d inflection points, gap is %d ns\n",
	NoCNICinflections(), NoCNICgap());

    printf("*** Net link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_NetLinkBandwidth, _NetLinkLatency);
    printf("*** NoC link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_NoCLinkBandwidth, _NoCLinkLatency);
    printf("*** I/O link: Bandwidth %" PRId64 " B/s, latency %" PRId64 " ns\n",
	_IOLinkBandwidth, _IOLinkLatency);
    printf("*** Link latency between routers: Network %" PRId64 " ns, NoC %" PRId64 " ns\n",
	_NetIntraLatency, _NoCIntraLatency);

    printf("*** Print NIC statistics for ranks: ");
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
	} else if (strcmp("NoC_x_dim", key) == 0)   {
	    _NoC_x_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoC_y_dim", key) == 0)   {
	    _NoC_y_dim= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("num_router_nodes", key) == 0)   {
	    _num_router_nodes= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("num_cores", key) == 0)   {
	    _num_cores= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NetNICgap", key) == 0)   {
	    _NetNICgap= strtol(value1, (char **)NULL, 0);
	} else if (strcmp("NoCNICgap", key) == 0)   {
	    _NoCNICgap= strtol(value1, (char **)NULL, 0);

	} else if (strcmp("NetLinkBandwidth", key) == 0)   {
	    _NetLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("NoCLinkBandwidth", key) == 0)   {
	    _NoCLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("NoCLinkLatency", key) == 0)   {
	    _NoCLinkLatency= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("IOLinkBandwidth", key) == 0)   {
	    _IOLinkBandwidth= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("IOLinkLatency", key) == 0)   {
	    _IOLinkLatency= strtoll(value1, (char **)NULL, 0);

	} else if (strcmp("NetIntraLatency", key) == 0)   {
	    _NetIntraLatency= strtoll(value1, (char **)NULL, 0);
	} else if (strcmp("NoCIntraLatency", key) == 0)   {
	    _NoCIntraLatency= strtoll(value1, (char **)NULL, 0);

	/* Net NIC parameters */
	} else if (strstr(key, "NetNICparams") == key)   {
	    if (_NetNICinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NetNICparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NetNICparams[_NetNICinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NetNICparams[_NetNICinflections].latency= strtoll(value2, (char **)NULL, 0);
	    if (_NetNICparams[_NetNICinflections].latency < _NetLinkLatency)   {
		/* Set Net link latency to smallest NIC lateny we know of */
		_NetLinkLatency= _NetNICparams[_NetNICinflections].latency;
	    }
	    _NetNICinflections++;
	} else if (strstr(key, "NoCNICparams") == key)   {
	    if (_NoCNICinflections >= MAX_INFLECTIONS)   {
		fprintf(stderr, "Too many NoCNICparams. Max is %d\n", MAX_INFLECTIONS);
		error= TRUE;
		break;
	    }
	    _NoCNICparams[_NoCNICinflections].inflectionpoint= strtol(value1, (char **)NULL, 0);
	    _NoCNICparams[_NoCNICinflections].latency= strtoll(value2, (char **)NULL, 0);
	    if (_NoCNICparams[_NoCNICinflections].latency < _NoCLinkLatency)   {
		/* Set NoC link latency to smallest NIC lateny we know of */
		_NoCLinkLatency= _NoCNICparams[_NoCNICinflections].latency;
	    }
	    _NoCNICinflections++;

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
NetNICgap(void)
{
    return _NetNICgap;
}  /* end of NetNICgap() */



int
NoCNICgap(void)
{
    return _NoCNICgap;
}  /* end of NoCNICgap() */



int
NetNICinflections(void)
{
    return _NetNICinflections;
}  /* end of NetNICinflections() */



int
NoCNICinflections(void)
{
    return _NoCNICinflections;
}  /* end of NoCNICinflections() */



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
NetLinkBandwidth(void)
{
    return _NetLinkBandwidth;
}  /* end of NetLinkBandwidth() */



int64_t
NoCLinkBandwidth(void)
{
    return _NoCLinkBandwidth;
}  /* end of NoCLinkBandwidth() */



int64_t
NetLinkLatency(void)
{
    return _NetLinkLatency;
}  /* end of NetLinkLatency() */



int64_t
NoCLinkLatency(void)
{
    return _NoCLinkLatency;
}  /* end of NoCLinkLatency() */



int64_t
IOLinkBandwidth(void)
{
    return _IOLinkBandwidth;
}  /* end of IOLinkBandwidth() */



int64_t
IOLinkLatency(void)
{
    return _IOLinkLatency;
}  /* end of IOLinkLatency() */



int64_t
NetIntraLatency(void)
{
    return _NetIntraLatency;
}  /* end of NetIntraLatency() */



int64_t
NoCIntraLatency(void)
{
    return _NoCIntraLatency;
}  /* end of NoCIntraLatency() */



int
NetRouterLatency(void)
{
    /*
    ** We set the router hop_delay to 0 and add latency to the link connected to it.
    ** That way SST can use it to loosen synchronization constraints.
    */
    return 0;

}  /* end of NetRouterLatency() */



int
NoCRouterLatency(void)
{
    /*
    ** We set the router hop_delay to 0 and add latency to the link connected to it.
    ** That way SST can use it to loosen synchronization constraints.
    */
    return 0;

}  /* end of NoCRouterLatency() */



void
machine_params(FILE *out)
{

int i;


    for (i= 0; i < _NICstat_num; i++)   {
	fprintf(out, "    <NICstat%d> %d </NICstat%d>\n", i, _NICstat[i], i);
    }

}  /* end of machine_params() */
