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
#include <stdint.h>
#include <string.h>
#include <inttypes.h>		/* For PRId64 */
#include "main.h"
#include "farlink.h"



#define MAX_LINE		1024
#define MAX_FARLINKS		2048
#define MAX_NICSTAT		256
#define PORT_FIELD_WIDTH	(8)
#define MAX_PORT		(1 << PORT_FIELD_WIDTH)


/* Local functions */
static void set_defaults(void);
static int error_check(void);
static void assign_farlink_ports(void);

/* 
** Parameter variables. Locally global so I don't have to keep
** passing them back and forth (too many of them)
*/
static int _FarLinksCnt;

typedef struct FarLink_t   {
    int src_node;
    int src_port;
    int dest_node;
    int dest_port;
} FarLink_t;

static FarLink_t *_FarLinkList= NULL;

typedef struct linkcnt_t   {
    int node;
    int port_offset;
} linkcnt_t;

static linkcnt_t *_LinkCounts= NULL;
static int _LastNodeIndex;
static int _farlink_in_use= 0;



static void
set_defaults(void)
{

int i;


    if (_FarLinkList != NULL)   {
	free(_FarLinkList);
    }

    _FarLinkList= (FarLink_t *)malloc(sizeof(FarLink_t) * MAX_FARLINKS);
    if (_FarLinkList == NULL)   {
	fprintf(stderr, "Out of memory for _FarLinkList!\n");
	exit(-1);
    }

    for (i= 0; i < MAX_FARLINKS; i++)   {
	_FarLinkList[i].src_node= -1;
	_FarLinkList[i].src_port= -1;
	_FarLinkList[i].dest_node= -1;
	_FarLinkList[i].dest_port= -1;
    }

    if (_LinkCounts != NULL)   {
	free(_LinkCounts);
    }


}  /* end of set_defaults() */



static int
error_check(void)
{

int i;


    for (i= 0; i < _FarLinksCnt; i++)   {
	if ((_FarLinkList[i].src_node < 0) ||
		(_FarLinkList[i].dest_node < 0) ||
		(_FarLinkList[i].dest_port < 0) ||
		(_FarLinkList[i].src_port < 0))   {
	    fprintf(stderr, "Far link entry %d not fully specified: %d:%d - %d:%d\n",
		i, _FarLinkList[i].src_node, _FarLinkList[i].src_port,
		_FarLinkList[i].dest_node, _FarLinkList[i].dest_port);
	    return FALSE;
	}

	if (_FarLinkList[i].src_node == _FarLinkList[i].dest_node)   {
	    fprintf(stderr, "Far link entry %d wrong (same src/dest node): %d:%d - %d:%d\n",
		i, _FarLinkList[i].src_node, _FarLinkList[i].src_port,
		_FarLinkList[i].dest_node, _FarLinkList[i].dest_port);
	    return FALSE;
	}

	if (_FarLinkList[i].dest_port >= MAX_PORT)   {
	    fprintf(stderr, "Far link entry %d destination port too large: - %d:%d, max is %d\n",
		i, _FarLinkList[i].dest_node, _FarLinkList[i].dest_port, MAX_PORT);
	    return FALSE;
	}
    }

    /* No errors found */
    return TRUE;

}  /* end of error_check() */



void
disp_farlink_params(void)
{

int i;


    if (!_farlink_in_use)   {
	return;
    }

    printf("# *** Total number of far links is %d\n", _FarLinksCnt);

    for (i= 0; i < _FarLinksCnt; i++)   {
    }

}  /* end of disp_farlink_params() */



int
read_farlink_file(FILE *fp_farlink, int verbose)
{

int error;
char line[MAX_LINE];
char *pos;
int src_node, dest_node;
int rc;
int i;


    if (fp_farlink == NULL)   {
	return FALSE;
    }

    /* Defaults */
    error= FALSE;
    _farlink_in_use= 1;
    set_defaults();


    /* Process the input file */
    while (fgets(line, MAX_LINE, fp_farlink) != NULL)   {
	/* Get rid of comments */
	pos= strchr(line, '#');
	if (pos)   {
	    *pos= '\0';
	}

	/* Now scan it */
	rc= sscanf(line, "%d - %d", &src_node, &dest_node);
	if (rc != 2)   {
	    continue;
	}

	if (verbose > 1)   {
	    printf("Debug: Found %d - %d\n", src_node, dest_node);
	}

	_FarLinkList[_FarLinksCnt].src_node= src_node;
	_FarLinkList[_FarLinksCnt].dest_node= dest_node;

	if (++_FarLinksCnt >= MAX_FARLINKS)   {
	    error= TRUE;
	    fprintf(stderr, "Too many far links. Can't handle more than %d\n", MAX_FARLINKS);
	    break;
	}
    }

    if (error)   {
	return FALSE;
    }

    /* Can't be more than 2 * num links distinctive end nodes */
    _LinkCounts= (linkcnt_t *)malloc(sizeof(linkcnt_t) * 2 * _FarLinksCnt);
    if (_LinkCounts == NULL)   {
	fprintf(stderr, "Out of memory for _LinkCounts!\n");
	exit(-1);
    }
    _LastNodeIndex= -1;

    /*
    ** Add nodes with far links to our list
    ** Go through our list of links and count how many links each
    ** node has.
    */
    for (i= 0; i < _FarLinksCnt; i++)   {
	int j;
	int found;

	found= 0;
	for (j= 0; j <= _LastNodeIndex; j++)   {
	    if (_LinkCounts[j].node == _FarLinkList[i].src_node)   {
		/* _LinkCounts[j].num_links++; */
		found= 1;
		break;
	    }
	}

	if (!found)   {
	    /* Add the source endpoint */
	    _LastNodeIndex++;
	    _LinkCounts[_LastNodeIndex].node= _FarLinkList[i].src_node;
	    _LinkCounts[_LastNodeIndex].port_offset= 0;
	    /* _LinkCounts[_LastNodeIndex].num_links= 1; */
	}

	found= 0;
	for (j= 0; j <= _LastNodeIndex; j++)   {
	    if (_LinkCounts[j].node == _FarLinkList[i].dest_node)   {
		/* _LinkCounts[j].num_links++; */
		found= 1;
		break;
	    }
	}

	if (!found)   {
	    /* Add the destination endpoint */
	    _LastNodeIndex++;
	    _LinkCounts[_LastNodeIndex].node= _FarLinkList[i].dest_node;
	    _LinkCounts[_LastNodeIndex].port_offset= 0;
	    /* _LinkCounts[_LastNodeIndex].num_links= 1; */
	}
    }


    assign_farlink_ports();

#ifdef DEBUG
    for (i= 0; i <= 2 * _FarLinksCnt; i++)   {
	if (_LinkCounts[i].node < 0)   {
	    break;
	}
	printf("    Node %3d has %d links\n", _LinkCounts[i].node, _LinkCounts[i].port_offset);
    }
#endif

    return error_check();

}  /* end of read_farlink_file() */



