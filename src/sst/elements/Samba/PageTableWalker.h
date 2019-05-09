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


#ifndef _H_SST_PTW
#define _H_SST_PTW

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/link.h>
#include <sst/core/event.h>
#include<map>
#include<vector>
#include <sst/core/sst_types.h>
// This file defines the page table walker and 

typedef std::pair<uint64_t, int> id_type;
typedef uint64_t Address_t;

using namespace SST::Interfaces;
using namespace SST;

namespace SST { namespace SambaComponent{

	class PageReferenceTable;

	class PageTableWalker
	{

		Output* output;

		int coreId;

		int fault_level; // indicates the step where the page fault handler is at

		int sizes; // This indicates the number of sizes supported

		uint64_t * page_size; // By default, lets assume 4KB pages

		int * assoc; // This represents the associativiety

		Address_t *** tags; // This will hold the tags

		bool *** valid; // This will hold the status of tags

		int *** lru; // This will hold the lru positions

		SST::Link * to_mem; // This links the Page table walker to the memory hierarchy

		SST::Link * to_opal; // This links the Page table walker to the opal memory manager

		uint32_t opal_latency; // network latency from ptw to opal

		int * hold; // This is used to tell the TLB hierarchy to stall, to emulate overhead of page fault handler

		int * shootdown; // This is used to indicate TLB hierarchy that TLB shootdown from other cores is in progress

		int shootdownId;

		int num_pages_migrated;

		//std::vector<Address_t, int> buffer;
		int *hasInvalidAddrs;

		std::vector<std::pair<Address_t, int> > * invalid_addrs; // This is used to store address invalidation requests.

		uint64_t tlb_shootdown_time;

		// ------------- Note that we assume that for each Samba componenet instance, all units run the same VMA, thus all share the same page table
		// ------------- Our assumption is based on the fact that Ariel instances (mapped one-to-one to Samba instances) can only run one application

		// Holds CR3 value of current context
		Address_t *CR3;
		int *cr3_init;

		// Holds the PGD physical pointers, the key is the 9 bits 39-47, i.e., VA/(4096*512*512*512)
		std::map<Address_t, Address_t> * PGD;

		// Holds the PUD physical pointers, the key is the 9 bits 30-38, i.e., VA/(4096*512*512)
		std::map<Address_t, Address_t> * PUD;

		// Holds the PMD physical pointers, the key is the 9 bits 21-29, i.e., VA/(4096*512)
		std::map<Address_t, Address_t> * PMD;

		// Holds the PTE physical pointers, the key is the 9 bits 12-20, i.e., VA/(4096)
		std::map<Address_t, Address_t> * PTE; // This should give you the exact physical address of the page

		// The structures below are used to quickly check if the page is mapped or not
		std::map<Address_t,int> * MAPPED_PAGE_SIZE4KB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE2MB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE1GB;

		std::map<Address_t,int> *PENDING_PAGE_FAULTS;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PGD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PUD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PMD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PTE;
		std::map<Address_t,int> *PENDING_SHOOTDOWN_EVENTS;

		//std::vector<std::pair<Address_t, int> > * PTR;
		//std::map<Address_t,int> * PTR_map;
		//int PTR_REF_POINTER;

		//PageReferenceTable *PAGE_REFERENCE_TABLE; // This is used to help page placement by sending page reference updates to Opal


		// This link is used to send internal events within the page table walker
		SST::Link * s_EventChan;



		bool stall; // This indicates the page table walker is stalling due to a fault

		Address_t stall_addr; // stores the address for which ptw was stalled

		int stall_at_levels;
		int stall_at_PGD;
		int stall_at_PUD;
		int stall_at_PMD;
		int stall_at_PTE;

		int  * sets; //stores the number of sets

		int* size; // Number of entries

		int hits; // number of hits

		int misses; // number of misses

		int os_page_size; // This is a hack for the size of the frames returned by the OS, by default

		int latency; // indicates the latency in cycles

		int emulate_faults; // if set, the page faults will be communicated to Opal

		int *page_placement;

		uint64_t *timeStamp;

		int upper_link_latency; // This indicates the upper link latency

		int max_outstanding; // indicates the number of maximum outstanding misses

		int max_width; // indicates the number of maximum accesses on the same cycle

		int parallel_mode; // very specific case for L1 PageTableWalker in case of overlapping with accessing the cache

		std::vector<SST::Event *> * service_back; // This is used to pass ready requests back to the previous level

		std::map<SST::Event *, long long int> * service_back_size; // This is used to pass the size of the  requests back to the previous level

		std::map<SST::Event *, SST::Cycle_t > ready_by; // this one is used to keep track of requests that are delayed inside this structure, compensating for latency

