// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <sst/core/componentExtension.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/link.h>
#include <sst/core/event.h>
#include<map>
#include<vector>
#include <sst/core/sst_types.h>

#include "utils.h"
#include "PageFaultHandler.h"

// This file defines the page table walker and

typedef std::pair<uint64_t, int> id_type;
typedef uint64_t Address_t;
enum PageMigrationType { NONE, FTP};
// FTP: First touch policy

using namespace SST::Interfaces;
using namespace SST;

namespace SST { namespace SambaComponent{

	class PageTableWalker : public ComponentExtension
	{

		Output* output;

		int coreId;
		int fault_level; // indicates the step where the page fault handler is at


		int sizes; // number of levels of page table (TODO, JVOROBY: RENAME THIS)

        //=== Params
		int os_page_size; // This is a hack for the size of the frames returned by the OS, by default
		int latency; // indicates the latency in cycles
		int emulate_faults; // if set, the page faults will be communicated to page fault handler
		int upper_link_latency; // This indicates the upper link latency
		int max_outstanding; // indicates the number of maximum outstanding misses
		int max_width; // indicates the number of maximum accesses on the same cycle
		int parallel_mode; // very specific case for L1 PageTableWalker in case of overlapping with accessing the cache
		int self_connected; // his parameter indidicates if the PTW is self-connected or actually connected to the memory hierarchy
		int page_walk_latency; // this is really nothing than the page walk latency in case of having no walkers

		uint32_t ptw_confined;


        //=== Per-page-table-level Params
        //- Each var is an array, one entry for each level of page table
        //- var[0] is the value for the leaf page table entry,
        //- in a 4-level page table, var[3] would be the root of the page table? (JVOROBY: TODO, figure out details)
        //
		uint64_t * page_size; // By default, lets assume 4KB pages
        

        //=== Page table walk cache parameters, for each level of PTWC:
		int* size;  // Number of entries in i-th level PTWC (param)
		int* assoc; // associativity of i-th PTWC (param)
		int* sets;  // number of sets in i-th PTWC (calculated)

        // === PTWC data: arranged like so: [PTWC_level][set][ent_in_set]
		Address_t *** tags; 
		bool *** valid; 
		int *** lru; // lru positions


        // == Stats
		int hits; // number of hits
		int misses; // number of misses


		int *page_placement; // JVOROBY: is set by TLBhierarchy. Seems to be unused?
		uint64_t *timeStamp; //JVOROBY: UNUSED??




		SST::Link * to_mem; // This links the Page table walker to the memory hierarchy


        //== Signals: pointers are set using setHold(), etc, by the class that created this (TLBHierarchy)
        //      we set (*hold=0 or *hold=1) to indicate holding
		int * hold; // tells TLB hierarchy to stall, to emulate overhead of page fault handler
		int * shootdown; // indicates to TLB hierarchy that TLB shootdown from other cores is in progress

//		int shootdownId;

		int num_pages_migrated; // JVOROBY: used for shootdowns?

		//std::vector<Address_t, int> buffer;
		int *hasInvalidAddrs;

		std::vector<std::pair<Address_t, int> > * invalid_addrs; // This is used to store address invalidation requests.

//		uint64_t tlb_shootdown_time;


		// ------------- Note that we assume that for each Samba component instance, all units run the same VMA, thus all share the same page table
		// ------------- Our assumption is based on the fact that Ariel instances (mapped one-to-one to Samba instances) can only run one application

		// Holds CR3 value of current context
		Address_t *CR3;
		int *cr3_init;

		// Holds the PGD, PUD, PMT, PTE physical pointers
		std::map<Address_t, Address_t> * PGD; // key is 9 bits 39-47, i.e., VA/(4096*512*512*512)
		std::map<Address_t, Address_t> * PUD; // key is 9 bits 30-38, i.e., VA/(4096*512*512)
		std::map<Address_t, Address_t> * PMD; // key is 9 bits 21-29, i.e., VA/(4096*512)
		std::map<Address_t, Address_t> * PTE; // key is 9 bits 12-20, i.e., VA/(4096)         
                                              // PTE should give you the exact physical address of the page


