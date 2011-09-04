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

void sst_header(FILE *sstfile);
void sst_footer(FILE *dotfile);

void sst_router_param_start(FILE *sstfile, char *Rname, int num_ports, uint64_t router_bw,
	int num_cores, int hop_delay, int wormhole, pwr_method_t power_method);
void sst_router_param_end(FILE *sstfile, char *Rname);

void sst_gen_param_start(FILE *sstfile, int gen_debug);
void sst_gen_param_entries(FILE *sstfile, int x_dim, int y_dim, int NoC_x_dim, int NoC_y_dim,
	int cores, int nodes, uint64_t net_lat,
        uint64_t net_bw, uint64_t node_lat, uint64_t node_bw, uint64_t compute_time,
	uint64_t app_time, int msg_len, char *method,
	uint64_t chckpt_interval, int envelope_size, int chckpt_size,
	char *pattern_name);
void sst_gen_param_end(FILE *sstfile, uint64_t node_latency, uint64_t net_latency);

void sst_pwr_param_entries(FILE *sstfile, pwr_method_t power_method);
void sst_pwr_component(FILE *sstfile, pwr_method_t power_method);

void sst_body_end(FILE *sstfile);
void sst_body_start(FILE *sstfile);

void sst_router_component_start(char *id, float weight, char *cname, router_function_t role,
	pwr_method_t power_method, FILE *sstfile);
void sst_router_component_end(pwr_method_t power_method, FILE *sstfile);
void sst_router_component_link(char *id, uint64_t link_lat, char *link_name, FILE *sstfile);

void sst_gen_component(char *id, char *net_link_id, char *net_aggregator_id,
	char *nvram_aggregator_id, char *ss_aggregator_id, float weight,
	int rank, char *pattern_name, FILE *sstfile);
void sst_pattern_generators(char *pattern_name, FILE *sstfile);
void sst_nvram_component(char *id, char *link_id, float weight, nvram_type_t type, FILE *sstfile);
void sst_nvram_param_entries(FILE *sstfile, int nvram_read_bw, int nvram_write_bw,
	int ssd_read_bw, int ssd_write_bw);
void sst_nvram(FILE *sstfile);
void sst_routers(FILE *sstfile, uint64_t node_latency, uint64_t net_latency,
	uint64_t nvram_latency, pwr_method_t power_method);

#endif /* _SST_GEN_H_ */