		std::map<SST::Event *, long long int> ready_by_size; // this one is used to keep track of requests' sizes inside this structure

		std::vector<SST::Event *> pushed_back; // This is what we got returned from other structures

		std::map<SST::Event *, long long int> pushed_back_size; // This is the sizes of the translations we got returned from other structures

		std::vector<SST::Event *> pending_misses; // This the number of pending misses, only erased when pushed back from next level

		std::vector<SST::Event *> not_serviced; // This holds those accesses not serviced yet


		int self_connected; // his parameter indidicates if the PTW is self-connected or actually connected to the memory hierarchy

		int page_walk_latency; // this is really nothing than the page walk latency in case of having no walkers

		int *STU_enabled;

		int hold_for_mem;

		int active_PTW;

		// This is and ID used to merely identify page walking requests
		long long int mmu_id=0;

		SST::Cycle_t currTime;

		SST::Component * Owner;

		uint64_t line_size; // For setting base address of MemEvents

		Statistic<uint64_t>* tlb_shootdown;
		Statistic<uint64_t>* tlb_shootdown_delay;
		Statistic<uint64_t>* cr3_addresses_allocated;
		Statistic<uint64_t>* pgd_addresses_allocated_local;
		Statistic<uint64_t>* pud_addresses_allocated_local;
		Statistic<uint64_t>* pmd_addresses_allocated_local;
		Statistic<uint64_t>* pte_addresses_allocated_local;
		Statistic<uint64_t>* pgd_addresses_allocated_global;
		Statistic<uint64_t>* pud_addresses_allocated_global;
		Statistic<uint64_t>* pmd_addresses_allocated_global;
		Statistic<uint64_t>* pte_addresses_allocated_global;

		Statistic<uint64_t>* pgd_accesses;
		Statistic<uint64_t>* pud_accesses;
		Statistic<uint64_t>* pmd_accesses;
		Statistic<uint64_t>* pte_accesses;
		Statistic<uint64_t>* pgd_unique_access;
		Statistic<uint64_t>* pud_unique_access;
		Statistic<uint64_t>* pmd_unique_access;
		Statistic<uint64_t>* pte_unique_access;

		std::map<uint64_t,int> pgd_unique_accesses;
		std::map<uint64_t,int> pud_unique_accesses;
		std::map<uint64_t,int> pmd_unique_accesses;
		std::map<uint64_t,int> pte_unique_accesses;

		Statistic<uint64_t>* pgd_addresses_migrated;
		Statistic<uint64_t>* pud_addresses_migrated;
		Statistic<uint64_t>* pmd_addresses_migrated;
		Statistic<uint64_t>* pte_addresses_migrated;
		Statistic<uint64_t>* num_of_pages_migrated;

		public: 

		PageTableWalker() {} // For serialization
		PageTableWalker(int page_size, int assoc, PageTableWalker * next_level, int size);
		PageTableWalker(int tlb_id, PageTableWalker * Next_level,int level, SST::Component * owner, SST::Params& params);

		void setPageTablePointers( Address_t * cr3, std::map<Address_t, Address_t> * pgd,  std::map<Address_t, Address_t> * pud,  std::map<Address_t, Address_t> * pmd, std::map<Address_t, Address_t> * pte, std::map<Address_t,int> * gb,  std::map<Address_t,int> * mb,  std::map<Address_t,int> * kb, std::map<Address_t,int> * pr,int *cr3I, std::map<Address_t,int> *pf_pgd,  std::map<Address_t,int> *pf_pud,  std::map<Address_t,int> *pf_pmd, std::map<Address_t,int> * pf_pte)//, std::map<Address_t,int> * sr,
				//std::vector<std::pair<Address_t, int> > * ptr, std::map<Address_t, int> * ptr_map)
		{
			CR3 = cr3;
			PGD = pgd;
			PUD = pud;
			PMD = pmd;
			PTE = pte;
			MAPPED_PAGE_SIZE4KB = kb;
			MAPPED_PAGE_SIZE2MB = mb;
			MAPPED_PAGE_SIZE1GB = gb;
			PENDING_PAGE_FAULTS = pr;
                        PENDING_PAGE_FAULTS_PGD = pf_pgd;
                        PENDING_PAGE_FAULTS_PUD = pf_pud;
                        PENDING_PAGE_FAULTS_PMD = pf_pmd;
                        PENDING_PAGE_FAULTS_PTE = pf_pte;
			//PENDING_SHOOTDOWN_EVENTS = sr;
			//PTR = ptr;
			//PTR_map = ptr_map;

			cr3_init = cr3I;
		}

		uint64_t *local_memory_size;

		// Does the translation and updating the statistics of miss/hit
		Address_t translate(Address_t vadd);


