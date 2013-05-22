//#include <assert.h>
//#include <iostream>
//#include <math.h>
//#include <string.h>

#include "cache_messages.h"
#include "hash_table.h"
#include "cache_types.h"

using namespace std;

#define SET_MISS false
#define SET_HIT true

#ifdef SIMPLE_CACHE_UTEST
int mshr_entry :: Mshr_entry_count = 0;
int hash_entry :: Hash_entry_count = 0;
#endif

// mshr_entry: Constructor
mshr_entry::mshr_entry (cache_req *request)
{
   this->creq = request;
#ifdef SIMPLE_CACHE_UTEST
   Mshr_entry_count++;
#endif
}

// mshr_entry: Destructor
mshr_entry::~mshr_entry (void)
{
#ifdef SIMPLE_CACHE_UTEST
   Mshr_entry_count--;
#endif
}

//! hash_entry: Constructor
//!
//! This is a single cache line
//! associated with one set within the cache. Contains pointers back
//! to the parent table and parent set, the address tag associated with
//! this line, and valid/dirty bits to indicate status.
hash_entry::hash_entry (hash_table *table, hash_set *set, paddr_t tag, bool valid)
{
   this->my_table = table;
   this->my_set = set;
   this->tag = tag;
   this->valid = valid;
   this->dirty = false;
#ifdef SIMPLE_CACHE_UTEST
   Hash_entry_count++;
#endif
}

// hash_entry: Destructor
hash_entry::~hash_entry (void)
{
#ifdef SIMPLE_CACHE_UTEST
   Hash_entry_count--;
#endif
}

//! hash_set: Constructor
//!
//! This is a set of cache lines, the number
//! of which is determined by the cache's associativity. Contains a pointer
//! back to the parent table, the associativity factor, and a list of pointers
//! to the hash_entries it owns.
hash_set::hash_set (hash_table *table, int assoc)
{
   this->my_table = table;
   this->assoc = assoc;

   // initialize the hash_entry list - all entries initialized to tag
   // 0x0 and marked invalid
   set_entries.clear();
   for (int i = 0; i < assoc; i++)
      set_entries.push_front(new hash_entry (table, this, 0x0, false));
}

// hash_set: Destructor
hash_set::~hash_set (void)
{
   while (!set_entries.empty ())
   {
      delete set_entries.back();
      set_entries.pop_back();
   }
}

//! hash_set: process_request
//!
//! Calls get_entry to determine whether the request is a hit or miss. If get_entry
//! returns NULL, we have a miss. Otherwise, we have a hit and we update the LRU
//! status of the member entries.
bool hash_set::process_request (cache_req *request)
{
   hash_entry *entry = get_entry (my_table->get_tag (request->addr));

   if (entry)
   {
      update_lru (entry);
      return SET_HIT;
   }
   return SET_MISS;
}

//! hash_set: get_entry
//!
//! Loops through each entry in the set to see if there's a tag match. Return NULL
//! if there's no match. Otherwise, return a pointer to the matching entry.
hash_entry* hash_set::get_entry (paddr_t tag)
{
   hash_entry *entry = NULL;
   list<hash_entry *>::iterator it;

   for (it = set_entries.begin(); it != set_entries.end(); it++)
   {
      if ((tag == (*it)->tag) && (*it)->valid)
      {
         entry = *it;
         break;
      }
   }

   return entry;
}

//! hash_set: replace_entry
//!
//! Replaces a line in the set with a new one fetched from lower level. Places
//! it at the front of the list so that it's ranked as MRU.
void hash_set::replace_entry (hash_entry *incoming, hash_entry *outgoing)
{
   list<hash_entry *>::iterator it;

   assert (incoming != NULL);
   assert (outgoing != NULL);

   for (it = set_entries.begin(); it != set_entries.end(); it++)
   {
      if ((*it)->tag == outgoing->tag)
      {
         set_entries.remove(outgoing);
         set_entries.push_front(incoming);
         break;
      }
   }
}

