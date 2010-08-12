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



void sst_header(FILE *sstfile);
void sst_footer(FILE *dotfile);

void sst_router_param_start(FILE *sstfile, int num_ports);
void sst_router_param_end(FILE *sstfile);

void sst_gen_param_start(FILE *sstfile, int gen_debug);
void sst_gen_param_entries(FILE *sstfile, int x_dim, int y_dim, uint64_t lat,
        uint64_t bw, uint64_t compute_time, int msg_len);
void sst_gen_param_end(FILE *sstfile);

void sst_body_end(FILE *sstfile);
void sst_body_start(FILE *sstfile);

void sst_router_component_start(char *id, float weight, char *cname, FILE *sstfile);
void sst_router_component_end(FILE *sstfile);
void sst_router_component_link(char *id, char *link_lat, char *link_name, FILE *sstfile);

void sst_gen_component(char *id, char *net_link_id, float weight, int rank,
    char *pattern_name, FILE *sstfile);
void sst_pattern_generators(char *pattern_name, FILE *sstfile);
void sst_routers(FILE *sstfile);

#endif /* _SST_GEN_H_ */
