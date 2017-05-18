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
#include "Messier_Event.h"
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
NVM_DIMM::NVM_DIMM(SST::Component * owner, NVM_PARAMS par)
{

	cycles = 0;

	// Here we should do the assignment of parameters to the NVM_DIMM object

	Owner = owner;

	params = new NVM_PARAMS();

	(*params) = par; 

	histogram_idle = Owner->registerStatistic<uint64_t>( "histogram_idle");

	WB = new NVM_WRITE_BUFFER(params->write_buffer_size, 0, 64 /*write buffer granularity, now assume 64B */, params->flush_th, params->flush_th_low);

	ranks = new RANK*[params->num_ranks];	


	cacheline_interleave = params->cacheline_interleaving;

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

	if(cacheline_interleave)
		return (add/64)%(params->num_banks);
	else
		return (add/params->row_buffer_size)%(params->num_banks);

}



// This implements the clock ticking function

unsigned long int last_empty = 0;

bool NVM_DIMM::tick()
{


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



	// We start with checking if any read request is ready at NVM, to schdule reading it form the NVM Chip
	schedule_delivery();
	// Checking if there is any pending requests	
	if(!transactions.empty())
	{

		// Try to submit a request to a free bank and rank
		//submit_request();
		submit_request_opt();
	}

	// Here we should check if the write buffer is full or exceeds the threshold and try to flush at least a request
	if(WB->getSize() > 0)
		try_flush_wb();



	return false;

}


void NVM_DIMM::schedule_delivery()
{

	std::map<NVM_Request *, long long int>::iterator st_1, en_1;
	st_1 = ready_at_NVM.begin();
	en_1 = ready_at_NVM.end();

	while (st_1 != en_1)
	{

		bool ready = false;

		if(st_1->second <= cycles)
		{


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

				m_EventChan->send(params->tCMD + params->tCL + params->tBURST, new MessierEvent(st_1->first, EventType::READ_COMPLETION)); 
				ready_at_NVM.erase(st_1);
				break;		
			}

		}

		if(!ready)
			st_1++;

	}



}

// Note this is just to evaluate the second chance idea
BANK * NVM_DIMM::getFreeBank( long long int Address)
{

	long long int add = Address;
	for(int i=0; i < params->num_banks; i++)
		if(bank_hist[i]==0)
			if(ranks[WhichRank(add)]->getBank(i)->getBusyUntil() < cycles)
				return (ranks[WhichRank(add)])->getBank(i);

	return (ranks[WhichRank(add)])->getBank(WhichBank(add));


}

void NVM_DIMM::try_flush_wb()
{

	bool flush_write = false;

	int MAX_WRITES = params->max_writes;

	if(WB->flush() || (transactions.empty() && !WB->empty()))
		flush_write = true;

	if(flush_write)
	{	


		std::list<NVM_Request *> writes_list = WB->getList();

		std::list<NVM_Request *>::iterator st_wl, en_wl;

		st_wl = writes_list.begin();
		en_wl = writes_list.end();

		while(st_wl != en_wl)
		{

			NVM_Request * temp = *st_wl;


			long long int add = temp->Address;
			bool ready = false;

			BANK * temp_bank;
			temp_bank = getBank(temp->Address);

			if(temp_bank == NULL)
				return;

			if (getRank(add)->getBusyUntil() < cycles)
			{
				if(temp_bank->getBusyUntil() < cycles)
				{
					ready = true;
				}
			}
			// Occupy the rank for the write time
			if(ready && (MAX_WRITES > curr_writes) && ((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->write_weight)))
			{


				WB->erase_entry(temp);
				// Note that the rank will be busy for the time of sending the data to the bank, in addition to sending the command 
				getRank(add)->setBusyUntil(cycles + params->tCMD + params->tBURST);
				(temp_bank)->setBusyUntil(cycles + params->tCMD + params->tCL_W + params->tBURST);
				curr_writes++;
				WRITES_COMPLETE[cycles + params->tCMD + params->tCL_W + params->tBURST]++;
				break;
			}

			st_wl++;

		}

	}

}



bool NVM_DIMM::find_in_wb(NVM_Request * temp)
{
	bool removed = false;

	if(WB->find_entry(temp->Address)!=NULL)
	{
		removed = true;
		MemRespEvent *respEvent = new MemRespEvent(
				NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );
		m_memChan->send(respEvent); //(SST::Event *)NVM_EVENT_MAP[temp]);
		bank_hist[WhichBank(temp->Address)]--;
		delete NVM_EVENT_MAP[temp];
		NVM_EVENT_MAP.erase(temp);	
		delete temp;
	}
	return removed;
}