		// The structures below are used to quickly check if the page is mapped or not
		std::map<Address_t,int> * MAPPED_PAGE_SIZE4KB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE2MB;
		std::map<Address_t,int> * MAPPED_PAGE_SIZE1GB;

		std::map<Address_t,int> *PENDING_PAGE_FAULTS;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PGD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PUD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PMD;
		std::map<Address_t,int> *PENDING_PAGE_FAULTS_PTE;
//		std::map<Address_t,int> *PENDING_SHOOTDOWN_EVENTS;



		// This link is used to send internal events within the page table walker
		SST::Link * s_EventChan;


		bool stall; // This indicates the page table walker is stalling due to a fault

		Address_t stall_addr; // stores the address for which ptw was stalled

		int stall_at_levels;
		int stall_at_PGD;
		int stall_at_PUD;
		int stall_at_PMD;
		int stall_at_PTE;


        //=======================================================================
        //
        //           BUFFERS FOR REQUESTS TRAVELLING UP AND DOWN TLBS
        //
        //=======================================================================

        //    //  note: the _size buffers holds the size of the relevant page,
        //    //  i.e 1 for 4k, 2 for 2M, 3 for 1GB (if using standard page sizes)

        //    // === Holds incoming requests, "input queue"
        //    std::vector<MemHierarchy::MemEventBase *> not_serviced;

        //    // === Holds a copy of requests that have missed in this level, and have been sent into the next level down.
        //    //  events are erased when they are fulfilled, and returned into `this->pushed_back`
        //    std::vector<MemHierarchy::MemEventBase *> pending_misses; 

        //    // === Holds requests that have gotten the data they need, but we need to wait the duration of the latency before returning
        //    std::map<MemHierarchy::MemEventBase *, SST::Cycle_t, MemEventPtrCompare> ready_by; 
        //    std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> ready_by_size; // keeps track of requests' sizes inside this structure


        //    // === Buffers for sending requests up/down TLB hierarchy:
        //    
        //    // When we miss, we send requests to next level down through `next_level->push_request()` or `PTW->push_request()`
        //    
        //    // completed requests from deeper in TLB hierarchy will be returned into `this->pushed_back`
        //    std::vector<MemHierarchy::MemEventBase *> pushed_back; // translation for requests, returned from lower-level structures
        //    std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> pushed_back_size; // page_sizes of the returned translations

        //    // when we're finished with a request, we send it back up the hierarchy by inserting into `service_back`
        //    // - pointer is wired up to `pushed_back` buffers of the next level up at TLB in constructor of TLBHierarchy
        //    std::vector<MemHierarchy::MemEventBase *> * service_back; // used to pass ready requests back to the previous level
        //    std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * service_back_size; // page_size of ready requests for next level up
        //


        // === Holds incoming requests, "input queue"
        std::vector<MemHierarchy::MemEventBase *> not_serviced;

		std::vector<MemHierarchy::MemEventBase *> * service_back; // This is used to pass ready requests back to the previous level
		std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * service_back_size; // This is used to pass the size of the  requests back to the previous level


        // === Holds requests that have gotten the data they need, but we need to wait the duration of the latency before returning
        std::map<MemHierarchy::MemEventBase *, SST::Cycle_t, MemEventPtrCompare> ready_by; 
        std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> ready_by_size; // keeps track of requests' sizes inside this structure



		std::vector<MemHierarchy::MemEventBase *> pending_misses; // This the number of pending misses, only erased when pushed back from next level



        //==== JVOROBY: these appear to be unused? There's no lower-level TLB below the PTW, so noone to push-back to us
		// std::vector<MemHierarchy::MemEventBase *> pushed_back; // This is what we got returned from other structures
		// std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> pushed_back_size; // This is the sizes of the translations we got returned from other structures



		SST::Cycle_t currTime;

		uint64_t line_size; // For setting base address of MemEvents

		PageFaultHandler* pageFaultHandler;

		public:

		PageTableWalker(ComponentId_t id, int page_size, int assoc, PageTableWalker * next_level, int size);
		PageTableWalker(ComponentId_t id, int tlb_id, PageTableWalker * Next_level,int level, SST::Params& params);

