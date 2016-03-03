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
** Work stealing benchmark
*/

#ifndef _WS_H_
#define _WS_H_


extern int my_rank;
extern int num_ranks;


int ws_init(int argc, char *argv[]);
double ws_work(void);
void ws_print(void);


#endif  /* _WS_H_ */
