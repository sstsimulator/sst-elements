#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "machine.h"



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
static int _envelope_size;
static int _NetNICgap;
static int _NoCNICgap;
static int _NetNICinflections;
static int _NoCNICinflections;

typedef struct NICparams_t   {
    int inflectionpoint;
    int64_t latency;
    int64_t bandwidth;
} NICparams_t;

static NICparams_t *_NetNICparams;
static NICparams_t *_NoCNICparams;

#define MAX_LINE	1024



void
set_defaults(void)
{

    _Net_x_dim= NO_DEFAULT;
    _Net_y_dim= NO_DEFAULT;
    _NoC_x_dim= NO_DEFAULT;
    _NoC_y_dim= NO_DEFAULT;
    _num_router_nodes= NO_DEFAULT;
    _num_cores= NO_DEFAULT;
    _envelope_size= DEFAULT_ENVELOPE;
    _NetNICinflections= 0;
    _NoCNICinflections= 0;
    _NetNICparams= NULL;
    _NoCNICparams= NULL;

}  /* end of set_defaults() */



int
error_check(void)
{

int i;


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

    if (_envelope_size < 0)   {
	fprintf(stderr, "envelope_size in machine file must be >= 0!\n");
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
	fprintf(stderr, "NetNICinflections in machine file must be > 0!\n");
	return FALSE;
    }

    if (_NoCNICinflections < 1)   {
	fprintf(stderr, "NoCNICinflections in machine file must be > 0!\n");
	return FALSE;
    }

    for (i= 0; i < _NetNICinflections; i++)   {
	if (_NetNICparams[i].inflectionpoint < 0)   {
	    fprintf(stderr, "NetNICinflection%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
	if (_NetNICparams[i].latency < 0)   {
	    fprintf(stderr, "NetNIClatency%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
	if (_NetNICparams[i].bandwidth < 0)   {
	    fprintf(stderr, "NetNICbandwidth%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
    }

    for (i= 0; i < _NoCNICinflections; i++)   {
	if (_NoCNICparams[i].inflectionpoint < 0)   {
	    fprintf(stderr, "NoCNICinflection%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
	if (_NoCNICparams[i].latency < 0)   {
	    fprintf(stderr, "NoCNIClatency%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
	if (_NoCNICparams[i].bandwidth < 0)   {
	    fprintf(stderr, "NoCNICbandwidth%d in machine file must be >= 0!\n", i);
	    return FALSE;
	}
    }

    /* No errors found */
    return TRUE;

}  /* end of error_check() */



void
disp_machine_params(void)
{

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

    printf("*** Message enevelope size is %d\n", envelope_size());
    printf("*** Network NIC has %d inflection points, gap is %dns\n",
	NetNICinflections(), NetNICgap());
    printf("*** NoC NIC has %d inflection points, gap is %dns\n",
	NoCNICinflections(), NoCNICgap());

}  /* end of disp_machine_params() */



int
read_machine_file(FILE *fp_machine, int verbose)
{

int error;
char line[MAX_LINE];
char *pos;
char key[MAX_LINE], value[MAX_LINE];
int index;
int i;


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
	if (sscanf(line, "%s = %s", key, value) != 2)   {
	    continue;
	}

	if (verbose > 1)   {
	    printf("Debug: Found %s = %s\n", key, value);
	}

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
	} else if (strcmp("NetNICgap", key) == 0)   {
	    _NetNICgap= strtol(value, (char **)NULL, 0);
	} else if (strcmp("NoCNICgap", key) == 0)   {
	    _NoCNICgap= strtol(value, (char **)NULL, 0);
	} else if (strcmp("NetNICinflections", key) == 0)   {
	    _NetNICinflections= strtol(value, (char **)NULL, 0);
	    if (_NetNICparams != NULL)   {
		free(_NetNICparams);
	    }
	    _NetNICparams= (NICparams_t *)malloc(sizeof(NICparams_t) * _NetNICinflections);
	    if (_NetNICparams == NULL)   {
		fprintf(stderr, "Out of memory for _NetNICparams!\n");
		error= TRUE;
	    }
	    for (i= 0; i < _NetNICinflections; i++)   {
		_NetNICparams[i].inflectionpoint= -1;
		_NetNICparams[i].latency= -1;
		_NetNICparams[i].bandwidth= -1;
	    }
	} else if (strcmp("NoCNICinflections", key) == 0)   {
	    _NoCNICinflections= strtol(value, (char **)NULL, 0);
	    if (_NoCNICparams != NULL)   {
		free(_NoCNICparams);
	    }
	    _NoCNICparams= (NICparams_t *)malloc(sizeof(NICparams_t) * _NoCNICinflections);
	    if (_NoCNICparams == NULL)   {
		fprintf(stderr, "Out of memory for _NoCNICparams!\n");
		error= TRUE;
		break;
	    }
	    for (i= 0; i < _NoCNICinflections; i++)   {
		_NoCNICparams[i].inflectionpoint= -1;
		_NoCNICparams[i].latency= -1;
		_NoCNICparams[i].bandwidth= -1;
	    }

	/* Net NIC parameters */
	} else if (strstr(key, "NetNICinflection") == key)   {
	    sscanf(key, "NetNICinflection%d", &index);
	    if ((index < 0) || (index >= _NetNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NetNICinflection!\n", index);
		error= TRUE;
		break;
	    }
	    _NetNICparams[index].inflectionpoint= strtol(value, (char **)NULL, 0);
	} else if (strstr(key, "NetNIClatency") == key)   {
	    sscanf(key, "NetNIClatency%d", &index);
	    if ((index < 0) || (index >= _NetNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NetNIClatency!\n", index);
		error= TRUE;
		break;
	    }
	    _NetNICparams[index].latency= strtol(value, (char **)NULL, 0);
	} else if (strstr(key, "NetNICbandwidth") == key)   {
	    sscanf(key, "NetNICbandwidth%d", &index);
	    if ((index < 0) || (index >= _NetNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NetNICbandwidth!\n", index);
		error= TRUE;
		break;
	    }
	    _NetNICparams[index].bandwidth= strtol(value, (char **)NULL, 0);

	/* NoC NIC parameters */
	} else if (strstr(key, "NoCNICinflection") == key)   {
	    sscanf(key, "NoCNICinflection%d", &index);
	    if ((index < 0) || (index >= _NoCNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NoCNICinflection!\n", index);
		error= TRUE;
		break;
	    }
	    _NoCNICparams[index].inflectionpoint= strtol(value, (char **)NULL, 0);
	} else if (strstr(key, "NoCNIClatency") == key)   {
	    sscanf(key, "NoCNIClatency%d", &index);
	    if ((index < 0) || (index >= _NoCNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NoCNIClatency!\n", index);
		error= TRUE;
		break;
	    }
	    _NoCNICparams[index].latency= strtol(value, (char **)NULL, 0);
	} else if (strstr(key, "NoCNICbandwidth") == key)   {
	    sscanf(key, "NoCNICbandwidth%d", &index);
	    if ((index < 0) || (index >= _NoCNICinflections))   {
		fprintf(stderr, "Invalid index (%d) for NoCNICbandwidth!\n", index);
		error= TRUE;
		break;
	    }
	    _NoCNICparams[index].bandwidth= strtol(value, (char **)NULL, 0);
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
envelope_size(void)
{
    return _envelope_size;
}  /* end of envelope_size() */



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
NoCNICbandwidth(int index)
{
    if ((index >= 0) && (index < _NoCNICinflections))   {
	return _NoCNICparams[index].bandwidth;
    } else   {
	return -1;
    }
}  /* end of NoCNICbandwidth() */



int64_t
NetNICbandwidth(int index)
{
    if ((index >= 0) && (index < _NetNICinflections))   {
	return _NetNICparams[index].bandwidth;
    } else   {
	return -1;
    }
}  /* end of NetNICbandwidth() */
