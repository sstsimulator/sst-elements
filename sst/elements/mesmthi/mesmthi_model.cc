#include <iostream>
#include <vector>
#include <set>
#include <map>
#include "mesmthi_model.h"

using std::set;
using std::map;
using std::vector;
using std::cout;

unsigned pre_evict_delay;
unsigned post_evict_delay;
unsigned post_main_delay;

bool main_mem_access;  uint64_t main_mem_addr;
bool evict_mem_access; uint64_t evict_mem_addr;

bool miss;
bool mshr_wait;

static unsigned log2_rows, net_delay, l2_delay, l1i_ways, log2_l1i_sets,
                l1d_ways, log2_l1d_sets, l2_ways, log2_l2_sets, log2_line_size;

unsigned which_tile(uint64_t addr)
{
  return (addr >> log2_line_size)&((1<<(2*log2_rows))-1);
}

uint64_t truncate(uint64_t addr)
{
  return addr & ~((1<<log2_line_size)-1);
}

unsigned net_dist(unsigned t0, unsigned t1) {
  unsigned x0 = t0&((1<<log2_rows)-1), x1 = t1&((1<<log2_rows)-1);
  unsigned y0 = t0>>log2_rows, y1 = t1>>log2_rows;
  return (x1>x0?x1-x0:x0-x1) + (y1>y0?y1-y0:y0-y1);
}

unsigned max_net_dist(unsigned src, set<unsigned> &dests) {
  set<unsigned>::iterator i = dests.begin();
  unsigned max = net_dist(src, *i); i++;
  while (i != dests.end()) {
    unsigned dist = net_dist(src, *i);
    if (dist > max) max = dist;
    i++;
  }
  return max;
}

unsigned mc_dist(unsigned src) {
  unsigned x = src&((1<<log2_rows)-1);
  return (1<<log2_rows)-x;
}

// Simple cache memory simulator with LRU replacement policy.
class cache_t {
private:
  struct line_t {
    line_t() : state('i') {}
    unsigned age;
    uint64_t addr;
    char state;
  };

  uint64_t truncate(uint64_t addr) {
    return addr & ~((1<<log2_line_size)-1);
  }

  unsigned get_idx(uint64_t addr) {
    return (addr >> log2_line_size)&((1<<log2_sets)-1);
  }

  line_t *get_line(uint64_t addr) {
    vector<line_t> *set = &array[get_idx(addr)];
    for (unsigned i = 0; i < ways; i++) {
      if ((*set)[i].addr == addr) return &((*set)[i]);
      else (*set)[i].age++;
    }
    return NULL;
  }

public:
  cache_t(unsigned ways, unsigned log2_sets, unsigned log2_line_size) :
    ways(ways), log2_sets(log2_sets), log2_line_size(log2_line_size)
  {
    for (unsigned i = 0; i < (1u<<log2_sets); i++) {
      array.push_back(vector<line_t>(ways));
    }
  }

  bool lookup(uint64_t addr) {
    line_t *l = get_line(addr);
    if (l == NULL) return false;
    return true;
  }

  char get_state(uint64_t addr) { return get_line(addr)->state; }

  bool set_state(uint64_t addr, char state) {
    line_t *l = get_line(addr);
    if (l == NULL) return false;
    l->state = state;
    return true;
  }

  void replace_victim(uint64_t addr, bool &victim_exists, uint64_t &vic_addr)
  {
    unsigned idx = get_idx(addr);

    vector<line_t> &s = array[idx];

    unsigned max_age = s[0].age;
    line_t *way_with_max_age = &s[0];

    if (way_with_max_age->state != 'i') {
      for (unsigned i = 1; i < ways; i++) {
        if (s[i].state == 'i') {
          way_with_max_age = &s[i];
          break;
        } else if (s[i].age > max_age) {
          max_age = s[i].age;
          way_with_max_age = &s[i];
        }
      }
    }
 
    victim_exists = (way_with_max_age->state != 'i');
    vic_addr =       way_with_max_age->addr;
 
    way_with_max_age->state = 'x';
    way_with_max_age->addr  = truncate(addr);
  }

  void invalidate(uint64_t addr) {
    line_t *l = get_line(addr);
    if (l != NULL) l->state = 'i';
  }

private:
  unsigned ways, log2_sets, log2_line_size;
  vector<vector<line_t> > array; 
};

map<uint64_t, set<unsigned> > directory;

vector<cache_t*> l1_icaches;
vector<cache_t*> l1_dcaches;

