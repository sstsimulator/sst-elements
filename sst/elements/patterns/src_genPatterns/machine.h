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

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <stdint.h> 



int read_machine_file(FILE *fp_machine, int verbose);
void disp_machine_params(void);

int Net_x_dim(void);
int Net_y_dim(void);
int Net_z_dim(void);
int NoC_x_dim(void);
int NoC_y_dim(void);
int NoC_z_dim(void);

int Net_x_wrap(void);
int Net_y_wrap(void);
int Net_z_wrap(void);
int NoC_x_wrap(void);
int NoC_y_wrap(void);
int NoC_z_wrap(void);

int num_net_routers(void);
int num_nodes(void);
int num_NoC_routers(void);
int num_cores(void);
int num_router_nodes(void);
int num_router_cores(void);
int NetNICgap(void);
int NoCNICgap(void);

int NetNICinflections(void);
int NoCNICinflections(void);
int NetRtrinflections(void);
int NoCRtrinflections(void);
int NetNICinflectionpoint(int index);
int NoCNICinflectionpoint(int index);
int NetRtrinflectionpoint(int index);
int NoCRtrinflectionpoint(int index);
int64_t NetNIClatency(int index);
int64_t NoCNIClatency(int index);
int64_t NetRtrlatency(int index);
int64_t NoCRtrlatency(int index);

int64_t NetLinkBandwidth(void);
int64_t NoCLinkBandwidth(void);
int64_t NetLinkLatency(void);
int64_t NoCLinkLatency(void);
int64_t IOLinkBandwidth(void);
int64_t IOLinkLatency(void);

void machine_params(FILE *out);

#endif /* _MACHINE_H_ */
