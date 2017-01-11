// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include <cstddef>
#include<iostream>
#include<list>
#include "Rank.h"
#include "WriteBuffer.h"
#include "NVM_DIMM.h"
#include "memReqEvent.h"
//using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST;
using namespace SST::MessierComponent;


// Here we hardcode the cache block size
int cache_block_size = 64;

// Here we hardcod the page size
int page_size = 4096;

// This is the constructor of the NVM-based DIMM
NVM_DIMM::NVM_DIMM(NVM_PARAMS par)
{

	// Here we should do the assignment of parameters to the NVM_DIMM object

	params = new NVM_PARAMS();

	(*params) = par; 

	WB = new NVM_WRITE_BUFFER(params->write_buffer_size, 0, 64 /*write buffer granularity, now assume 64B */, params->flush_th);

	ranks = new RANK*[params->num_ranks];	

	// Initializing each rank
	for ( int i = 0; i < params->num_ranks; i++)
	{
		ranks[i] = new RANK(params->num_banks);

	}

	curr_reads = 0;
	curr_writes = 0;

}


// To maximize performance, we will assume that consecutive row numbers be located in different ranks
int NVM_DIMM::WhichRank(long long int add)
{

	return (add/params->row_buffer_size)%(params->num_ranks);

}



int NVM_DIMM::WhichBank(long long int add)
{

	return (add/params->row_buffer_size)%(params->num_banks);

}