// The L2 cache is distributed, but represented as a single cache_t object.
cache_t *l2_cache;

MesmthiModel::MesmthiModel(unsigned log2_r,
                           unsigned net_d, unsigned l2_d,
                           unsigned l1i_w, unsigned log2_l1i_s,
                           unsigned l1d_w, unsigned log2_l1d_s,
                           unsigned l2_w,  unsigned log2_l2_s,
                           unsigned log2_line_s)
{
  log2_rows = log2_r; net_delay = net_d; l2_delay = l2_d; 
  l1i_ways = l1i_w; log2_l1i_sets = log2_l1i_s; 
  l1d_ways = l1d_w; log2_l1d_sets = log2_l1d_s; 
  l2_ways = l2_w; log2_l2_sets = log2_l2_s; 
  log2_line_size = log2_line_s;

  unsigned n_tiles = 1<<(2*log2_rows);
  l1i_hits             = new uint64_t[n_tiles];
  l1i_misses           = new uint64_t[n_tiles];
  l1d_read_hits        = new uint64_t[n_tiles];
  l1d_write_hits       = new uint64_t[n_tiles];
  l1d_read_misses      = new uint64_t[n_tiles];
  l1d_write_misses     = new uint64_t[n_tiles];
  l2_read_hits         = new uint64_t[n_tiles];
  l2_read_misses       = new uint64_t[n_tiles];
  l2_write_hits        = new uint64_t[n_tiles];
  l2_write_misses      = new uint64_t[n_tiles];
  network_transactions = new uint64_t[n_tiles];
  total_packets        = new uint64_t[n_tiles];

  for (unsigned i = 0; i < n_tiles; i++) {
    l1_icaches.push_back(new cache_t(l1i_ways, log2_l1i_sets, log2_line_size));
    l1_dcaches.push_back(new cache_t(l1d_ways, log2_l1d_sets, log2_line_size));
    l2_cache = new cache_t(l2_ways, log2_l2_sets + 2*log2_rows, 
                           log2_line_size);
  }
}

MesmthiModel::~MesmthiModel() {
  delete[] l1i_hits;             delete[] l1i_misses;
  delete[] l1d_read_hits;        delete[] l1d_read_misses;
  delete[] l1d_write_hits;       delete[] l1d_write_misses;
  delete[] l2_read_hits;         delete[] l2_read_misses;
  delete[] l2_write_hits;        delete[] l2_write_misses;
  delete[] network_transactions; delete[] total_packets;
}

void MesmthiModel::do_net(unsigned src, unsigned dest, unsigned n) {
  unsigned y0 = src >> log2_rows, 
           y1 = dest >> log2_rows,
           x0 = src & ((1<<log2_rows)-1), 
           x1 = dest & ((1<<log2_rows)-1);
  
  unsigned x = x0, y = y0;

  network_transactions[src] += n;

  while (x != x1) {
    total_packets[(y << log2_rows) + x] += n;
    x = (x < x1) ? x+1 : x-1;
  }

  while (y != y1) {
    total_packets[(y << log2_rows) + x] += n;
    y = (y < y1) ? y+1 : y-1;
  }

  total_packets[(y1 << log2_rows) + x1] += n;
}

void MesmthiModel::do_net_mc(unsigned src, unsigned n) {
  unsigned y = src >> log2_rows;
  unsigned x = src & ((1<<log2_rows)-1);

  network_transactions[src] += n;

  while (x < (1u<<log2_rows)) {
    total_packets[(y << log2_rows) + x] += n;
    x++;
  }
}