		void setEventChannel(SST::Link * x) { s_EventChan = x; }

		void finish(){}

		void set_ToMem(SST::Link * l) { to_mem = l;} 

		void set_ToOpal(SST::Link * l) { to_opal = l;} 

		void setLineSize(uint64_t size) { line_size = size; }

		// invalidate PTWC
		void invalidate(Address_t vadd, int id);

		// shootdown ack
		void sendShootdownAck(int delay, int page_swapping_delay);

		// Find if it exists
		bool check_hit(Address_t vadd, int struct_id);

		// To insert the translaiton
		int find_victim_way(Address_t vadd, int struct_id);

		void setServiceBack( std::vector<SST::Event *> * x) { service_back = x;}

		void setHold(int * tmp) { hold = tmp; }

		void setShootDownEvents(int * sd, int *iva, std::vector<std::pair<Address_t, int> > * x) { shootdown = sd; hasInvalidAddrs = iva; invalid_addrs = x;}

		void setPagePlacement(int *page_plac) { page_placement = page_plac; }

		void setTimeStamp(uint64_t *time) { timeStamp = time; }

		void setLocalMemorySize(uint64_t *mem_size) { local_memory_size = mem_size;
		std::cerr << " PTW local memory size: " << *local_memory_size << std::endl;
		}

		void initaitePageMigration(Address_t vaddress, Address_t paddress);

		void updatePTR(Address_t paddress);

		void registerPageReference(Address_t vaddress, Address_t paddress);

		//void setPageReferenceTable(PageReferenceTable * prt) { PAGE_REFERENCE_TABLE = prt;}

		void recvResp(SST::Event* event);

		void recvOpal(SST::Event* event);

		void setServiceBackSize( std::map<SST::Event *, long long int> * x) { service_back_size = x;}

		std::vector<SST::Event *> * getPushedBack(){return & pushed_back;}

		std::map<SST::Event *, long long int> * getPushedBackSize(){return & pushed_back_size;}

		std::map<long long int, int> WSR_COUNT;
		std::map<long long int, bool> WSR_READY;

		std::map<long long int, Address_t> WID_Add;

		std::map<long long int, SST::Event *> WID_EV;
		std::map<id_type, long long int> MEM_REQ;

		void update_lru(Address_t vaddr, int struct_id);


		Statistic<uint64_t>* statPageTableWalkerHits;

		Statistic<uint64_t>* statPageTableWalkerMisses;

		void handleEvent(SST::Event* event);

		int getHits(){return hits;}
		int getMisses(){return misses;}

		void insert_way(Address_t vaddr, int way, int struct_id);

		// This one is to push a request to this structure
		void push_request(SST::Event * x) {not_serviced.push_back(x);}

		bool tick(SST::Cycle_t x);

		void setSTU(int *stu_en) { STU_enabled = stu_en; }

	};

	class PageReferenceTable {
	public:
		int size;
		int max_size;
	    std::vector<std::pair<Address_t, int> > page_reference;
	    int refpointer;
	    std::pair<Address_t, int> previously_accessed_page; // previously accessed page and its index

	    PageReferenceTable(int size_)
	    {
	    	max_size = size_;
	    	reset();
	    }

	    void updatePageReference(Address_t page, int core)
	    {
	    	int ret = find(page);

	    	if(ret < 0)
	    	{
	    		// page is not found.
	    		if(size < max_size)
	    		{
	    			// create a new entry if the table is not full
	    			page_reference.push_back(std::make_pair(page,1));	// page address along with used bit
	    			size++;
	    		}
	    		else
	    		{
	    			// reference table is full find the victim entry
					while(page_reference[refpointer].second)	// page recently used or not
					{
						page_reference[refpointer].second = 0;	// reset it as unused
						refpointer = (refpointer+1)%max_size;
					}
					page_reference[refpointer].first = page;	// replace old entry with new entry
					page_reference[refpointer].second = 1;		// set used bit
					previously_accessed_page = std::make_pair(page,refpointer);
					refpointer = (refpointer+1)%max_size;
	    		}
	    	}
	    	else
	    	{
	    		// page found
	    		page_reference[ret].second = 1;
	    		previously_accessed_page = std::make_pair(page,ret);
	    	}

	    }

	    int find(Address_t page)
	    {
	    	if(previously_accessed_page.first == page)
	    		return previously_accessed_page.second;

	    	for(int i=0; i<size; i++)
	    		if(page_reference[i].first == page)
	    			return i;

	    	return -1;
	    }

	    void reset()
	    {
			size = 0;
			refpointer = 0;
			page_reference.clear();
			previously_accessed_page = std::make_pair(-1,-1);
	    }

	};


}}

#endif


