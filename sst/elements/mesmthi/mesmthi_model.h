// This is the MESMTHI model, a "winner-at-fetch" cache and NOC model. It has
// been separated from MESMTHI proper in an attempt to modularize the system
// for debugging.
#ifndef __MESMTHI_MODEL_H
#define __MESMTHI_MODEL_H

#include <stdint.h>

class MesmthiModel {
 private:
  void do_net(unsigned src, unsigned dest, unsigned n);
  void do_net_mc(unsigned src, unsigned n);
 public:
  MesmthiModel(unsigned log2_rows, 
               unsigned net_delay, unsigned l2_delay,
               unsigned l1i_ways, unsigned log2_l1i_sets,
               unsigned l1d_ways, unsigned log2_l1d_sets, 
               unsigned l2_ways,  unsigned log2_l2_sets,
               unsigned log2_line_size);

  ~MesmthiModel();

  void do_access(unsigned tile, uint64_t addr, bool inst, bool write);

  void get_results(unsigned &pre_evict_delay, unsigned &post_evict_delay,
                   unsigned &post_main_delay,
                   bool &miss, bool& mshr_wait, 
                   bool &evict_mem_access, uint64_t &evict_mem_addr,
                   bool &main_mem_access, uint64_t &main_mem_addr);

  // Counter sets. These point to a vector, one counter per tile.
  uint64_t *l1i_hits, *l1i_misses, 
           *l1d_read_hits, *l1d_write_hits, 
           *l1d_read_misses, *l1d_write_misses,
           *l2_read_hits, *l2_read_misses, *l2_write_hits, *l2_write_misses,
           *network_transactions, *total_packets;

  // Single counters
  uint64_t ram_accesses;
};

#endif
