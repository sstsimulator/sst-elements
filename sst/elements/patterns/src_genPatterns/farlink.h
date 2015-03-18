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

#ifndef _FARLINK_H_
#define _FARLINK_H_


int read_farlink_file(FILE *fp_farlink, int verbose);
int farlink_in_use(void);
void disp_farlink_params(void);
int numFarPorts(int node);
int numFarLinks(void);
int farlink_dest_port(int src_node, int dest_node);
int farlink_src_port(int src_node, int dest_node);
int farlink_exists(int src_node, int dest_node);

void farlink_params(FILE *out, int port_offset);

#endif /* _FARLINK_H_ */