static void
assign_farlink_ports(void)
{

int i, j;


    for (i= 0; i < _FarLinksCnt; i++)   {
	/* Find the source node and extract a port number */
	for (j= 0; j <= _LastNodeIndex; j++)   {
	    if (_FarLinkList[i].src_node == _LinkCounts[j].node)   {
		_FarLinkList[i].src_port= _LinkCounts[j].port_offset++;
		break;
	    }
	}
	/* Find the destination node and extract a port number */
	for (j= 0; j <= _LastNodeIndex; j++)   {
	    if (_FarLinkList[i].dest_node == _LinkCounts[j].node)   {
		_FarLinkList[i].dest_port= _LinkCounts[j].port_offset++;
		break;
	    }
	}
    }

} /* end of assign_farlink_ports() */



/*
** How many far links are attached to a node?
*/
int
numFarPorts(int node)
{

int i;


    if (!_farlink_in_use)   {
	return 0;
    }

    /* Find node */
    for (i= 0; i <= _LastNodeIndex; i++)   {
	if (node == _LinkCounts[i].node)   {
	    return _LinkCounts[i].port_offset;
	}
    }

    return 0;

}  /* end of numFarPorts() */



int
farlink_in_use(void)
{
    return _farlink_in_use;
}  /* end of farlink_in_use() */



/*
** Which outgoing port to use for this link?
*/
int
farlink_src_port(int src_node, int dest_node)
{

int i;

    /* Find node */
    for (i= 0; i < _FarLinksCnt; i++)   {
	if ((src_node == _FarLinkList[i].src_node) && (dest_node == _FarLinkList[i].dest_node))   {
	    return _FarLinkList[i].src_port;
	} else if ((src_node == _FarLinkList[i].dest_node) && (dest_node == _FarLinkList[i].src_node))   {
	    return _FarLinkList[i].dest_port;
	}
    }

    return -2;

}  /* end of numFarPorts() */



/*
** Which incoming port to use for this link?
*/
int
farlink_dest_port(int src_node, int dest_node)
{

int i;

    /* Find node */
    for (i= 0; i < _FarLinksCnt; i++)   {
	if ((src_node == _FarLinkList[i].src_node) && (dest_node == _FarLinkList[i].dest_node))   {
	    return _FarLinkList[i].dest_port;
	} else if ((src_node == _FarLinkList[i].dest_node) && (dest_node == _FarLinkList[i].src_node))   {
	    return _FarLinkList[i].src_port;
	}
    }

    return -3;

}  /* end of numFarPorts() */



/*
** Is there a far link between these two nodes?
*/
int
farlink_exists(int src_node, int dest_node)
{

int i;

    if (!_farlink_in_use)   {
	return FALSE;
    }

    /* Find node */
    for (i= 0; i < _FarLinksCnt; i++)   {
	if ((src_node == _FarLinkList[i].src_node) && (dest_node == _FarLinkList[i].dest_node))   {
	    return TRUE;
	} else if ((src_node == _FarLinkList[i].dest_node) && (dest_node == _FarLinkList[i].src_node))   {
	    return TRUE;
	}
    }

    return FALSE;

}  /* end of numFarPorts() */



int
numFarLinks(void)
{
    if (!_farlink_in_use)   {
	return 0;
    }

    return _FarLinksCnt;
}  /* end of numFarLinks() */



void
farlink_params(FILE *out, int port_offset)
{

int i;
uint64_t code;


    if (!_farlink_in_use)   {
	return;
    }

    fprintf(out, "    <FarLinkPortFieldWidth> %d </FarLinkPortFieldWidth>\n",
	    PORT_FIELD_WIDTH);

    for (i= 0; i < _FarLinksCnt; i++)   {
	code= _FarLinkList[i].dest_node << PORT_FIELD_WIDTH |
	    (_FarLinkList[i].dest_port + port_offset);

	fprintf(out, "    <FarLink%dp%d> %" PRId64 " </FarLink%dp%d>\n",
	    _FarLinkList[i].src_node, (_FarLinkList[i].src_port + port_offset),
	    code,
	    _FarLinkList[i].src_node, (_FarLinkList[i].src_port + port_offset));
    }

}  /* end of farlink_params() */
