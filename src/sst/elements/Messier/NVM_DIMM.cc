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

using namespace SST::MemHierarchy;
using namespace SST;
using namespace SST::MessierComponent;

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

// This is the constructor of the NVM-based DIMM

int SIZE_DIST[10000];


int gs;
int lg;


NVM_DIMM::NVM_DIMM(SST::Component * owner, NVM_PARAMS par)
{

	// Here we should do the assignment of parameters to the NVM_DIMM object

	read_count = 1;

	group_locked = 0;

	cycles = 0;

	enabled = false;
	Owner = owner;

	params = new NVM_PARAMS();

	(*params) = par; 

	histogram_idle = Owner->registerStatistic<uint64_t>( "histogram_idle");
	reads = Owner->registerStatistic<uint64_t>( "reads");
	writes = Owner->registerStatistic<uint64_t>( "writes");

	WB = new NVM_WRITE_BUFFER(params->write_buffer_size, 0, 64 /*write buffer granularity, now assume 64B */, params->flush_th, params->flush_th_low);


	if(params->cache_enabled)
		cache = new NVM_CACHE(params->cache_size, params->cache_assoc, params->cache_latency, params->cache_bs);
	else
		cache = NULL;

	ranks = new RANK*[params->num_ranks];	


	cacheline_interleave = params->cacheline_interleaving;

	// Initializing each rank
	for ( int i = 0; i < params->num_ranks; i++)
	{
		ranks[i] = new RANK(params->num_banks);

	}

	curr_reads = 0;
	curr_writes = 0;

	gs = params->group_size;
	lg = group_locked;	

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


	if(!enabled)
		return false;	


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


	if(params->modulo)
	{

		if(!WB->empty())
		{
			// Here we should check if the write buffer is full or exceeds the threshold and try to flush at least a request



			if((read_count%params->modulo_unit)==0 || WB->flush())
			{
				if(try_flush_wb())
				{
					read_count++;

				}

			}
			else
			{
				// Checking if there is any pending requests	
				if(!transactions.empty()) 
				{

					// Try to submit a request to a free bank and rank
					if(submit_request_opt())
					{
						read_count++;

					}
				}

			}

		}
		else
		{
			//	std::cout<<Owner->getName()<<" Tries to submit "<<std::endl;
			submit_request_opt();
			read_count++;
		}

	}
	else
	{

		if(!transactions.empty())
		{
			submit_request_opt();
		}


		if(WB->getSize() > 0)
			try_flush_wb();

	}



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
				(getBank(add))->set_last(true);
				ready_trans[st_1->first] = cycles + params->tCMD + params->tCL + params->tBURST;
				(st_1->first)->meta_data = EventType::READ_COMPLETION;
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

bool NVM_DIMM::try_flush_wb()
{

	bool flush_write = false;

//	bool pull_idle = false;
	int MAX_WRITES = params->max_writes;

	if(WB->flush() || (transactions.empty() && !WB->empty()) || (params->modulo && !WB->empty()))
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
			bool removed = false;

			BANK * temp_bank;
			temp_bank = getBank(temp->Address);

			if(temp_bank == NULL)
			{
				std::cout<<"Errrror"<<std::endl;
				return false;

			}

			if (getRank(add)->getBusyUntil() < cycles)
			{
				if(temp_bank->getBusyUntil() < cycles)
				{
					ready = true;
				}
			}
			// Occupy the rank for the write time
			if((!params->adaptive_writes || (group_locked==(WhichBank(temp->Address)/params->group_size))) && ready && (MAX_WRITES > curr_writes) && ((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->write_weight)))
			{


				removed = true;
				WB->erase_entry(temp);
				// Note that the rank will be busy for the time of sending the data to the bank, in addition to sending the command 
				getRank(add)->setBusyUntil(cycles + params->tCMD + params->tBURST);
				(temp_bank)->setBusyUntil(cycles + params->tCMD + params->tCL_W + params->tBURST);
				temp_bank->set_last(false); // setting it to write
				temp_bank->set_last_address(temp->Address);
				curr_writes++;
				WRITES_COMPLETE[cycles + params->tCMD + params->tCL_W + params->tBURST]++;

				delete temp;

				return true;		

			}

			st_wl++;

		}

	}

	return false;

}