bool NVM_DIMM::row_buffer_hit(long long int add, long long int bank_add)
{


	if(cacheline_interleave)
	{
		if((add/(params->num_banks*params->row_buffer_size)) == bank_add/params->num_banks)	
			return true;
		else
			return false;

	}
	else // if bank interleaving (meaning each page goes to a specific bank), consecutive pages go to different banks
	{
		if((add/params->row_buffer_size) == bank_add)
			return true;
		else
			return false;
	}

}


bool NVM_DIMM::submit_request()
{


	NVM_Request * temp = transactions.front();  
	bool removed = false;

	// First check if this is a write request and the write buffer is not full 
	if(!temp->Read) // if write request
	{
		if(!WB->full())
		{
			NVM_Request * write_req = new NVM_Request();
			write_req->req_ID = 0;
			write_req->Read = false;
			write_req->Address = temp->Address;


			WB->insert_write_request(write_req);

			transactions.pop_front();
			MemRespEvent *respEvent = new MemRespEvent(
					NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );
			m_memChan->send(respEvent);			
			bank_hist[WhichBank(temp->Address)]--;
			delete NVM_EVENT_MAP[temp];
			NVM_EVENT_MAP.erase(temp);

			delete temp;
			removed = true;
		}

	}
	else  // if read request
	{
		// Check if in the write buffer
		removed = find_in_wb(temp);

		if(removed)
		{
			transactions.pop_front();
		}
		else
		{
			// First find out the corresponding bank to the read request and check if busy
			RANK * corresp_rank = getRank(temp->Address);
			BANK * corresp_bank = getBank(temp->Address);

			// Check if the rank is not busy
			if ((corresp_rank->getBusyUntil() < cycles) && (corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked() && ((long long int) outstanding.size() < params->max_outstanding))
			{
				long long int time_ready;
				// Check if row buffer hit
				bool issued=false;




				//histogram_idle->addData(WhichBank(temp->Address));

				if ( row_buffer_hit(temp->Address, corresp_bank->getRB()))
				{	
					//corresp_bank->setBusyUntil(cycles);
					time_ready = cycles;
					issued = true;
				}
				else if((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->read_weight)) 
				{
					// Allocate the Rank circuitary to submit the command
					corresp_rank->setBusyUntil(cycles + params->tCMD);
					// Set the bank busy until we read it
					corresp_bank->setBusyUntil(cycles + params->tCMD + params->tRCD);
					time_ready = cycles + params->tRCD + params->tCMD;
					curr_reads++;
					READS_COMPLETE[cycles + params->tRCD + params->tCMD]++;
					corresp_bank->setRB(temp->Address/params->row_buffer_size);
					issued = true;
				}

				if(issued)
				{
					outstanding.push_back(temp);
					transactions.pop_front();
					removed=true;
					// Lock the bank so no other request comes in and try to activate another row while waiting for the activation
					corresp_bank->setLocked(true, cycles);

					m_EventChan->send(time_ready-cycles, new MessierEvent(temp, EventType::DEVICE_READY));
				}
			}
		}
	}

	if(!removed)
	{
		transactions.push_back(transactions.front());
		transactions.pop_front();
	}

	return removed;
}


bool NVM_DIMM::pop_optimal()
{

	std::list<NVM_Request *>::iterator st, en;
	st = transactions.begin();
	en = transactions.end();

	long long time_ready;

	while(st!=en)
	{
		NVM_Request * temp = *st;

		RANK * corresp_rank = getRank(temp->Address);
		BANK * corresp_bank = getBank(temp->Address);
		if (temp->Read && (corresp_rank->getBusyUntil() < cycles) && (corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked() && ((long long int) outstanding.size() < params->max_outstanding))
		{

			//if((temp->Address/params->row_buffer_size) == (corresp_bank->getRB()))
			if ( row_buffer_hit(temp->Address, corresp_bank->getRB()))
			{	
				//corresp_bank->setBusyUntil(cycles);
				time_ready = cycles + 1;
				outstanding.push_back(temp);
				transactions.erase(st);
				// Lock the bank so no other request comes in and try to activate another row while waiting for the activation

				corresp_bank->setLocked(true, cycles);
				m_EventChan->send(time_ready-cycles, new MessierEvent(temp, EventType::DEVICE_READY));
				return true;
			}

		}

		st++;
	}

	return false;

}

long long int last_write=0;

bool NVM_DIMM::submit_request_opt()
{
	NVM_Request * temp; // = transactions.front();  
	bool removed = false;
	bool found = pop_optimal();
	if(found)
	{
		removed = true;

	}
	else
	{

		std::list<NVM_Request *>::iterator st, en;
		st = transactions.begin();
		en = transactions.end();

		//	long long time_ready;


		while(st!=en)
		{

			temp = *st;

			// First check if this is a write request and the write buffer is not full 
			removed = false;
			if(!temp->Read) // if write request
			{
				if(!WB->full())
				{
					//if(last_write!=0)
					//  histogram_idle->addData((cycles-last_write)/10);

					last_write = cycles;
					NVM_Request * write_req = new NVM_Request();
					write_req->req_ID = 0;
					write_req->Read = false;
					write_req->Address = temp->Address;


					WB->insert_write_request(write_req);

					transactions.erase(st);

					MemRespEvent *respEvent = new MemRespEvent(
							NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );
					m_memChan->send(respEvent);
					bank_hist[WhichBank(temp->Address)]--;

					delete NVM_EVENT_MAP[temp];

					NVM_EVENT_MAP.erase(temp);
					delete temp;
					removed = true;
					break;
				}
			}
			else  // if read request
			{
				// Check if in the write buffer


				removed = find_in_wb(temp);
				if(removed)
				{
					transactions.erase(st);
					break;
				}
				else //if(!removed)
				{
					// First find out the corresponding bank to the read request and check if busy
					RANK * corresp_rank = getRank(temp->Address);
					BANK * corresp_bank = getBank(temp->Address);

					// Check if the rank is not busy
					if ((corresp_rank->getBusyUntil() < cycles) && (corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked() && ((long long int) outstanding.size() < params->max_outstanding))
					{
						long long int time_ready;
						// Check if row buffer hit
						bool issued=false;
						//if((temp->Address/params->row_buffer_size) == (corresp_bank->getRB()))
						if ( row_buffer_hit(temp->Address, corresp_bank->getRB()))
						{	
							//corresp_bank->setBusyUntil(cycles);
							time_ready = cycles + 1;
							issued = true;
						}
						else if((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->read_weight)) 
						{

							//	histogram_idle->addData(WhichBank(temp->Address));

							// Allocate the Rank circuitary to submit the command
							corresp_rank->setBusyUntil(cycles + params->tCMD);
							// Set the bank busy until we read it
							corresp_bank->setBusyUntil(cycles + params->tCMD + params->tRCD);
							time_ready = cycles + params->tRCD + params->tCMD;
							curr_reads++;
							READS_COMPLETE[cycles + params->tRCD + params->tCMD]++;
							corresp_bank->setRB(temp->Address/params->row_buffer_size);
							issued = true;
						}
						if(issued)
						{
							outstanding.push_back(temp);
							transactions.erase(st);
							removed=true;
							// Lock the bank so no other request comes in and try to activate another row while waiting for the activation
							corresp_bank->setLocked(true, cycles);
							m_EventChan->send(time_ready-cycles, new MessierEvent(temp, EventType::DEVICE_READY));
							break;
						}
					}
				}
			}
			st++;
		}

	}


	return removed;
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




// This handles the events of the NVM_DIMM

void NVM_DIMM::handleEvent(SST::Event* e)
{


	MessierEvent tmp = *((MessierComponent::MessierEvent*) (e));

	if(tmp.ev == EventType::READ_COMPLETION)
	{
		NVM_Request * req = tmp.req;

		if(NVM_EVENT_MAP.find(req)!= NVM_EVENT_MAP.end())
		{
			NVM_Request * temp = req;
			MemRespEvent *respEvent = new MemRespEvent(
					NVM_EVENT_MAP[temp]->getReqId(), NVM_EVENT_MAP[temp]->getAddr(), NVM_EVENT_MAP[temp]->getFlags() );

			//	histogram_idle->addData(cycles - TIME_STAMP[temp]);
			TIME_STAMP.erase(temp);

			m_memChan->send((SST::Event *) respEvent);
			bank_hist[WhichBank(temp->Address)]--;
			delete NVM_EVENT_MAP[temp];
			NVM_EVENT_MAP.erase(req);
			delete e;

		}

		(getBank(req->Address))->setLocked(false, cycles);
		ready_trans.erase(req);
		outstanding.remove(req);

		delete req;

	} 
	else if (tmp.ev == EventType::DEVICE_READY)
	{

		NVM_Request * req = tmp.req;
		ready_at_NVM[req] = cycles;
		delete e;	

	}	


}



void NVM_DIMM::handleRequest(SST::Event* e)
{


	MessierComponent::MemReqEvent *event  = dynamic_cast<MessierComponent::MemReqEvent*>(e);

	NVM_Request * tmp = new NVM_Request();

	//TODO: ADD a map for NVM_Request to MemReqEvent

	if(!event->getIsWrite())
		tmp->Read = true;
	else
		tmp->Read = false;

	NVM_EVENT_MAP[tmp]= event;


	tmp->Size = event->getNumBytes();
	tmp->Address = event->getAddr() ;

	bank_hist[WhichBank(tmp->Address)]++;
	push_request(tmp);
	// Push the request
}

