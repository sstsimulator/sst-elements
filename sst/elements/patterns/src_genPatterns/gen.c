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
** $Id: gen.c,v 1.7 2010/05/13 19:27:22 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"

/* How many NICs per router max? */
/* FIXME: Some day I should make this dynamic */
#define MAX_NICS	(1024)

/* Max length of link label */
#define MAX_LABEL	(32)

/* No plans right now to attach more than one NVRAM to an aggregator */
#define MAX_NVRAM	(2)


typedef struct nic_t   {
    int rank;
    int router_id;
    int router_port;
    int net_aggregator_id;
    int net_aggregator_port;
    int nvram_aggregator_id;
    int nvram_aggregator_port;
    int ss_aggregator_id;
    int ss_aggregator_port;
    char label[MAX_LABEL];
    struct nic_t *next;
} nic_t;


typedef struct nvram_t   {
    int id;
    int router_id;
    int router_port;
    int mpi_rank;
    nvram_type_t type;
    struct nvram_t *next;
} nvram_t;


typedef struct link_t   {
    int id;
    int left_router;
    int right_router;
    int left_router_port;
    int right_router_port;
    char label[MAX_LABEL];
    link_type_t type;
    struct link_t *next;
} link_t;


typedef struct router_t   {
    int id;
    int num_ports;
    int next_nic;
    int next_link;
    int next_nvram;
    int wormhole;
    int mpi_rank;
    router_function_t role;
    nic_t *nics[MAX_NICS];
    nvram_t *nvram[MAX_NVRAM];
    link_t **links;
    struct router_t *next;
} router_t;


static router_t *router_current= NULL;
static router_t *router_list= NULL;
static router_t *router_list_end= NULL;

static nic_t *nic_current= NULL;
static nic_t *nic_list= NULL;
static nic_t *nic_list_end= NULL;

static int next_link_id= 0;
static link_t *link_list= NULL;
static link_t *link_list_end= NULL;

static nvram_t *nvram_current= NULL;
static nvram_t *nvram_list= NULL;
static nvram_t *nvram_list_end= NULL;



/*
** Add a Router
*/
void
gen_router(int id, int num_ports, router_function_t role, int wormhole, int mpi_rank)
{

router_t *current;
int i;


    if (num_ports < 1)   {
	fprintf(stderr, "Routers without ports are useless!\n");
	exit(9);
    }

    current= (router_t *)malloc(sizeof(router_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    /* FIXME: Why the + 1? */
    current->links= (link_t **)malloc(sizeof(link_t) * (num_ports + 1));
    if (current->links == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->id= id;
    current->num_ports= num_ports;
    current->wormhole= wormhole;
    current->role= role;
    current->mpi_rank= mpi_rank;

    current->next_nic= 0;
    for (i= 0; i < MAX_NICS; i++)   {
	current->nics[i]= NULL;
    }

    current->next_nvram= 0;
    for (i= 0; i < MAX_NVRAM; i++)   {
	current->nvram[i]= NULL;
    }

    current->next_link= 0;
    for (i= 0; i <= num_ports; i++)   {
	current->links[i]= NULL;
    }

    if (router_list_end)   {
	/* Append */
	router_list_end->next= current;
	router_list_end= current;
    } else   {
	/* First entry */
	router_list= current;
	router_list_end= current;
    }

}  /* end of gen_router() */



/*
** Find the router with this id and return a pointer to it.
*/
static router_t *
find_router(int id)
{

router_t *current;


    current= router_list;
    while (current)   {
	if (id == current->id)   {
	    return current;
	}
	current= current->next;
    }

    return NULL;

}  /* end of find_router() */



/*
** Reset the nic port pointer inside a router
*/
void
reset_router_nics(int router)
{

router_t *r;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before pattern generators.\n", router);
	exit(8);
    }
    r->next_nic= 0;

}   /* end of reset_router_nics() */



/*
** Traverse the list of ports connected to NICs inside a router
*/
int
next_router_nic(int router, int *port, link_type_t *ltype)
{

router_t *r;
nic_t *n;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before pattern generators.\n", router);
	exit(8);
    }

    if (r->next_nic >= MAX_NICS)   {
	return 0;
    }

    n= r->nics[r->next_nic];
    if (n == NULL)   {
	return 0;
    }

    if (n->router_id == router)   {
	/* This is a NoC router */
	*port= n->router_port;
	*ltype= LNoC; /* FIXME: This could also be LNoCNIC */
    } else if (n->net_aggregator_id == router)   {
	/* This is an Network aggregator */
	*port= n->net_aggregator_port;
	*ltype= LnetAccess;
    } else if (n->nvram_aggregator_id == router)   {
	/* This is an NVRAM aggregator */
	*port= n->nvram_aggregator_port;
	*ltype= LNVRAM;
    } else if (n->ss_aggregator_id == router)   {
	/* This is an Stable Stoage aggregator */
	*port= n->ss_aggregator_port;
	*ltype= LIO;
    } else   {
	fprintf(stderr, "Inconsistency: Router/aggregator link to a NIC with a link "
	    "to another router!\n");
	exit(10);
    }

    r->next_nic++;
    return 1;

}   /* end of next_router_nic() */



/*
** Reset the link port pointer inside a router
*/
void
reset_router_links(int router)
{

router_t *r;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before links.\n", router);
	exit(8);
    }
    r->next_link= 0;

}   /* end of reset_router_links() */