void MesmthiModel::do_access(unsigned tile, uint64_t addr, 
                             bool inst, bool write)
{
  // Initialize all of the results to not taken/zero delay (represents a hit).
  miss = mshr_wait = false;
  pre_evict_delay = post_evict_delay = post_main_delay = 0;
  main_mem_access = evict_mem_access = false;

  addr = truncate(addr);

  // Is it a hit in L1?
  miss = inst?(!l1_icaches[tile]->lookup(addr)):
              (!l1_dcaches[tile]->lookup(addr));

  if (!miss) {
    // Register L1 hit.
    if      (inst)  l1i_hits      [tile]++;
    else if (write) l1d_write_hits[tile]++;
    else            l1d_read_hits [tile]++;
  }

  if (miss) {
    // Register L1 miss.
    if (inst)       l1i_misses      [tile]++;
    else if (write) l1d_write_misses[tile]++;
    else            l1d_read_misses [tile]++;

    unsigned home_tile = which_tile(addr);

    pre_evict_delay += l2_delay;
    pre_evict_delay += net_dist(tile, home_tile) * net_delay;
    post_main_delay += net_dist(tile, home_tile) * net_delay;
    do_net(tile, home_tile, 2);

    if (directory.find(addr) == directory.end()) {
      // The line is in main memory. Get it into L2.
      bool evict;
      uint64_t evict_addr;
 
      l2_cache->replace_victim(addr, evict, evict_addr);
      if (evict) { 
        evict_mem_access = true;
        evict_mem_addr = evict_addr;
        directory.erase(evict_addr);
        do_net_mc(home_tile, 1);
      }
      directory[addr].clear();

      main_mem_access = true;
      main_mem_addr   = addr;

      post_evict_delay += mc_dist(home_tile) * net_delay;
      post_main_delay += mc_dist(home_tile) * net_delay;
      do_net_mc(home_tile, 2);

      // Register L2 miss.
      if (write) l2_write_misses[home_tile]++;
      else       l2_read_misses [home_tile]++;
    } else {
      // Register L2 hit.
      if (write) l2_write_hits[home_tile]++;
      else       l2_read_hits [home_tile]++;
    }

    // The line is now in L2.
  
    pre_evict_delay += l2_delay;

    if (write && directory[addr].size() > 0) {
      // We are writing, invalidate other copies.
      unsigned invalidate_dist = max_net_dist(tile, directory[addr]);
      post_evict_delay += invalidate_dist * net_delay;
      set<unsigned> &dirent = directory[addr];
      for (set<unsigned>::iterator i = dirent.begin(); i != dirent.end(); i++)
      {
        l1_icaches[*i]->invalidate(addr); l1_dcaches[*i]->invalidate(addr);
      }
      directory[addr].clear();
      directory[addr].insert(tile);
    }

    // Get the line into the corresponding L1.
    bool evict;
    uint64_t evict_addr;

    cache_t *cache = inst?l1_icaches[tile]:l1_dcaches[tile];
    cache_t *other = inst?l1_dcaches[tile]:l1_icaches[tile];
    other->invalidate(addr); // Only one of the caches can hold this line.
    cache->replace_victim(addr, evict, evict_addr);
    if (evict) {
      // Line is evicted back to L2. Remove it from the directory.
      directory[evict_addr].erase(tile);
    }

    // Place this tile in the directory.
    if (directory[addr].count(tile) == 0) directory[addr].insert(tile);

    // Set the line state appropriately.
    char state;
    if (write)                            state = 'm';
    else if (directory[addr].size() == 1) state = 'e';
    else                                  state = 's';
    cache->set_state(addr, state);
  } else if (write) {
    // If the line was in other L1 caches, invalidate it in each.
    // Find the max round trip time for an invalidation and add this to the
    // pre-evict delay. Treat this like an MSHR wait.
    mshr_wait = true;
    if (directory[addr].size() > 0) {
      set<unsigned> &dirent = directory[addr];
      unsigned invalidate_dist = max_net_dist(tile, dirent);
      pre_evict_delay += invalidate_dist * net_delay;
      for (set<unsigned>::iterator i = dirent.begin(); i != dirent.end(); i++)
      {
        if (*i != tile) {
          l1_icaches[*i]->invalidate(addr);
          l1_dcaches[*i]->invalidate(addr);
        }
      }
    }
    if (inst) l1_icaches[tile]->set_state(addr, 'm');
    else      l1_dcaches[tile]->set_state(addr, 'm');
  }

  if (0 && directory.find(addr) != directory.end()) {
    cout << "Tile " << tile << ": 0x" << std::hex << addr << ": directory = {";
    set<unsigned>::iterator i = directory[addr].begin();
    while (i != directory[addr].end()) { std::cout << ' ' << *i; i++; }
    cout << "} ";
    if (inst) cout << "I ";
    if (write) cout << "W";
    cout << '\n';
  }

}

void MesmthiModel::get_results(unsigned &pre_e_d, unsigned &post_e_d,
			       unsigned &post_m_d,bool &m, bool &mw, 
                               bool &e_mem_ac, uint64_t &e_mem_ad, 
                               bool &m_mem_ac, uint64_t &m_mem_ad)
{
  pre_e_d = pre_evict_delay;
  post_e_d = post_evict_delay;
  post_m_d = post_main_delay;
  m = miss; mw = mshr_wait; 
  e_mem_ac = evict_mem_access;
  e_mem_ad = evict_mem_addr;
  m_mem_ac = main_mem_access;
  m_mem_ad = main_mem_addr;
}