//! hash_set: get_replacement_entry
//!
//! Returns the least recently used entry in the set. This is always the entry
//! at the back of the list container.
hash_entry* hash_set::get_replacement_entry (void)
{
   return (set_entries.back());
} 

//! hash_set: update_lru
//!
//! On a hit, move this entry to the front of the list container to indicate
//! MRU status.
void hash_set::update_lru (hash_entry *entry)
{
   set_entries.remove (entry);
   set_entries.push_front (entry);
}

// hash_table: Constructor
hash_table::hash_table (/*const char *name, cache_type_t type,*/ int size,
                        int assoc, int block_size, int hit_time,
                        int lookup_time, replacement_policy_t rp)
{
//   this->name = name;
//   this->type = type;
   this->size = size;
   this->assoc = assoc;
   this->sets = (size) / (assoc * block_size);
   this->block_size = block_size;
   this->hit_time = hit_time;
   this->lookup_time = lookup_time;
   this->replacement_policy = rp;

   num_index_bits = (int) log2 (sets);
   num_offset_bits = (int) log2 (block_size);

   tag_mask = ~0x0;
   tag_mask = tag_mask << (num_index_bits + num_offset_bits);

   index_mask = ~0x0;
   index_mask = index_mask << num_offset_bits;
   index_mask = index_mask & ~tag_mask;

   offset_mask = ~0x0;
   offset_mask = offset_mask << num_offset_bits;

   for (int i = 0; i < sets; i++)
      my_sets[i] = new hash_set (this, assoc);

   mshr_buffer.clear();
}

// hash_table: Constructor
hash_table::hash_table (simple_cache_settings my_settings)
{
//   this->name = my_settings.name;
//   this->type = my_settings.type;
   this->size = my_settings.size;
   this->assoc = my_settings.assoc;
   this->sets = (my_settings.size) / (my_settings.assoc * my_settings.block_size);
   this->block_size = my_settings.block_size;
   this->hit_time = my_settings.hit_time;
   this->lookup_time = my_settings.lookup_time;
   this->replacement_policy = my_settings.replacement_policy;

   num_index_bits = (int) log2 (sets);
   num_offset_bits = (int) log2 (block_size);

   tag_mask = ~0x0;
   tag_mask = tag_mask << (num_index_bits + num_offset_bits);

   index_mask = ~0x0;
   index_mask = index_mask << num_offset_bits;
   index_mask = index_mask & ~tag_mask;

   offset_mask = ~0x0;
   offset_mask = offset_mask << num_offset_bits;

   for (int i = 0; i < sets; i++)
      my_sets[i] = new hash_set (this, assoc);

   mshr_buffer.clear();
}

// hash_table: Destructor
hash_table::~hash_table (void)
{
   for (int i = 0; i < sets; i++)
      delete my_sets[i];
   my_sets.clear();

   while (!mshr_buffer.empty())
   {
      delete mshr_buffer.back();
      mshr_buffer.pop_back();
   }
}

//! hash_table: get_tag
//!
//! Mask out the tag bits from the address and return the tag.
paddr_t hash_table::get_tag (paddr_t addr)
{
   return (addr & tag_mask);
}

//! hash_table: get_index
//!
//! Mask out the index bits from the address and return the index.
paddr_t hash_table::get_index (paddr_t addr)
{
   return ((addr & index_mask) >> num_offset_bits);
}

//! hash_table: get_line_addr
//!
//! Mask out the offset bits from the address and return the tag plus index.
paddr_t hash_table::get_line_addr (paddr_t addr)
{
   return (addr & offset_mask);
}

//! hash_table: get_set
//!
//! Return a pointer to the set addressed.
hash_set* hash_table::get_set (paddr_t addr)
{
   return my_sets[get_index (addr)];
}

//! hash_table: get_entry
//!
//! Return a pointer to the cache line addressed.
hash_entry* hash_table::get_entry (paddr_t addr)
{
   return my_sets[get_index (addr)]->get_entry (get_tag (addr));
}

