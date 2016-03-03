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
** $Id: gen.h,v 1.7 2010/05/13 19:27:23 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#ifndef _GEN_H_
#define _GEN_H_

#define FALSE		(0)
#define TRUE		(1)

typedef enum {LOCAL_NVRAM, SSD} nvram_type_t;
typedef enum {Rnet, RNoC, RnetPort, Rnvram, Rstorage, RstoreIO} router_function_t;
typedef enum {Lnet, LNoC, LIO, LNVRAM, LnetAccess, LnetNIC, LNoCNIC} link_type_t;
/*
** Lnet		Link between network routers
** LNoC		Link between NoC routers
** LIO		
** LNVRAM
** LnetAccess	Link between network aggregator and network router
** LnetNIC	Link between NIC and network aggregator
** LNoCNIC	Link between NIC and NoC router (no aggregator)
*/

void gen_nic(int rank, int router, int port, int aggregator,
	int aggregator_port, int nvram, int nvram_port, int ss, int ss_port);
void gen_router(int id, int num_ports, router_function_t role, int wormhole,
	int mpi_rank);
void gen_link(int Arouter, int Aport, int Brouter, int Bport, link_type_t ltype);

void reset_router_list(void);
int next_router(int *id, router_function_t *role, int *wormhole,
	int *num_ports, int *mpi_rank);
void reset_router_nics(int router);
int next_router_nic(int router, int *port, link_type_t *ltype);
void reset_router_links(int router);
int next_router_link(int router, int *link_id, int *lport, int *rport,
	link_type_t *ltype);

void reset_nic_list(void);
int next_nic(int *id, int *router, int *port, int *mpi_rank,
	int *aggregator, int *aggregator_port,
	int *nvram, int *nvram_port, int *ss, int *ss_port, char **label);

void reset_router_nvram(int router);
void reset_nvram_list(void);
int next_nvram(int *id, int *router, int *port, int *mpi_rank,
	nvram_type_t *type);
void gen_nvram(int id, int router, int port, nvram_type_t type, int mpi_rank);
int next_router_nvram(int router, int *port);

#endif /* _GEN_H_ */
