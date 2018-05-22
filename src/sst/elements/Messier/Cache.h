// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#ifndef _H_SST_MESSIER_CACHE
#define _H_SST_MESSIER_CACHE

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>

#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include<iostream>

using namespace std;
using namespace SST;

namespace SST{ namespace MessierComponent {

	// This defines the internal inside the NVM-based DIMM memory
	class NVM_CACHE
	{


		int size; // The size in KB

		int latency; // Latency

		int assoc; // Associativity

		int bs; // This is the block size of the internal cache

		int ** lru; // This indicates the LRU information

		bool ** valid;

		bool ** dirty;

		long long int ** tag_array; // This indicates the tag array

		long long int num_sets; // This inidicates the number of sets

		public:


		NVM_CACHE(int Size, int Assoc, int Latency, int BS)
		{

			size = Size;
			assoc = Assoc;
			latency = Latency;
			bs = BS;

			num_sets = ((long long int) size*1024)/(bs*assoc); // Assigning the number of sets

			tag_array = new long long int *[num_sets];
			lru =  new int * [num_sets];
			valid = new bool * [num_sets];
			dirty = new bool * [num_sets];

			for(int i = 0; i < num_sets; i++)
			{
				tag_array[i] = new long long int[assoc];
				lru[i] = new int [assoc];
				valid[i] = new bool [assoc];
				dirty[i] = new bool [assoc];

				for(int j=0; j<assoc; j++)
				{
					tag_array[i][j] = -1;
					lru[i][j] = j;
					valid[i][j] = false;
					dirty[i][j] = false;
				}		


			}



		}

		// Check if a hit or miss
		bool check_hit(long long int add)
		{

			long long int set = (add/bs)%num_sets;

			for(int i=0; i < assoc; i++)
				if(valid[set][i] && (tag_array[set][i]/bs == add/bs))
					return true;

			return false;

		}


		// This adds the block to the cache
		long long int insert_block(long long int add, bool clean)
		{

			long long int set = (add/bs)%num_sets;
			long long int evicted = 0;

			for(int i=0; i < assoc; i++)
				if(lru[set][i]==(assoc-1))
				{
					evicted = tag_array[set][i];
					valid[set][i] = true;
					tag_array[set][i] = add;
					dirty[set][i] = !clean;
					break;
				}

			return evicted;

		}

	 	bool dirty_eviction(long long int add)
                {

                        long long int set = (add/bs)%num_sets;

                        for(int i=0; i < assoc; i++)
                                if(lru[set][i]==(assoc-1))
                                {
                                     //   valid[set][i] = true;
                                      //  tag_array[set][i] = add;
                                     if(dirty[set][i] && valid[set][i])
					return true;
				     else
					return false;
                                }

			return false;

                }


		void set_dirty(long long int add)
		{


			long long int set = (add/bs)%num_sets;
			for(int i=0; i < assoc; i++)
                        {

                                if(tag_array[set][i]/bs==add/bs)
                                {
					dirty[set][i] = true;
                                        //lru_pos = lru[set][i];
                                        //lru_ind  = i;
                                        break;
                                }
                        }


		}


		// This function updates the LRU position of the block
		void update_lru(long long int add)
		{

			long long int set = (add/bs)%num_sets;

			int lru_pos = -1;
			int lru_ind = -1;

			for(int i=0; i < assoc; i++)
			{

				if(tag_array[set][i]/bs==add/bs)
				{
					lru_pos = lru[set][i];
					lru_ind  = i;
					break;
				}
			}

			if(lru_pos == -1)
				return;


			for(int i=0; i < assoc; i++)
				if(lru[set][i] < lru_pos)
					lru[set][i]++;

			lru[set][lru_ind] = 0;


		}

		// This function invalidates a cache block
		void invalidate(long long int add)
		{

			long long int set = (add/bs)%num_sets;

			for(int i=0; i < assoc; i++)
			{

				if(tag_array[set][i]/bs==add/bs)
				{
					valid[set][i] = false;
					break;
				}
			}

		}
	};


}}
#endif
