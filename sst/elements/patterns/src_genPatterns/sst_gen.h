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



typedef enum {pwrNone, pwrMcPAT, pwrORION} pwr_method_t;

void sst_header(FILE *sstfile);
void sst_footer(FILE *dotfile);

void sst_router_param_start(FILE *sstfile, int num_ports, uint64_t router_bw,
	int num_cores, pwr_method_t power_method);
void sst_router_param_end(FILE *sstfile);

void sst_gen_param_start(FILE *sstfile, int gen_debug);
void sst_gen_param_entries(FILE *sstfile, int x_dim, int y_dim, int cores, uint64_t net_lat,
        uint64_t net_bw, uint64_t node_lat, uint64_t node_bw, uint64_t compute_time,
	uint64_t app_time, int msg_len, char *method, uint64_t chckpt_delay,
	uint64_t chckpt_interval, uint64_t envelope_write_time);
void sst_gen_param_end(FILE *sstfile, uint64_t node_latency);

void sst_pwr_param_entries(FILE *sstfile, pwr_method_t power_method);
void sst_pwr_component(FILE *sstfile, pwr_method_t power_method);

void sst_body_end(FILE *sstfile);
void sst_body_start(FILE *sstfile);

void sst_router_component_start(char *id, float weight, char *cname, FILE *sstfile);
void sst_router_component_end(FILE *sstfile);
void sst_router_component_link(char *id, uint64_t link_lat, char *link_name, FILE *sstfile);

void sst_gen_component(char *id, char *net_link_id, float weight, int rank,
    char *pattern_name, FILE *sstfile);
void sst_pattern_generators(char *pattern_name, FILE *sstfile);
void sst_routers(FILE *sstfile, uint64_t node_latency, uint64_t net_latency, pwr_method_t power_method);

#endif /* _SST_GEN_H_ */
