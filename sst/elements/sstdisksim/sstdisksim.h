// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_H
#define _SSTDISKSIM_H

#include <sst/log.h>
#include <sst/core/eventFunctor.h>
#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <stdlib.h>
#include <stddef.h>

#include "syssim_driver.h"
#include <disksim_interface.h>
#include <disksim_rand48.h>

using namespace std;
using namespace SST;

#ifndef DISKSIM_DBG
#define DISKSIM_DBG 0
#endif

enum blocktype {READ, WRITE};

typedef	struct	{
  int n;
  double sum;
  double sqr;
} sstdisksim_stat;

class sstdisksim : public Component {

    public: // functions

        sstdisksim( ComponentId_t id, Params_t& params );
	~sstdisksim();
        int Finish();

    private: // types
	sstdisksim_stat __disksim_stat;
	struct disksim_interface* __disksim;
	Simulation* __sim;
	Params_t __params;
	ComponentId_t __id;

	//	SysTime __now;		/* current time */
	//	SysTime __next_event;	/* next event */
	//	int __completed;	/* last request was completed */
	//	sstdisksim_stat __st;
	Log< DISKSIM_DBG >&  m_dbg;

    private: // functions

        sstdisksim( const sstdisksim& c );
        bool clock( Cycle_t cycle );

        void readBlock(unsigned id, uint64_t addr, uint64_t clockcycle);
        void writeBlock(unsigned id, uint64_t addr, uint64_t clockcycle);

	/*	void panic(const char* s);
	void print_statistics(sstdisksim_stat *s, const char *title);
	void add_statistics(sstdisksim_stat *s, double x);
	void syssim_schedule_callback(disksim_interface_callback_t fn, 
				      SysTime t, 
				      void *ctx);
	void syssim_deschedule_callback(double t, void *ctx);
	void syssim_report_completion(SysTime t, struct disksim_request *r, void *ctx);*/
};

#endif // _SSTDISKSIM_H