// This implements the clock ticking function
bool NVM_DIMM::tick()
{


//	std::cout<<"Tikcing the NVDIMM "<<std::endl;
	// Incrementing the cycles count
	cycles++;



	if(READS_COMPLETE.find(cycles)!=READS_COMPLETE.end())
	{
		curr_reads = curr_reads - READS_COMPLETE[cycles];
		READS_COMPLETE.erase(cycles);
	}

	if(WRITES_COMPLETE.find(cycles)!=WRITES_COMPLETE.end())
	{
		curr_writes = curr_writes - WRITES_COMPLETE[cycles];
		WRITES_COMPLETE.erase(cycles);
	}



	// Iterating over the requests currently being brough from PCM chips to see if anyone is ready
	std::map<NVM_Request *, long long int>::iterator st, en;
	st = ready_trans.begin();
	en = ready_trans.end();

	while (st != en)
	{

		bool deleted = false;
		long long int add = (st->first)->Address;
		if(st->second <= cycles)
		{

//			completed_requests.push_back(st->first);	

			if(NVM_EVENT_MAP.find(st->first)!= NVM_EVENT_MAP.end())
			{
			NVM_Request * temp = st->first;
			MemRespEvent *respEvent = new MemRespEvent(
                                NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );

			m_memChan->send((SST::Event *) respEvent);

			NVM_EVENT_MAP.erase(st->first);
			}
			else
			std::cout<<"Couldn't find the event "<<std::endl;

			(getBank(add))->setLocked(false);
			ready_trans.erase(st);
			outstanding.remove(st->first);
			deleted = false;
			break;
		}
		if(!deleted)
			st++;

	}


	// We start with checking if any read request is ready at NVM, to schdule reading it form the NVM Chip
	std::map<NVM_Request *, long long int>::iterator st_1, en_1;
	st_1 = ready_at_NVM.begin();
	en_1 = ready_at_NVM.end();

	while (st_1 != en_1)
	{

		bool ready = false;

		if(st_1->second <= cycles)
		{

//				std::cout<<"Something is ready at the NVM Chips"<<std::endl;

			bool ready = false;
			// Check if the bank and rank are free to submit the command there
			long long int add = (st_1->first)->Address;
			if (getRank(add)->getBusyUntil() < cycles)
			{
				if(getBank(add)->getBusyUntil() < cycles)
					ready = true;
			}

			if(ready) // This means that the request is ready and the data is ready to be ready by internal controller
			{

				// Occuping the rank and back for reading the ready data
				getRank(add)->setBusyUntil(cycles + params->tCMD + params->tCL + params->tBURST);
				(getBank(add))->setBusyUntil(cycles + params->tCMD + params->tCL + params->tBURST);
				ready_trans[st_1->first] = cycles + params->tCMD + params->tCL + params->tBURST;
				//std::cout<<"The request was scheduled to be read at "<<ready_trans[st_1->first]<<std::endl;
				ready_at_NVM.erase(st_1);
				break;		
				//st_1 = ready_at_NVM.begin();
				//		en_1 = ready_at_NVM.end();	
			}

		}

		if(!ready)
			st_1++;

	}
	// Here we should check if the write buffer is full or exceeds the threshold and try to flush at least a request
	if(WB->flush())
	{	
		NVM_Request * temp = WB->getFront();
		long long int add = temp->Address;
		bool ready = false;

		if (getRank(add)->getBusyUntil() < cycles)
		{
			if(getBank(add)->getBusyUntil() < cycles)
			{ready = true;
				WB->pop_entry();
				//	transactions.remove(temp);
			}
		}
		// Occupy the rank for the write time
		if(ready && ((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->write_weight)))
		{
			// Note that the rank will be busy for the time of sending the data to the bank, in addition to sending the command 
			getRank(add)->setBusyUntil(cycles + params->tCMD + params->tBURST);
			(getBank(add))->setBusyUntil(cycles + params->tCMD + params->tCL_W + params->tBURST);
			curr_writes++;
			WRITES_COMPLETE[cycles + params->tCMD + params->tCL_W + params->tBURST]++;
		
			MemRespEvent *respEvent = new MemRespEvent(
                                NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );

			m_memChan->send(respEvent);
			NVM_EVENT_MAP.erase(temp);

		}

	}


	// Checking if there is any pending requests	
	if(!transactions.empty())
	{
		//std::cout<<"Messier received a command :) :) "<<std::endl;

		NVM_Request * temp = transactions.front();  
		bool removed = false;

		// First check if this is a write request and the write buffer is not full 
		if(!temp->Read) // if write request
		{
			if(!WB->full())
			{
				std::cout<<"Write buffer is not full "<<std::endl;
				WB->insert_write_request(temp); 
				transactions.pop_front();
				removed = true;
			}
			else
				std::cout<<"The write buffer is full :( "<<std::endl;

		}
		else  // if read request
		{

			// Check if in the write buffer
			if(WB->find_entry(temp->Address)!=NULL)
			{
				removed = true;
				transactions.pop_front();
			//	completed_requests.push_back(temp);
			//	NVM_EVENT_MAP[temp]->makeResponse(this);
				//MemEvent *event  = dynamic_cast<MemEvent*>(NVM_EVENT_MAP[temp]);
				//event->makeResponse();
				  MemRespEvent *respEvent = new MemRespEvent(
                                NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );
				m_memChan->send(respEvent); //(SST::Event *)NVM_EVENT_MAP[temp]);
				NVM_EVENT_MAP.erase(temp);	
			}

			// Check for power budget of current simultaneous activations/reads/writes on the NVM chips

			//TODO

			else
			{

				// First find out the corresponding bank to the read request and check if busy
				RANK * corresp_rank = getRank(temp->Address);
				BANK * corresp_bank = getBank(temp->Address);

				// Check if the rank is not busy
				if ((corresp_rank->getBusyUntil() < cycles) && (corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked() && (outstanding.size() < params->max_outstanding))
				{

					// DEBUG
					//std::cout<<"We could submit the request"<<std::endl;
					// Allocate the Rank circuitary to submit the command
					corresp_rank->setBusyUntil(cycles + params->tCMD);

					long long int time_ready;
					// Check if row buffer hit
					if((temp->Address/params->row_buffer_size) == (corresp_bank->getRB()))
					{	
						std::cout<<"Row buffer hit :) "<<std::endl;
						//corresp_bank->setBusyUntil(cycles);
						time_ready = cycles;

					}
					else if((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->read_weight)) 
					{


						std::cout<<"Row buffer miss :( "<<std::endl;
						// Allocate the Rank circuitary to submit the command
						corresp_rank->setBusyUntil(cycles + params->tCMD);
						// Set the bank busy until we read it
						corresp_bank->setBusyUntil(cycles + params->tCMD + params->tRCD);
						time_ready = cycles + params->tRCD + params->tCMD;
						curr_reads++;
						READS_COMPLETE[cycles + params->tRCD + params->tCMD]++;
						corresp_bank->setRB(temp->Address/params->row_buffer_size);

					}

					outstanding.push_back(temp);
					transactions.pop_front();
					removed=true;
					// Lock the bank so no other request comes in and try to activate another row while waiting for the activation

					corresp_bank->setLocked(true);
					//std::cout<<"The time this is ready is "<<time_ready<<std::endl;
					ready_at_NVM[temp] = time_ready;


				}
			}
		}

		if(!removed)
		{
			transactions.push_back(transactions.front());
			transactions.pop_front();
		}


	}


	return false;

}


NVM_Request * NVM_DIMM::pop_request()
{


	if(completed_requests.size() >= 1)
	{
		NVM_Request * temp = completed_requests.front();
		completed_requests.pop_front();
		return temp;
	}
	else
	{
		return NULL;
	}

}

void NVM_DIMM::handleRequest(SST::Event* e)
{


	MemReqEvent *event  = dynamic_cast<MemReqEvent*>(e);

	NVM_Request * tmp = new NVM_Request();

	//TODO: ADD a map for NVM_Request to MemReqEvent

	if(!event->getIsWrite())
		tmp->Read = true;
	else
		tmp->Read = false;

	NVM_EVENT_MAP[tmp]= event;

	std::cout<<"The address is "<<event->getAddr()<<std::endl;

	tmp->Size = event->getNumBytes();
	tmp->Address = event->getAddr() ;
	push_request(tmp);
        // Push the request
}

