// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Includes
#include<list>
#include<map>

// This defines a physical frame of size 4KB by default
class Frame{

public:
   // Constructor
   Frame() { starting_address = 0; metadata = 0;}

   // Constructor with paramteres
   Frame(long long int st, long long int md) { starting_address = st; metadata = 0;}

   // The starting address of the frame
   long long int starting_address;

   // This will be used to store information about current allocation
   int metadata;

};


// This class defines a memory pool

class Pool{

public:
   //Constructor for pool
   Pool(long long int st1, long long int size1, int framesize);

   // The size of the memory pool in KBs
   long long int size;

   // The starting address of the memory pool
   long long int start;

   // Allocate N contigiuous frames, returns the starting address if successfull, or -1 if it fails!
   long long int allocate_frame(int N);

   // Freeing N frames starting from Address X, this will return -1 if we find that these frames were not allocated
   int deallocate_frame(long long int X, int N);

   // Current number of free frames
   int freeframes() { return freelist.size(); }

   // Frame size in KBs
   int frsize;

private:
   // The list of free frames
   std::list<Frame*> freelist;

   // The list of allocated frames --- the key is the starting physical address
   std::map<long long int, Frame*> alloclist;


};