/*
** Traverse the list of ports connected to other routers
*/
int
next_router_link(int router, int *link_id, int *lport, int *rport, link_type_t *ltype)
{

router_t *r;
link_t *l;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before links.\n", router);
	exit(8);
    }

    if (r->next_link >= r->num_ports)   {
	return 0;
    }

    l= r->links[r->next_link];
    if (l == NULL)   {
	return 0;
    }

    if ((router != l->left_router) && (router != l->right_router))   {
	/* Something is wrong */
	fprintf(stderr, "Cannot find router %d on either end of this link!\n", router);
	exit(8);
    }

    if (router == l->left_router)   {
	*lport= l->left_router_port;
	*rport= l->right_router_port;
    } else   {
	*lport= l->right_router_port;
	*rport= l->left_router_port;
    }

    if (l->left_router == l->right_router)   {
	/* This link loops back to the same router. SST doesn't like that. */
	/* Return success, but mark it */
	*link_id= -1;
    } else   {
	*link_id= l->id;
    }

    *ltype= l->type;
    r->next_link++;
    return 1;

}   /* end of next_router_link() */



/*
** Add a NIC and the link to its router
*/
void
gen_nic(int rank, int router, int port, int aggregator, int aggregator_port,
    int nvram, int nvram_port, int ss, int ss_port)
{

nic_t *current;
router_t *r;
int i;


    current= (nic_t *)malloc(sizeof(nic_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->rank= rank;
    current->next= NULL;
    current->router_id= router;
    current->router_port= port;
    current->net_aggregator_id= aggregator;
    current->net_aggregator_port= aggregator_port;
    current->nvram_aggregator_id= nvram;
    current->nvram_aggregator_port= nvram_port;
    current->ss_aggregator_id= ss;
    current->ss_aggregator_port= ss_port;
    snprintf(current->label, MAX_LABEL, "-- R%d/p%d", router, port);

    /* It could be a simulation of a single core w/o a NoC */
    if (router >= 0)   {
	r= find_router(router);
	if (!r)   {
	    fprintf(stderr, "Cannot find router %d! Routers must be defined before "
		"pattern generators.\n", router);
	    exit(8);
	}

	/* Point the NoC router at this NIC */
	for (i= 0; i < MAX_NICS; i++)   {
	    if (r->nics[i] == NULL)   {
		/* Unused slot */
		r->nics[i]= current;
		break;
	    }
	}

	if (i >= MAX_NICS)   {
	    fprintf(stderr, "Out of NIC port slots! Cannot handle more than %d pattern "
		"generators per router.\n", MAX_NICS);
	    exit(8);
	}
    }



    /* Find and attach the Network aggregator, if there is one */
    if (aggregator >= 0)   {
	r= find_router(aggregator);
	if (!r)   {
	    fprintf(stderr, "Cannot find Network aggregator %d! Routers must be defined before "
		"pattern generators.\n", router);
	    exit(8);
	}

	/* Point the aggregator at this NIC */
	for (i= 0; i < MAX_NICS; i++)   {
	    if (r->nics[i] == NULL)   {
		/* Unused slot */
		r->nics[i]= current;
		break;
	    }
	}

	if (i >= MAX_NICS)   {
	    fprintf(stderr, "Out of NIC port slots on Network aggregator! Cannot handle more than "
		"%d pattern generators per router.\n", MAX_NICS);
	    exit(8);
	}
    }



    /* Find and attach the NVRAM aggregator, if there is one */
    if (nvram >= 0)   {
	r= find_router(nvram);
	if (!r)   {
	    fprintf(stderr, "Cannot find NVRAM aggregator %d! Routers must be defined before "
		"pattern generators.\n", router);
	    exit(8);
	}

	/* Point this NVRAM aggregator at this NIC */
	for (i= 0; i < MAX_NICS; i++)   {
	    if (r->nics[i] == NULL)   {
		/* Unused slot */
		r->nics[i]= current;
		break;
	    }
	}

	if (i >= MAX_NICS)   {
	    fprintf(stderr, "Out of NVRAM port slots on aggregator! Max is %d\n", MAX_NICS);
	    exit(8);
	}
    }



    /* Find and attach the Stable Storage aggregator, if there is one */
    if (ss >= 0)   {
	r= find_router(ss);
	if (!r)   {
	    fprintf(stderr, "Cannot find Stable Storage aggregator %d! Routers must be defined "
		"before pattern generators.\n", router);
	    exit(8);
	}

	/* Point the aggregator at this NIC */
	for (i= 0; i < MAX_NICS; i++)   {
	    if (r->nics[i] == NULL)   {
		/* Unused slot */
		r->nics[i]= current;
		break;
	    }
	}

	if (i >= MAX_NICS)   {
	    fprintf(stderr, "Out of NVRAM port slots on SS aggregator! Max is %d\n", MAX_NICS);
	    exit(8);
	}
    }



    /* Attach this NIC to our list of NICs */
    if (nic_list_end)   {
	/* Append */
	nic_list_end->next= current;
	nic_list_end= current;
    } else   {
	/* First entry */
	nic_list= current;
	nic_list_end= current;
    }

}  /* end of gen_nic() */



/*
** Add an NVRAM component
*/
void
gen_nvram(int id, int router, int port, nvram_type_t type, int mpi_rank)
{

nvram_t *current;
router_t *r;
int i;


    current= (nvram_t *)malloc(sizeof(nvram_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->id= id;
    current->next= NULL;
    current->router_id= router;
    current->router_port= port;
    current->type= type;
    current->mpi_rank= mpi_rank;

    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before "
	    "pattern generators.\n", router);
	exit(8);
    }

    /* Point the NVRAM aggregator at this bit bucket */
    for (i= 0; i < MAX_NVRAM; i++)   {
	if (r->nvram[i] == NULL)   {
	    /* Unused slot */
	    r->nvram[i]= current;
	    break;
	}
    }

    if (i >= MAX_NVRAM)   {
	fprintf(stderr, "Out of NVRAM port slots! Cannot handle more than %d NVRAM "
	    "per aggregator (node).\n", MAX_NVRAM);
	exit(8);
    }


    /* Attach this NVRAM to our list of NVRAMs */
    if (nvram_list_end)   {
	/* Append */
	nvram_list_end->next= current;
	nvram_list_end= current;
    } else   {
	/* First entry */
	nvram_list= current;
	nvram_list_end= current;
    }

}  /* end of gen_nvram() */



/*
** Add a link between routers
** Call this only once per link. It will hook into both routers, so
** no need to call it for each router individually!
** If you do call it for both ends, the second call will be ignored.
*/
void
gen_link(int Arouter, int Aport, int Brouter, int Bport, link_type_t ltype)
{

link_t *current;
int i;
router_t *A, *B;


    current= link_list;
    while (current)   {
	if ((current->left_router == Arouter) && (current->right_router == Brouter) &&
		(current->left_router_port == Aport) && (current->right_router_port == Bport))   {
	    /* A to B link already exists. No need to add it */
	    return;
	}
	if ((current->left_router == Brouter) && (current->right_router == Arouter) &&
		(current->left_router_port == Bport) && (current->right_router_port == Aport))   {
	    /* B to A link already exists. No need to add it */
	    return;
	}
	current= current->next;
    }



    current= (link_t *)malloc(sizeof(link_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->id= next_link_id++;
    current->type= ltype;
    current->left_router= Arouter;
    current->right_router= Brouter;
    current->left_router_port= Aport;
    current->right_router_port= Bport;
    snprintf(current->label, MAX_LABEL, "R%d/p%d -- R%d/p%d", Arouter, Aport, Brouter, Bport);

    A= find_router(Arouter);
    if (!A)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before pattern generators.\n", Arouter);
	exit(8);
    }

    B= find_router(Brouter);
    if (!B)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before pattern generators.\n", Brouter);
	exit(8);
    }

    for (i= 0; i < A->num_ports; i++)   {
	if (A->links[i] == NULL)   {
	    /* Unused slot */
	    A->links[i]= current;
	    break;
	}
    }

    if (i >= A->num_ports)   {
	fprintf(stderr, "Out of router port slots on source router!\n");
	exit(8);
    }

    for (i= 0; i < B->num_ports; i++)   {
	if (B->links[i] == NULL)   {
	    /* Unused slot */
	    B->links[i]= current;
	    break;
	}
    }

    if (i >= B->num_ports)   {
	fprintf(stderr, "Out of router port slots on destination router!\n");
	fprintf(stderr, "gen_link(Arouter %d, Aport %d, Brouter %d, Bport %d, ltype %d)\n",
	    Arouter, Aport, Brouter, Bport, (int)ltype);
	exit(8);
    }



    if (link_list_end)   {
	/* Append */
	link_list_end->next= current;
	link_list_end= current;
    } else   {
	/* First entry */
	link_list= current;
	link_list_end= current;
    }

}  /* end of gen_link() */



void
reset_router_list(void)
{
    router_current= router_list;
}  /* end of reset_router_list() */



int
next_router(int *id, router_function_t *role, int *wormhole, int *num_ports,
	int *mpi_rank)
{

    if (!router_current)   {
	return 0;
    }
    *id= router_current->id;
    *role= router_current->role;
    *wormhole= router_current->wormhole;
    *num_ports= router_current->num_ports;
    *mpi_rank= router_current->mpi_rank;
    router_current= router_current->next;
    return 1;

}  /* end of next_router() */



void
reset_nic_list(void)
{
    nic_current= nic_list;
}  /* end of reset_nic_list() */



int
next_nic(int *id, int *router, int *port, int *mpi_rank,
	int *aggregator, int *aggregator_port,
	int *nvram, int *nvram_port, int *ss, int *ss_port, char **label)
{

router_t *r;


    if (!nic_current)   {
	return 0;
    }

    if (mpi_rank != NULL)   {
	/* Dig up the mpi_rank of the router we are attached to */
	r= find_router(nic_current->router_id);
	if (r == NULL)   {
	    return 0;
	}
	*mpi_rank= r->mpi_rank;
    }

    *id= nic_current->rank;
    *router= nic_current->router_id;
    *port= nic_current->router_port;
    *aggregator= nic_current->net_aggregator_id;
    *aggregator_port= nic_current->net_aggregator_port;
    *nvram= nic_current->nvram_aggregator_id;
    *nvram_port= nic_current->nvram_aggregator_port;
    *ss= nic_current->ss_aggregator_id;
    *ss_port= nic_current->ss_aggregator_port;
    *label= nic_current->label;
    nic_current= nic_current->next;
    return 1;

}  /* end of next_nic() */



/* Reset the nvram port pointer inside a router */
void
reset_router_nvram(int router)
{

router_t *r;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d!\n", router);
	exit(8);
    }
    r->next_nvram= 0;

}   /* end of reset_router_nvram() */



void
reset_nvram_list(void)
{
    nvram_current= nvram_list;
}  /* end of reset_nvram_list() */



int
next_nvram(int *id, int *router, int *port, int *mpi_rank, nvram_type_t *type)
{

    if (!nvram_current)   {
	return 0;
    }

    *id= nvram_current->id;
    *router= nvram_current->router_id;
    *port= nvram_current->router_port;
    *mpi_rank= nvram_current->mpi_rank;
    *type= nvram_current->type;
    nvram_current= nvram_current->next;
    return 1;

}  /* end of next_nvram() */



/* Traverse the list of ports connected to NICs inside a router */
int
next_router_nvram(int router, int *port)
{

router_t *r;
nvram_t *n;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d for NVRAM access\n", router);
	exit(8);
    }

    if (r->next_nvram >= MAX_NVRAM)   {
	return 0;
    }

    n= r->nvram[r->next_nvram];
    if (n == NULL)   {
	return 0;
    }

    if (n->router_id == router)   {
	/* Found router */
	*port= n->router_port;
    } else   {
	fprintf(stderr, "Inconsistency: aggregator link to NVRAM!\n");
	exit(10);
    }

    r->next_nvram++;
    return 1;

}   /* end of next_router_nvram() */