//! hash_table: insert_mshr_entry
//!
//! Upon a miss, push an entry into the MSHR buffer. Note that even if there's
//! already an entry for the same line, an entry is created so that later on
//! a response can be sent. However, in this case, we should not send a request
//! to the memory controller.
//! To avoid confusion regarding memory deallocation, when MSHR creates an
//! entry, it allocates new memory.
//! @return  Returns false if an request for the same cache line is already
//! in the MSHR; true otherwise.
bool hash_table::insert_mshr_entry (cache_req *request)
{
   if (mshr_buffer.size() == 0) {
      cache_req* req = new cache_req(*request); //make a copy
      mshr_buffer.push_front (new mshr_entry (req));
      return true;
   }
   else
   {
      bool found = false;
      list<mshr_entry *>::iterator it;
      for (it = mshr_buffer.begin(); it != mshr_buffer.end(); it++)
      {
         if (get_line_addr ((*it)->creq->addr) == get_line_addr (request->addr)) {
            found = true;
	    break;
         }
      }
      cache_req* req = new cache_req(*request); //make a copy
      mshr_buffer.push_front (new mshr_entry (req));
      return !found;
   }
}


//! hash_table: free_mshr_entry
//!
//! Once a miss has been serviced by the lower level of memory, free the
//! corresponding MSHR entry, including the cache_req.
//!
//! @arg \c request  A cache request.
//! @arg \c reqlist  A list to hold all entries in MSHR for the same line as request.
//
void hash_table::free_mshr_entry (cache_req *request, std::list<cache_req*>& reqlist)
{
   list<mshr_entry *>::iterator it = mshr_buffer.begin();

   while ( it != mshr_buffer.end() )
   {
      if (get_line_addr ((*it)->creq->addr) == get_line_addr (request->addr))
      {
	 reqlist.push_back((*it)->creq); //all requests for the same line

         delete (*it);
         it = mshr_buffer.erase (it); //erase() returns the iterator pointing to the next element
      }
      else
          ++it;
   }
}

//! hash_table: process_request
//!
//! Called when the cache receives a load request from the processor or higher-level
//! cache. Returns the appropriate message to send back to the processor and/or
//! next level of memory.
cache_msg_t hash_table::process_request (cache_req *request)
{
   hash_set *set;
//   bool created_mshr = false;

   assert (request != NULL);

   set = get_set (request->addr);
   if (set->process_request (request))
      return CACHE_HIT;
   else
   {
      if (request->op_type == OpMemLd) {
         if(insert_mshr_entry (request) )
            return CACHE_MISS;
         else
            return CACHE_MISS_DUPLICATE;
      }
      else { //STORE request
         return CACHE_MISS;
      }
   }
}

//! hash_table: process_response
//!
//! Called when a LOAD response comes back from memory. Releases
//! the necessary MSHR buffer entry and does a line replacement if necessary.
//! In addition, all requests R in the MSHR for the same line as the parameter
//! request are returned in a list so responses to the processor can be sent.
//! When R was first received and it was a miss, because there was already a request
//! for the same cache line, no request to the memory was sent, but a MSHR entry
//! was created, so response can be sent to the processor.
//!
//! @arg \c request  A cache request.
//! @arg \c reqlist  A list to hold all entries in MSHR for the same line as request.
void hash_table::process_response (cache_req *request, std::list<cache_req*>& reqlist)
{
   assert (request != NULL);
   assert(request->msg == LD_RESPONSE);

   hash_set* set = get_set (request->addr);
   assert (mshr_buffer.size() != 0);
   free_mshr_entry (request, reqlist);
   hash_entry* outgoing = set->get_replacement_entry();
   assert (outgoing != NULL);
   hash_entry* incoming = new hash_entry (this, set, get_tag (request->addr), true);
   set->replace_entry (incoming, outgoing);
   delete outgoing;
}

