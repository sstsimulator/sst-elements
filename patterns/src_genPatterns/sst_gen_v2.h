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
** $Id: sst_gen.h,v 1.9 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#ifndef _SST_GEN_H_
#define _SST_GEN_H_

#include <stdio.h>
#include <stdint.h>
#include "gen.h"


#define RNAME_NETWORK		"Rnet"
#define RNAME_NoC		"RNoC"
#define RNAME_NET_ACCESS	"RnetPort"
#define RNAME_NVRAM		"Rnvram"
#define RNAME_STORAGE		"Rstorage"
#define RNAME_IO		"RstoreIO"


typedef enum {pwrNone, pwrMcPAT, pwrORION} pwr_method_t;

void sst_header(FILE *sstfile, int partition);
void sst_footer(FILE *dotfile);

void sst_variables(FILE *sstfile, uint64_t IO_latency);

void sst_param_start(FILE *sstfile);
void sst_param_end(FILE *sstfile);

void sst_router_param_start(FILE *sstfile, char *Rname, int num_ports,
	int num_router_cores, int wormhole, pwr_method_t power_method);
void sst_router_param_end(FILE *sstfile, char *Rname);

void sst_gen_param_start(FILE *sstfile, int gen_debug);
void sst_gen_param_entries(FILE *sstfile);
void sst_gen_param_end(FILE *sstfile);

void sst_pwr_param_entries(FILE *sstfile, pwr_method_t power_method);
void sst_pwr_component(FILE *sstfile, pwr_method_t power_method);

void sst_body_end(FILE *sstfile);
void sst_body_start(FILE *sstfile);

void sst_router_component_start(char *id, char *cname, router_function_t role,
	int num_ports, pwr_method_t power_method, int mpi_rank, FILE *sstfile);
void sst_router_component_end(FILE *sstfile);
void sst_router_component_link(char *id, uint64_t link_lat, char *link_name, FILE *sstfile);

void sst_gen_component(char *id, char *net_link_id, char *net_aggregator_id,
	char *nvram_aggregator_id, char *ss_aggregator_id,
	int core_num, int mpi_rank, FILE *sstfile);
void sst_pattern_generators(FILE *sstfile);
void sst_nvram_component(char *id, char *link_id, nvram_type_t type,
	int mpi_rank, FILE *sstfile);
void sst_nvram_param_entries(FILE *sstfile, int nvram_read_bw, int nvram_write_bw,
	int ssd_read_bw, int ssd_write_bw);
void sst_nvram(FILE *sstfile);
void sst_routers(FILE *sstfile, uint64_t nvram_latency, pwr_method_t power_method);

#endif /* _SST_GEN_H_ */