bool NVM_DIMM::find_in_wb(NVM_Request * temp)
{
	bool removed = false;

	if(WB->find_entry(temp->Address)!=NULL)
	{
		removed = true;
		MemRespEvent *respEvent = new MemRespEvent(
				NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

		if(SQUASHED.find(temp->req_ID)==SQUASHED.end())
		{

			m_memChan->send(respEvent); //(SST::Event *)NVM_EVENT_MAP[temp]);
			TIME_STAMP.erase(temp);


		}
		else
		{
			SQUASHED.erase(temp->req_ID);
			std::cout<<"Found something squashed " <<std::endl;
		}

		bank_hist[WhichBank(temp->Address)]--;
		delete NVM_EVENT_MAP[temp->req_ID];
		NVM_EVENT_MAP.erase(temp->req_ID);	
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


bool NVM_DIMM::pop_optimal()
{

	std::list<NVM_Request *>::iterator st, en;
	st = transactions.begin();
	en = transactions.end();

	long long time_ready;

	while(st!=en)
	{
		NVM_Request * temp = *st;
		if(SQUASHED.find(temp->req_ID)!=SQUASHED.end())
		{
			SQUASHED.erase(temp->req_ID);
			transactions.erase(st);
			delete NVM_EVENT_MAP[temp->req_ID];
			delete temp;
			break;
		}


		RANK * corresp_rank = getRank(temp->Address);
		BANK * corresp_bank = getBank(temp->Address);
		if ((!params->adaptive_writes || group_locked!=(WhichBank(temp->Address)/params->group_size)) &&  (HOLD.find(temp->req_ID)==HOLD.end()) && temp->Read && (corresp_rank->getBusyUntil() < cycles) && (corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked() && (outstanding.size() < params->max_outstanding))
		{

			if ( row_buffer_hit(temp->Address, corresp_bank->getRB()))
			{	
				time_ready = cycles + 1;
				outstanding.push_back(temp);
				transactions.erase(st);
				// Lock the bank so no other request comes in and try to activate another row while waiting for the activation

				corresp_bank->setLocked(true, cycles);
				temp->meta_data = EventType::DEVICE_READY;
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


		while(st!=en)
		{

			temp = *st;

			// First check if this is a write request and the write buffer is not full 
			removed = false;


			if(SQUASHED.find(temp->req_ID)!=SQUASHED.end())
			{
				SQUASHED.erase(temp->req_ID);
				transactions.erase(st);
				delete NVM_EVENT_MAP[temp->req_ID];
				delete temp;
				break;
			}


			if((!temp->Read))
			{
				if(!WB->full())
				{

					last_write = cycles;

					NVM_Request * write_req = new NVM_Request();
					write_req->req_ID = 0;
					write_req->Read = false;
					write_req->Address = temp->Address; 


					WB->insert_write_request(write_req); 
					transactions.erase(st);

					MemRespEvent *respEvent = new MemRespEvent(
							NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

					m_memChan->send(respEvent);
					bank_hist[WhichBank(temp->Address)]--;

					if(cache!=NULL)
						if(!cache->check_hit(temp->Address))
						{
							cache->insert_block(temp->Address, true);
							cache->update_lru(temp->Address);
						}


					delete NVM_EVENT_MAP[temp->req_ID];

					NVM_EVENT_MAP.erase(temp->req_ID);
					delete temp;
					removed = true;
					break;
				}
			}
			else  if(temp->Read)// if read request
			{
				// Check if in the write buffer

				if(HOLD.find(temp->req_ID)==HOLD.end())
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
					if ((!params->adaptive_writes || group_locked!=(WhichBank(temp->Address)/params->group_size)) && (HOLD.find(temp->req_ID)==HOLD.end()) &&   (corresp_rank->getBusyUntil() < cycles) && (((corresp_bank->getBusyUntil() < cycles) && !corresp_bank->getLocked()) || (params->write_cancel && !WB->flush() && !corresp_bank->read() &&(corresp_bank->getBusyUntil() - cycles < (100-4*WB->getSize())*1.0*params->tCL_W/100.0 ))) && (outstanding.size() < params->max_outstanding))
					{


						// If this comes here due to write cancellation: do the right business
						if(params->write_cancel &&  (corresp_bank->getBusyUntil() >= cycles) && !corresp_bank->read() && !WB->flush() && (corresp_bank->getBusyUntil() - cycles < (100-4*WB->getSize())*1.0*params->tCL_W/100.0 ))							
						{
						// Write cancellation business
						corresp_bank->setLocked(false, cycles);
						// Put the request back in the write buffer
						NVM_Request * evicted = new NVM_Request();
                                                evicted->req_ID = 0;
                                                evicted->Read = false;
                                                evicted->Address = corresp_bank->get_last_address();;

                                                WB->insert_write_request(evicted);

						}	


						long long int time_ready;
						// Check if row buffer hit
						bool issued=false;
						if ( row_buffer_hit(temp->Address, corresp_bank->getRB()))
						{	
							time_ready = cycles + 1;
							issued = true;
						}
						else if((params->write_weight*curr_writes + params->read_weight*curr_reads) <= (params->max_current_weight - params->read_weight)) 
						{


							// Allocate the Rank circuitary to submit the command
							corresp_rank->setBusyUntil(cycles + params->tCMD);
							// Set the bank busy until we read it
							corresp_bank->setBusyUntil(cycles + params->tCMD + params->tRCD);
							corresp_bank->set_last(true);
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
							temp->meta_data = EventType::DEVICE_READY;
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

void NVM_DIMM::handleEvent( SST::Event* e )
{



	MessierEvent * temp_ptr =  dynamic_cast<MessierComponent::MessierEvent*> (e);

	if(temp_ptr==NULL)
		std::cout<<" Error in Casting to MessierEvent "<<std::endl;

	MessierEvent tmp = *temp_ptr;

	NVM_Request * temp_req = tmp.getReq();

	tmp.setType(temp_req->meta_data);

	if(tmp.getType() == EventType::READ_COMPLETION)
	{
		NVM_Request * req = tmp.getReq();

		if(NVM_EVENT_MAP.find(req->req_ID)!= NVM_EVENT_MAP.end())
		{
			NVM_Request * temp = req;

			histogram_idle->addData((cycles - TIME_STAMP[temp])/1000);
			TIME_STAMP.erase(temp);
			if(SQUASHED.find(temp->req_ID)==SQUASHED.end())
			{
				MemRespEvent *respEvent = new MemRespEvent(
						NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

				m_memChan->send((SST::Event *) respEvent);


			}
			else
				SQUASHED.erase(temp->req_ID);


			if(cache!=NULL)
				if(!cache->check_hit(temp->Address))
				{
					if(!params->cache_persistent)
					{
						cache->insert_block(temp->Address, true);
						cache->update_lru(temp->Address);
					}
					else
					{
						bool dirty = cache->dirty_eviction(temp->Address);


						if(!dirty)
						{

							cache->insert_block(temp->Address, true);
							cache->update_lru(temp->Address);



						}
						else
						{

							long long int evicted_address = cache->check_hit(temp->Address);
							if(!WB->full())
							{
								last_write = cycles;

								NVM_Request * evicted = new NVM_Request();
								evicted->req_ID = 0;
								evicted->Read = false;
								evicted->Address = evicted_address;

								WB->insert_write_request(evicted);
								cache->insert_block(temp->Address, true);
								cache->update_lru(temp->Address);


							}						


						}
					}
					bank_hist[WhichBank(temp->Address)]--;
					delete NVM_EVENT_MAP[temp->req_ID];
					NVM_EVENT_MAP.erase(req->req_ID);
					delete e;

				}

			(getBank(req->Address))->setLocked(false, cycles);
			ready_trans.erase(req);
			outstanding.remove(req);
			delete req;

		}

	} 
	else if (tmp.getType() == EventType::DEVICE_READY)
	{

		NVM_Request * req = tmp.getReq();
		ready_at_NVM[req] = cycles;
		delete e;	

	}
	else if(tmp.getType() == EventType::HIT_MISS)
	{



		NVM_Request * req = tmp.getReq();
		NVM_Request * temp = req;
		if(NVM_EVENT_MAP.find(temp->req_ID)==NVM_EVENT_MAP.end())
		{

			delete e;
			return;
		}


		if(temp->Read)
		{	if(cache->check_hit(temp->Address))
			{

				MemRespEvent *respEvent = new MemRespEvent(
						NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

				m_memChan->send((SST::Event *) respEvent);
				cache->update_lru(temp->Address);
				if(params->cache_persistent)
					HOLD.erase(temp->req_ID);

				SQUASHED[temp->req_ID] = 1;


			}
			else
			{
				if(params->cache_persistent)
					HOLD.erase(temp->req_ID);

			}
		}
		else
		{

			if(params->cache_persistent)
			{
				bool dirty = cache->dirty_eviction(temp->Address);

				if(dirty)
				{

					long long int evicted_address = cache->check_hit(temp->Address);

					if(!WB->full())
					{
						last_write = cycles;

						NVM_Request * evicted = new NVM_Request();
						evicted->req_ID = 0;
						evicted->Read = false;
						evicted->Address = evicted_address;

						WB->insert_write_request(evicted); 

						MemRespEvent *respEvent = new MemRespEvent(
								NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

						m_memChan->send(respEvent);

						if(cache!=NULL) 
						{
							if(!cache->check_hit(temp->Address))
							{
								cache->insert_block(temp->Address, false);
								cache->update_lru(temp->Address);
							}
							else
							{
								cache->set_dirty(temp->Address);
								cache->update_lru(temp->Address);

							}
                        }

						delete NVM_EVENT_MAP[temp->req_ID];

						NVM_EVENT_MAP.erase(temp->req_ID);
					}
					else
					{
						m_EventChan->send(50, e); // Try after 50 cycles, to see if we got room in the write buffer to evict the dirty block
						return; //
					}
				}
				else
				{
					MemRespEvent *respEvent = new MemRespEvent(
							NVM_EVENT_MAP[temp->req_ID]->getReqId(), NVM_EVENT_MAP[temp->req_ID]->getAddr(), NVM_EVENT_MAP[temp->req_ID]->getFlags() );

					m_memChan->send(respEvent);

					if(cache!=NULL)
					{
						if(!cache->check_hit(temp->Address))
						{
							cache->insert_block(temp->Address, false);
							cache->update_lru(temp->Address);
						}
						else
						{
							cache->set_dirty(temp->Address);
							cache->update_lru(temp->Address);

						}
					}


					delete NVM_EVENT_MAP[temp->req_ID];

					NVM_EVENT_MAP.erase(temp->req_ID);




				}



			}

		}

		delete temp;
		delete e;


	}	
	else if(tmp.getType() == EventType::INVALIDATE_WRITE)
	{
		NVM_Request * req = tmp.getReq();
		cache->invalidate(req->Address);
		delete req;
		delete e;
	}

}



void NVM_DIMM::handleRequest(SST::Event* e)
{

	enabled = true;


	MessierComponent::MemReqEvent *event  = dynamic_cast<MessierComponent::MemReqEvent*>(e);

	NVM_Request * tmp = new NVM_Request();

	//TODO: ADD a map for NVM_Request to MemReqEvent

	if(!event->getIsWrite())
	{
		reads->addData(1);
		tmp->Read = true;

	}
	else
	{
		tmp->Read = false;
		writes->addData(1);

	}
	tmp->req_ID = event->getReqId();




	NVM_EVENT_MAP[event->getReqId()]= event;


	tmp->Size = event->getNumBytes();
	tmp->Address = event->getAddr() ;




	if(cache!=NULL)
	{

		NVM_Request * tmp2 = new NVM_Request();

		if(!event->getIsWrite())
			tmp2->Read = true;
		else
			tmp2->Read = false;

		tmp2->req_ID = event->getReqId();

		tmp2->Size = event->getNumBytes();
		tmp2->Address = event->getAddr() ;



		if(!tmp2->Read)
		{


			tmp2->meta_data = params->cache_persistent?EventType::HIT_MISS:EventType::INVALIDATE_WRITE;

			MessierEvent * mess = new MessierEvent(tmp2, params->cache_persistent?EventType::HIT_MISS:EventType::INVALIDATE_WRITE);
			m_EventChan->send(params->cache_persistent?params->cache_latency:1, mess);



		}
		else
		{
			// Hold servicing the request till we check the cache!
			if(params->cache_persistent)
				HOLD[tmp2->req_ID] = 1;

			tmp2->meta_data = EventType::HIT_MISS;
			m_EventChan->send(params->cache_latency, new MessierEvent(tmp2, EventType::HIT_MISS));

		}
	}

	bank_hist[WhichBank(tmp->Address)]++;

	// If write and the cache is peristent, just put it directly in the cache
	if(!(params->cache_persistent && (cache!=NULL) && (!tmp->Read)))
		push_request(tmp); // Push the request
}

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif
