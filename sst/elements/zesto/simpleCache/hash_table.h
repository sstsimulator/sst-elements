#ifndef SIMPLE_CACHE_HASH_TABLE_H_
#define SIMPLE_CACHE_HASH_TABLE_H_

#include "cache_types.h"
#include "cache_messages.h"

#include <list>
#include <map>

class hash_entry;
class hash_set;
class hash_table;

class mshr_entry {
   public:
      mshr_entry (cache_req *request);
      ~mshr_entry (void);

      cache_req *creq;
#ifdef SIMPLE_CACHE_UTEST
      static int Mshr_entry_count;
#endif
};

class hash_entry {
   public:
      hash_entry (hash_table *table, hash_set *set, paddr_t tag, bool valid);
      ~hash_entry (void);

      hash_table *my_table;
      hash_set *my_set;
      paddr_t tag;
      bool valid;
      bool dirty;
#ifdef SIMPLE_CACHE_UTEST
      static int Hash_entry_count;
#endif
};

class hash_set {
   public:
      hash_set (hash_table *table, int assoc);
      ~hash_set (void);

      bool process_request (cache_req *request);
      hash_entry* get_entry (paddr_t tag);
      void replace_entry (hash_entry *incoming, hash_entry *outgoing);
      hash_entry* get_replacement_entry (void);

#ifndef SIMPLE_CACHE_UTEST
   private:
#endif
      void update_lru (hash_entry *entry);

      hash_table *my_table;
      int assoc;
      std::list<hash_entry *> set_entries;
};

class hash_table {
   public:
      hash_table (/*const char *name, cache_type_t type,*/ int size,
                  int assoc, int block_size, int hit_time,
                  int lookup_time, replacement_policy_t rp);
      hash_table (simple_cache_settings my_settings);
      ~hash_table (void);

      int get_hit_time() const { return hit_time; }
      int get_lookup_time() const { return lookup_time; }
      int get_index_bits() const { return num_index_bits; }
      int get_offset_bits() const { return num_offset_bits; }
      paddr_t get_tag_mask() const { return tag_mask; }
      paddr_t get_tag (paddr_t addr);
      paddr_t get_index (paddr_t addr);
      paddr_t get_line_addr (paddr_t addr);

      cache_msg_t process_request (cache_req *request);
      void process_response (cache_req *request, std::list<cache_req*>&);

#ifndef SIMPLE_CACHE_UTEST
   private:
#endif
      hash_set* get_set (paddr_t addr);
      hash_entry* get_entry (paddr_t addr);
      bool insert_mshr_entry (cache_req *request);
      void free_mshr_entry (cache_req *request, std::list<cache_req*>&);

      const char *name;
      cache_type_t type;
      int size;
      int assoc;
      int sets;
      int block_size;
      int hit_time;
      int lookup_time;
      replacement_policy_t replacement_policy;

      int num_index_bits;
      int num_offset_bits;
      paddr_t tag_mask;
      paddr_t index_mask;
      paddr_t offset_mask;

      std::map<paddr_t, hash_set *> my_sets;
      std::list<mshr_entry *> mshr_buffer;

      // void tick (void);
};

#endif // SIMPLE_CACHE_HASH_TABLE_H_
