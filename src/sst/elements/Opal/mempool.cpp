// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


/* Author: Amro Awad
 * E-mail: amro.awad@ucf.edu
 */

#include "mempool.h"


//Constructor for pool
Pool::Pool(long long int st1, long long int size1)
{

	start = st1;
	size = size1;
	
	// Create free frames of size 4KB, note that the size is in KB
	for(int i=0; i< size/4; i++)
		freelist.push_back(new Frame(((long long int) i*4096) + start, 0));



}

// Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
long long int Pool::allocate_frame(int N)
{




}


// Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
int Pool::deallocate_frame(long long int X, int N)
{







}

