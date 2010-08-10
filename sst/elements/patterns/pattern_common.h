/*
** Copyright 2009-2010 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
** 
** Copyright (c) 2009-2010, Sandia Corporation
** All rights reserved.
** 
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

#ifndef _PATTERN_COMMON_H
#define _PATTERN_COMMON_H

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#include <sst_config.h>


// Events among pattern generators
typedef enum {START, COMPUTE_DONE, RECEIVE, FAIL, RESEND_MSG} pattern_event_t;

class Patterns   {
    public:
	Patterns()   {
	    // Nothing to do for now
	}

	int init(int x, int y, int my_rank);
	void send(int dest, int len);
	// void event_send(int dest, pattern_event_t event, double delay);


    private:

} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
