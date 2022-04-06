// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SST includes
#include <sst_config.h>

// Other Includes
#include "mempool.h"


//Constructor for pool
Pool::Pool(long long int st1, long long int size1, int framesize)
{
   start = st1;
   size = size1;
   frsize = framesize;

   // Create free frames of size framesize, note that the size is in KB
   for(int i=0; i< size/framesize; i++) {
      freelist.push_back(new Frame(((long long int) i*frsize*1024) + start, 0));
   }
}

// Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
long long int Pool::allocate_frame(int N)
{

   // Make sure we have free frames first
   if(freelist.empty())
      return -1;

   // For now, we will assume you can only allocate 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating contigous physical spaces
   if(N>=1) {
         return -1;
   } else {
         // Simply, pop the first free frame and assign it
         Frame * temp = freelist.front();
         freelist.pop_front();
         alloclist[temp->starting_address] = temp;

         return temp->starting_address;
   }
}


// Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
int Pool::deallocate_frame(long long int X, int N)
{
   // For now, we will assume you can free only 1 frame, TODO: We will implemenet a buddy-allocator style that enables allocating and freeing contigous physical spaces
   if( N >= 1 ) {
      return -1;
   } else {
         // If we can find the frame to be free in the allocated list
         if(alloclist.find(X)!=alloclist.end()) {
            // Remove from allocation map and add to free list
            Frame * temp = alloclist[X];
            freelist.push_back(temp);
            alloclist.erase(X);
         // Means we couldn't find an allocated frame that is being unmapped
         } else {
            return -1;
         }
   }

   return 1;
}