		void setPageTablePointers( Address_t * cr3, std::map<Address_t, Address_t> * pgd,  std::map<Address_t, Address_t> * pud,  std::map<Address_t, Address_t> * pmd, std::map<Address_t, Address_t> * pte,
				std::map<Address_t,int> * gb,  std::map<Address_t,int> * mb,  std::map<Address_t,int> * kb, std::map<Address_t,int> * pr, int *cr3I, std::map<Address_t,int> *pf_pgd,  std::map<Address_t,int> *pf_pud,
				std::map<Address_t,int> *pf_pmd, std::map<Address_t,int> * pf_pte)
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

			cr3_init = cr3I;
		}

		void setPageFaultHandler( PageFaultHandler *pfh) {
			pageFaultHandler = pfh;
		}

		uint64_t *local_memory_size;

		// Does the translation and updating the statistics of miss/hit
		Address_t translate(Address_t vadd);


		void setEventChannel(SST::Link * x) { s_EventChan = x; }

		void finish(){}

		void set_ToMem(SST::Link * l) { to_mem = l;}

		void setLineSize(uint64_t size) { line_size = size; }


        // ===== PTWC Methods

		void invalidate(Address_t vadd, int id);  // invalidate PTWC
		void sendShootdownAck(int delay, int page_swapping_delay);  // shootdown ack
		bool check_hit(Address_t vadd, int struct_id);  // Find if it exists
		int find_victim_way(Address_t vadd, int struct_id);  // To insert the translaiton
		void update_lru(Address_t vaddr, int struct_id);
		void insert_way(Address_t vaddr, int way, int struct_id);

        // ====== Wire-up methods
        // (for parent obj to set out pointers to their versions of the objects)

		void setServiceBack( std::vector<MemHierarchy::MemEventBase *> * x) { service_back = x;}
		void setHold(int * tmp) { hold = tmp; }
		void setShootDownEvents(int * sd, int *iva, std::vector<std::pair<Address_t, int> > * x) 
                { shootdown = sd; hasInvalidAddrs = iva; invalid_addrs = x;}

		void setPagePlacement(int *page_plac) { page_placement = page_plac; }
		void setTimeStamp(uint64_t *time) { timeStamp = time; }

		void setLocalMemorySize(uint64_t *mem_size) { 
            local_memory_size = mem_size;
			//std::cerr << " PTW local memory size: " << *local_memory_size << std::endl;
		}

        //=========

        // JVOROBY: Unused??
		void initaitePageMigration(Address_t vaddress, Address_t paddress);

		void recvResp(SST::Event* event);

		bool recvPageFaultResp(PageFaultHandler::PageFaultHandlerPacket pkt);

		void setServiceBackSize( std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * x) { service_back_size = x;}


        //==== JVOROBY: these appear to be unused? There's no lower-level TLB below the PTW, so noone to push-back to us
		//std::vector<MemHierarchy::MemEventBase *> * getPushedBack(){return & pushed_back;}
		//std::map<MemHierarchy::MemEventBase *, long long int, MemEventPtrCompare> * getPushedBackSize(){return & pushed_back_size;}


        //===== Memory-request tracking structs
        // Misses in PTWC generate mem events to actually query mem for the translations,
        // which 
        //

        //Autoincrementing ID, used to index events held in the `WSR_` and `WID_` maps
		long long int mmu_id=0;

        // For a given page-walk memory request:
		std::map<long long int, int> WSR_PT_LEVEL; // what level of the PT does it refer to (0 = PTE, 3 = PGD)
		std::map<long long int, bool> WSR_READY;

		std::map<long long int, Address_t> WID_Add;
		std::map<long long int, MemHierarchy::MemEventBase*> WID_EV;

        // Each Walk request generates a MemEvent that is sent out;
        // This maps `memevent->getID()` to the corresponding `mmu_id`  used in the WSR_ and WID_ objects
		std::map<id_type, long long int> MEM_REQ;

        //=== Etc



		Statistic<uint64_t>* statPageTableWalkerHits;

		Statistic<uint64_t>* statPageTableWalkerMisses;

		void handleEvent(SST::Event* event);

		int getHits(){return hits;}
		int getMisses(){return misses;}


		// This one is to push a request to this structure
		void push_request(MemHierarchy::MemEventBase * x) {not_serviced.push_back(x);}

		bool tick(SST::Cycle_t x);


	};
}}

#endif
