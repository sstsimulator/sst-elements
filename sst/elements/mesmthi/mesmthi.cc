// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include <qsim.h>
#include <sst/elements/include/memoryDev.h>
#include <sst/elements/include/memMap.h>

#include "mesmthi.h"
#include "aux/des.h"
#include "aux/cont_proc.h"

using namespace std;
using namespace Slide;
using namespace Qsim;
using namespace SST;

// TODO: Arun's going to make this variable disappear.
int badCheat = 1;

// A simple hack to make debugging a little easier.
uint64_t insts__ = 0;
bool debug = false;
#define _STRINGIFY_(x) #x
#define _STRINGIFY(x) _STRINGIFY_(x)
#define DBG if (debug) \
    cout << "Debug ("/*"thd=" << _running << */\
      /*", l=" _STRINGIFY(__LINE__) ", "*/ << "t=" << dec << _now << "): "
#define DBGC if (debug) cout

#define DBGT DBG << dec << '(' << tile->here_x << ", " << tile->here_y << "): "
#define DBGTR DBGT << "rid=" << dec << rid << "; "

// A way to keep track of all of our counters
map <string, map<string, uint64_t *> > counter_registry;

void print_counters() {
  typedef map <string, map<string, uint64_t *> >::iterator set_it;
  typedef map <string, uint64_t *>::iterator counter_it;

  for (set_it i = counter_registry.begin(); i != counter_registry.end(); i++) {
    for (counter_it j = i->second.begin(); j != i->second.end(); j++) {
      cout << i->first << ';' << j->first << " = " << dec << *j->second <<'\n';
    }
  }
}

void clear_counters() {
  typedef map <string, map<string, uint64_t *> >::iterator set_it;
  typedef map <string, uint64_t *>::iterator counter_it;

  for (set_it i = counter_registry.begin(); i != counter_registry.end(); i++) {
    for (counter_it j = i->second.begin(); j != i->second.end(); j++) {
      *j->second = 0;
    }
  }
}

inline void register_counter(string set, string name, uint64_t &counter) {
  counter_registry[set][name] = &counter;
}

// Debugging tool: keep track of requests so we can always query the oldest
// pending request that locked up.
uint64_t last_rid = 0;

struct req_t {
  uint64_t addr;
  unsigned x, y;
  bool write;
  bool inst;
  uint64_t rid;

  req_t(uint64_t a, unsigned x, unsigned y, bool w, 
        bool i, uint64_t r) : 
    addr(a), x(x), y(y), write(w), inst(i), rid(r) {}
};

multimap<uint64_t, req_t> outstanding_reqs;
typedef multimap<uint64_t, req_t>::iterator or_it;

or_it start_req(uint64_t addr, unsigned x, unsigned y, 
                bool write, bool inst, uint64_t i) 
{
  return outstanding_reqs.insert(pair<uint64_t, req_t>
				 (_now, req_t(addr, x, y, write, inst, i)));
}

void end_req(or_it i) {
  outstanding_reqs.erase(i);
}

void show_oldest_reqs() {
  cout << "10 oldest outstanding requests:\n";
  unsigned i; or_it it;

  for (i = 0, it = outstanding_reqs.begin(); 
       i < 10 && it != outstanding_reqs.end(); 
       i++, it++)
    cout << "  " << dec << it->first << ": rid = " << it->second.rid << "; (" 
         << it->second.x << ", " << it->second.y << "); 0x" << hex 
         << it->second.addr << "; " << (it->second.inst?'I':'D') 
         << (it->second.write?"-WRITE":"-READ") << '\n';
}


// Timings and other parameters; converting the names of the old const values
// to the ones declared in mesmthi.h
#define ROW_SIZE_L2  log2_row_size
#define ROW_SIZE     (1<<ROW_SIZE_L2)
#define THREADS      threads
#define FETCH_BYTES  fetch_bytes
#define BPB_LOG2     log2_bytes_per_block
#define I_SETS_LOG2  log2_sets
#define D_SETS_LOG2  log2_sets
#define L2_SETS_LOG2 log2_sets
#define I_WAYS       i_ways
#define D_WAYS       d_ways
#define L2_WAYS      l2_ways
#define L2_L1_SWAP_T l2_l1_swap_t
#define D_I_SWAP_T   d_i_swap_t
#define CYC_PER_TICK cyc_per_tick
#define PADDR_BITS   paddr_bits
#define NET_LATENCY  net_latency
#define NET_CPP      net_cpp
#define NET_DELAY    (NET_LATENCY - NET_CPP)

void show_dir_ent(uint64_t addr, 
                  set<pair<unsigned, unsigned> > &present)
{
  DBG << "Dir. ent. for 0x" << hex << addr << ": ";

  set<pair<unsigned, unsigned> >::iterator i;
  for (i = present.begin(); i != present.end(); i++) {
    DBGC << '(' << dec << i->first << ", " << i->second << ')';
  }
  DBGC << '\n';
}

struct memop_t { 
  uint64_t addr; int type;
  memop_t(uint64_t a, int t) : addr(a), type(t) {}
};

struct MemoryController;
MemoryController *mc;

struct MemoryController {
  MemoryDev <uint64_t, cont_cond_t*> *mem_dev;

  MemoryController() {
    register_counter("Memory Controller", "Reads",  reads);
    register_counter("Memory Controller", "Writes", writes);


    // Hardcoding these things for now. Don't do this in the rewrite.
    Params memParams;
    memParams["mem.initialCredit"]   = "8";
    memParams["mem.readRespCredit"]  = "1";
    memParams["mem.writeRespCredit"] = "1";
    mem_dev = new MemoryDev<uint64_t, cont_cond_t*>(*_sstComponent, 
                                                    memParams, 
                                                    "bus");

    // Spawn the cookie polling process.
    spawn_cont_proc(new cookie_poll_cp());
  }

  ~MemoryController() {
    delete mem_dev;
  }

  // Poll for memory responses every cycle. There has to be a better way to do
  // this.
  class cookie_poll_cp : public cont_proc_t {public:
    delay_cpt delay;
    cont_cond_t *c;

    cookie_poll_cp() : delay(1) {}

    cont_proc_t *main() {
      CONT_BEGIN();
      while (1) {
        while (mc->mem_dev->popCookie(c)) { 
          DBG << "Popped cookie c=" << c << '\n';
          *c = true;
        }
        CONT_CALL(&delay);
      }
      CONT_END();
    }
  };

  class mem_req_cp : public cont_proc_t {public:
    // Arguments
    uint64_t addr;
    unsigned y;
    bool write;
    
    // Local variables.
    cont_cond_t *c;

    mem_req_cp() {}

    cont_proc_t *main() {
      CONT_BEGIN();
      DBG << (write?"W_REQ(0x":"R_REQ(0x") << hex << addr << ")\n";
      (write?mc->writes:mc->reads)++;
	

      c = new cont_cond_t(false);
      DBG << "Created cookie c=" << c << '\n';

      if (write) mc->mem_dev->write(addr, c);
      else       mc->mem_dev->read (addr, c);

      CONT_WAIT(*c, true);
      delete c;

      DBG << "Finished " << (write?"W_REQ(0x":"R_REQ(0x") << hex << addr
          << ")\n";
      CONT_END();
    }
  };

  uint64_t reads, writes;
};

const char *packet_type_s[] = {
  "WRITE_S", "WRITE_I", "READ_I", "FAIL_RESP", "INV", "RESP", "RESP_E",
  "R_REQ", "W_REQ", "CLEAR_P", "C_REQ"
};

class Tile {
public:
  // Data delivered by callbacks from QSim.
  uint64_t inst_addr;
  uint8_t inst_len;
  

  // Constructor; pre wire-up
  Tile(unsigned base_cpuid, unsigned x, unsigned y) : 
    tile_n(NULL), tile_e(NULL), tile_s(NULL), tile_w(NULL),
    here_x(x), here_y(y),
    l1i(this, BPB_LOG2, I_WAYS, I_SETS_LOG2), 
    l1d(this, BPB_LOG2, D_WAYS, D_SETS_LOG2), 
    l2u(this, BPB_LOG2, L2_WAYS, L2_SETS_LOG2),
    instructions(0), bus_transfers(0), bus_requests(0), 
    dir_reads(0), dir_writes(0), whohas()
  {
    DBG << "Constructing Tile.\n";

    // Assign counters to the registry.
    ostringstream setname;
    setname << "T(" << here_x << ", " << here_y << ")";
    register_counter(setname.str(), "Bus Transfers", bus_transfers);
    register_counter(setname.str(), "Instructions",  instructions);
    register_counter(setname.str(), "Directory Reads", dir_reads);
    register_counter(setname.str(), "Directory Writes", dir_writes);

    register_counter(setname.str(), "L1I Reads",  l1i.reads);
    register_counter(setname.str(), "L1I Writes", l1i.writes);
    register_counter(setname.str(), "L1I Write Hits", l1i.write_hits);
    register_counter(setname.str(), "L1I Read Hits", l1i.read_hits);

    register_counter(setname.str(), "L1D Reads",  l1d.reads);
    register_counter(setname.str(), "L1D Writes", l1d.writes);
    register_counter(setname.str(), "L1D Write Hits", l1d.write_hits);
    register_counter(setname.str(), "L1D Read Hits", l1d.read_hits);

    register_counter(setname.str(), "L2 Reads",  l2u.reads);
    register_counter(setname.str(), "L2 Writes", l2u.writes);
    register_counter(setname.str(), "L2 Write Hits", l2u.write_hits);
    register_counter(setname.str(), "L2 Read Hits", l2u.read_hits);

    register_counter(setname.str(), "Network Transactions", packets_sent);
    register_counter(setname.str(), "Total Packets", packets_total);

    // Create THREADS thread processes for this tile and put them in the queue
    for (unsigned i = 0; i < THREADS; i++) {
      // Create the new thread process.
      hw_thread_cp *hwt = new hw_thread_cp(this, base_cpuid + i);
      // Put it in this tile's thread queue.
      thread_q.push(hwt);
      // Make it runnable in cont_proc.
      spawn_cont_proc(hwt);
    }
    thread_q.front()->my_turn = true;
  }

  // Functions for performing wire-up
  void connect_n(Tile &n) { tile_n = &n; }
  void connect_e(Tile &e) { tile_e = &e; }
  void connect_s(Tile &s) { tile_s = &s; }
  void connect_w(Tile &w) { tile_w = &w; }

  // Reset; post wire-up initialization
  void reset() {
  }

  // Counters
  uint64_t instructions;  // # Instructions executed.
  uint64_t bus_transfers; // # Lines transferred over the bus.
  uint64_t bus_requests;  // # requests (just address and req. type)

  uint64_t dir_reads;
  uint64_t dir_writes;

  uint64_t packets_sent;
  uint64_t packets_total;

  // Prevent copies and assignment of this simulation component.
  Tile(Tile &t) : l1i(0l, 0, 0, 0), l1d(0l, 0, 0, 0), l2u(0l, 0, 0, 0),
                  e_busy(false), n_busy(false), w_busy(false), s_busy(false)
    {}
  Tile &operator=(Tile &t) {}

  cont_cond_t e_busy, n_busy, w_busy, s_busy;

  // A. The Thread Process.

  // Forward declarations of cont_proc functions to be called.
  class generic_ac_cp;

  class hw_thread_cp : public cont_proc_t { public:
    Tile*       tile;     // Pointer to the parent class.
    cont_cond_t my_turn;  // True when first in this tile's thread queue
    unsigned    qsim_cpu; // CPU ID in qsim that corresponds to this thread

    // cont_proc_t functions to be called.
    generic_ac_cp     *i_rd;
    generic_ac_cp     *d_rd;
    generic_ac_cp     *d_wr;
    delay_cpt          delay;
    queue<memop_t>     mem_ops;

    // local vars
    or_it cur_req;
    
    hw_thread_cp(Tile *t, unsigned c) 
      : my_turn(false), qsim_cpu(c), tile(t), delay(1)
    {
      DBG << "Constructing HW Thread " << c << '\n';

      i_rd = new generic_ac_cp(&tile->l1i, &tile->l1d, false);
      d_rd = new generic_ac_cp(&tile->l1d, &tile->l1i, false);
      d_wr = new generic_ac_cp(&tile->l1d, &tile->l1i, true);

      i_rd->tile = d_rd->tile = d_wr->tile = tile;

      i_rd->is_d = false;
      d_rd->is_d = d_wr->is_d = true;
    }

    ~hw_thread_cp() {
      delete i_rd;
      delete d_rd;
      delete d_wr;
    }

    int fetched_bytes;
    bool first_memop;
    cont_proc_t *main() {
      uint64_t rid;

      CONT_BEGIN();
      DBGT << "Beginning hw_thread_cp " << dec << qsim_cpu << " main().\n";
      while (simulation_running) {
	CONT_WAIT(my_turn, true);

	// If the thread has not been started yet, don't do anything.
	if (cd->booted(qsim_cpu)) {

	  //DBG << "It's thread " << qsim_cpu << "'s turn.\n";
	  // Run this CPU 1 instruction.
	  tile->cur_thread = this;
	  cd->run(qsim_cpu, 1);
	  tile->instructions++; insts__++;

	  // Perform instruction fetch in L1 i-cache.
	  //DBG << "Doing fetch for latest inst at 0x" << hex 
          //    << tile->inst_addr << ".\n";
	  fetched_bytes = 0;
	  while (fetched_bytes < tile->inst_len) {
	    // Let fetches beyond the first FETCH_BYTES incur 1-cycle penalty.
	    if (fetched_bytes > 0) CONT_CALL(&delay);

            rid = last_rid++;
	    //DBGTR << "READ from L1 I$ @0x" << hex 
	    //      << tile->inst_addr + fetched_bytes << '\n';
	    i_rd->addr = tile->inst_addr + fetched_bytes;
            i_rd->rid = rid;
            cur_req = start_req(tile->inst_addr + fetched_bytes, 
                                tile->here_x, tile->here_y, false, true, 
                                rid);
	    CONT_CALL(i_rd);
            end_req(cur_req);

	    fetched_bytes += FETCH_BYTES;
	  }

          // Perform all outstanding L1 d-cache accesseses.
	  first_memop = true;
	  while (!mem_ops.empty()) {
	    if (first_memop == false) CONT_CALL(&delay);

            rid = last_rid++;
	    if (mem_ops.front().type) {
	      //DBGTR << "WRITE to L1 D$ @0x" << hex 
	      //      << mem_ops.front().addr << '\n';
	      d_wr->addr = mem_ops.front().addr;
              d_wr->rid = rid;
	      cur_req = start_req(d_wr->addr, 
                                  tile->here_x, tile->here_y, 
                                  true, false, rid);
	      CONT_CALL(d_wr);
              end_req(cur_req);
	    } else {
	      //DBGTR << "READ from L1 D$ @0x" << hex
	      //      << mem_ops.front().addr << '\n';
	      d_rd->addr = mem_ops.front().addr;
              d_rd->rid = rid;
              cur_req = start_req(d_rd->addr, 
                                  tile->here_x, tile->here_y, 
                                  false, false, rid);
	      CONT_CALL(d_rd);
              end_req(cur_req);
	    }
	    mem_ops.pop();

	    // Let memory operations beyond the first incur 1-cycle penalty.
	    first_memop = false;
	  }

        }
        // Wait for the clock to tick before we go to the next instruction.
	// This implies that we want even as-yet unbooted threads to still
	// require cycles.
	CONT_CALL(&delay);
	
	// Pass the torch to the next thread.
	my_turn = false;
	tile->thread_q.pop();
	tile->thread_q.push(this);
	tile->thread_q.front()->my_turn = true;
      }
      CONT_END();
    }
  private:
    
  };

  hw_thread_cp        *cur_thread;
  queue<hw_thread_cp*> thread_q;


  // The caches within a tile are fully exclusive in order to simplify
  // coherence between tiles. The inter-tile coherency protocol is a full
  // directory MESI protocol.

  // We use a real LRU replacement policy, which actually doesn't show up in
  // hardware much beyond 2-way caches. Intel and AMD use tree-based pseudo-LRU
  // policies, which are harder to implement in software but much easier to
  // implement in hardware than actual LRU policies. Examples of pseudo-LRU and
  // random implementations can be seen in the prototype cache models.

  // The exclusive policy complicates handling misses by making all misses that
  // hit in another cache swap operations. This adds another bus transfer for
  // these operations, and may cause unnecessary thrashing when a line could be
  // shared. This is only apparent when threads on the same core are executing
  // and reading/writing within the same line at the same time (as in self-
  // modifying code) which never happens in any of the considered benchmarks.

  // Also, note that for convenience we are compltely ignoring the concept of a
  // tag and using the base address of the cache block. The behavior is the
  // same with no performance hit, and the line_t object looks the same no 
  // matter which cache it is in.

  // Our mechanism for avoiding race conditions is simply keeping track of
  // which cache each line lives in with a directory. In actual hardware,
  // the caches themselves keep track of their own lines and mutual exclusion
  // is a property of the bus protocol.

  class cache_t;
  class mshr_sync_t;
  map <uint64_t, cache_t*> whohas;

  class mshr_wait_cp : public cont_proc_t { public:
    uint64_t addr; Tile *tile;

    mshr_sync_t *sync;
    cont_proc_t *main() {
      CONT_BEGIN();

      // If there is no MSHR, there's no need to wait.
      if (tile->mshrs.find(addr) == tile->mshrs.end()) {
	DBGT << "Nothing to wait on.\n";
	CONT_RET();
      }

      // If the present bit has been set, this MSHR is waiting on an eviction
      // and we have no need to use a sync structure.
      if (tile->mshrs[addr].present) {
        DBGT << "MSHR already signalled.";
	CONT_RET();
      }

      // Create a new sync structure if it does not already exist.
      if (tile->mshrs[addr].sync == NULL) {
        tile->mshrs[addr].sync = new mshr_sync_t();
      }
      
      sync = tile->mshrs[addr].sync;

      sync->waiters++;
      CONT_WAIT(sync->present, true);
      sync->waiters--;

      // If there are no more waiters for this sync object, delete it and
      // fix up the MSHR, if it pointed to this sync object.
      if (sync->waiters == 0) {
	if (tile->mshrs.find(addr) != tile->mshrs.end() && 
            tile->mshrs[addr].sync == sync) {
          tile->mshrs[addr].sync = NULL;
        }

        delete sync;
      }

      CONT_END();
    }
  };

  struct mshr_sync_t {
    mshr_sync_t() : present(false), waiters(0) {}
    cont_cond_t present;
    unsigned waiters;
  };

  struct mshr_t {
    mshr_t() : 
      deletable(false), 
      invalidated(false),
      present(false),
      sync(NULL),
      state('i') {
      DBG << "NEW MSHR_T CREATED!\n"; 
    }

    bool deletable;
    bool invalidated;
    bool present;
    mshr_sync_t *sync;
    char state;
  };
  map <uint64_t, mshr_t> mshrs;

  class cache_t {
  public:
    cache_t(Tile *t, 
	    unsigned l2_bpb, 
	    unsigned ways, 
	    unsigned l2_sets)
      : l2_sets(l2_sets), sets(1<<l2_sets), ways(ways),
	l2_bpb(l2_bpb), writes(0), reads(0), write_hits(0), read_hits(0),
	tile(t)
    {
      // Size and initialize the array.
      array.resize(sets*ways);

      for (unsigned i = 0; i < sets*ways; i++) {
	array[i].set_state('i');
      }

      DBGT << "New cache " << this << ", array base " << &array[0] << ".\n";
    }

    class line_t {
    public:
      line_t() {
        DBG << "line_t constructed [" << this << "]\n";
        addr = 0; state = 'i';
      }

      line_t(const line_t &l) {
        DBG << "line_t copy-constructed [" << this << "]\n";
        addr = l.addr; state = l.state; age = l.age;
      }

      void set_state(char s) {
        DBG << "State changed on line holding 0x" << hex << addr << '\'' 
            << state << "'<=>'" << s << "' [" << this << "].\n";
        state = s; 
      }

      char get_state() {
        //DBG << "get_state() on 0x" << hex << addr << " returns '" << state
        //    << "' [" << this << "]\n";
        return state;
      }

      line_t &operator=(const line_t &rhs) {
        DBG << "Line 0x" << hex << addr << '(' << state << ")["
            << this << "] assigned 0x" << rhs.addr << '(' << rhs.state 
            << ")[" << &rhs << "]\n";
        addr = rhs.addr;
        state = rhs.state;
        return *this; 
      }

      uint64_t    addr;
      uint64_t    age;

     private:
       char state;
     };

     line_t *get(uint64_t addr) {
       addr = mask_addr(addr);
       line_t *set_base = &array[((addr >> l2_bpb)&(sets-1))*ways];
       for (unsigned i = 0; i < ways; i++) {
	 if (set_base[i].addr == addr && set_base[i].get_state() != 'i') 
           return &set_base[i];
       }
       return NULL;
     }

     // Find a block to hold an incoming line. Return the way that is being
     // evicted and a pointer to the line.
     unsigned choose_victim(line_t **l, uint64_t addr) {
       line_t *set_base = &array[((addr >> l2_bpb)&(sets-1))*ways];

       // First check for lines in the invalid state which can be replaced.
       for (unsigned i = 0; i < ways; i++) {
	 if (set_base[i].get_state() == 'i') {
	   *l = &set_base[i];
	   return i;
	 }
       }

       // Next check for the oldest line, which must be written out.
       uint64_t max_age = 0;
       unsigned evict_way = 0;
       for (unsigned i = 0; i < ways; i++)
	 if (set_base[i].age >= max_age) evict_way = i;
       *l = &set_base[evict_way];
       return evict_way;
     }

     void reserve_mshr(uint64_t addr) {
       DBGT << "Reserving MSHR for " << hex << mask_addr(addr) << '\n';
       tile->mshrs[mask_addr(addr)] = mshr_t();
     }

     void signal_mshr(uint64_t addr) {
       addr = mask_addr(addr);

       DBGT << "signal_mshr(0x" << hex << addr << ")\n";

       mshr_sync_t *sync = tile->mshrs[addr].sync;

       if (sync) {
	 tile->mshrs[addr].sync->present = true;

	 if (sync->waiters > 0) 
	   DBGT << "  ^^" << dec << sync->waiters << " waiters.\n";
       }

       tile->mshrs[addr].present = true;

       if (tile->mshrs[addr].deletable) {
	 DBGT << "Deletable MSHR in signal_mshr(0x" << hex << addr << ")\n";
	 //tile->mshrs.erase(addr);
       } else if (tile->mshrs[addr].invalidated) {
	 // Remove from the whohas list if it's been invalidated
	 DBGT << "Removing 0x" << hex << addr << " from whohas.\n";
	 tile->whohas.erase(addr);
       }
     }

     void delete_mshr(uint64_t addr) {
       addr = mask_addr(addr);
       DBGT << "delete_mshr(0x" << hex << addr << ")\n";
       if (tile->mshrs.find(addr) != tile->mshrs.end()) {
	 tile->mshrs[addr].deletable = true;
       }
     }

     // Counters
     uint64_t writes, reads, write_hits, read_hits;

   private:
     unsigned ways;      // # ways
     unsigned sets;      // 1<<l2sets
     unsigned l2_sets;   // log2 #sets
     unsigned l2_bpb;    // log2 #bytes per block

     uint64_t mask_addr(uint64_t &u) { return u&~((1<<l2_bpb)-1); }

     vector <line_t> array;

     Tile *tile;
   };

   unsigned here_x, here_y;

   cache_t l1i, l1d, l2u;

   // F, H. Directory and Router

   struct dir_ent_t {
     // Tile addresses on which line is present;
     set<pair<unsigned, unsigned> > present;  
   };
   map <uint64_t, dir_ent_t> directory;

   Tile *tile_n, *tile_e, *tile_s, *tile_w;

   // Circumvent the inherent bias from using the modulo operator. Only use 
   // thisfor numbers much smaller than RAND_MAX.
   unsigned unbiased_rand (unsigned max) {
     int r = rand();
     double u = r/((double)RAND_MAX+1);
     return (unsigned)(u*max);
   }

   template <typename T> const T &rand_element(set<T> &s) {
     if (s.size() == 0) { *((int*)0) = 1234; }
     unsigned n = unbiased_rand(s.size());
     typename set<T>::iterator it;
     unsigned i;
     for (it = s.begin(), i = 0; it != s.end(); i++, it++) {
       if (i == n) return *it;
     }
     cerr << "rand_element() is broken. This should never happen.\n";
     exit(1);
   }

   // For explanation of directory protocol, see TILE_MEM_ALGOS.
   enum packet_type_t { 
     WRITE_S, WRITE_I, READ_I, FAIL_RESP, INV, RESP, RESP_E, R_REQ, W_REQ,
     CLEAR_P, C_REQ
   };

   struct packet_t {
     int delta_x, delta_y;
     uint64_t addr;
     unsigned x_src, y_src;
     packet_type_t type;
     uint64_t rid;
   };

   class broadcast_inv_cp;

   class sendpkt_cp : public cont_proc_t {public:
     // Parameters
     Tile *tile;
     packet_t p;

     // Local variables
     sendpkt_cp *next_hop;
     delay_cpt proc_delay; // "processing delay"; cycles per issue at router
     delay_cpt delay;      // router delay minus processing delay
     broadcast_inv_cp *broadcast_inv;
     map<uint64_t, dir_ent_t>::iterator it;
     set<pair<unsigned, unsigned> > inv_set;
     pair<unsigned, unsigned> req_dest;
     MemoryController::mem_req_cp mem_req;
     mshr_wait_cp mshr_wait;
     char s;
     uint64_t rid;

     sendpkt_cp() : proc_delay(NET_CPP), delay(NET_DELAY) {}

     cont_proc_t *main() {
       CONT_BEGIN();
       rid = p.rid;

       tile->packets_total++;

       if (p.delta_x == 0 && p.delta_y == 0) {
	 // Deliver packet (perform appropriate action)

	 if (p.type == WRITE_S) {
	   DBGTR << "WRITE_S(0x" << hex << p.addr << ") from (" 
		 << p.x_src << ", " << p.y_src << ")\n";
	   // If we're getting a WRITE_S from a now invalid source, send a
	   // FAIL_RESP.
           tile->dir_reads++;
	   if (tile->directory[p.addr].present.find(
		 pair<unsigned, unsigned>(p.x_src, p.y_src)
	       ) == tile->directory[p.addr].present.end()) {
	     next_hop = new sendpkt_cp();
	     next_hop->tile = tile;
	     next_hop->p.addr = p.addr;
	     next_hop->p.type = FAIL_RESP;
	     next_hop->p.delta_x = p.x_src - tile->here_x;
	     next_hop->p.delta_y = p.y_src - tile->here_y;
	     next_hop->p.x_src = tile->here_x;
	     next_hop->p.y_src = tile->here_y;
	     next_hop->p.rid = rid;
	     CONT_CALL(next_hop);
	     p = next_hop->p;
	     delete(next_hop);
	   } else {
	     // Make a copy of the present bitmap.
	     inv_set = tile->directory[p.addr].present;
             tile->dir_reads++;

	     // Make the present bitmap contain only the requestor.
	     tile->directory[p.addr].present.clear();
	     tile->directory[p.addr].present.insert(pair<unsigned, unsigned>
						      (p.x_src, p.y_src));
             tile->dir_writes++;

	     // Broadcast invalidate to other owners.
	     inv_set.erase(pair<unsigned, unsigned>(p.x_src, p.y_src));
	     broadcast_inv = new broadcast_inv_cp(tile, inv_set, p.addr, rid);
	     CONT_CALL(broadcast_inv);
	     delete broadcast_inv;

	     // Send RESP to requestor.
	     next_hop = new sendpkt_cp();
	     next_hop->tile = tile;
	     next_hop->p.addr = p.addr;
	     next_hop->p.type = RESP;
	     next_hop->p.delta_x = p.x_src - tile->here_x;
	     next_hop->p.delta_y = p.y_src - tile->here_y;
	     next_hop->p.x_src = tile->here_x;
	     next_hop->p.y_src = tile->here_y;
	     next_hop->p.rid = rid;
	     CONT_CALL(next_hop);
	     p = next_hop->p;
	     delete(next_hop);
	   }
	 } else if (p.type == WRITE_I) {
	   DBGTR << "WRITE_I(0x" << hex << p.addr << ")\n";
           tile->dir_reads++;
           tile->dir_writes++;
	   if (tile->directory.find(p.addr) == tile->directory.end()) {
	     tile->directory[p.addr];
	     tile->directory[p.addr].present.insert(pair<unsigned, unsigned>
						      (p.x_src, p.y_src));

	     // The line isn't anywhere. Get it from main memory.
	     next_hop = new sendpkt_cp();
	     next_hop->tile = tile;
	     next_hop->p = p; // Let MC respond directly to orig.
	     next_hop->p.type = R_REQ;
	     next_hop->p.delta_x = ROW_SIZE;
	     next_hop->p.delta_y = 0;
	     next_hop->p.rid = rid;
	     CONT_CALL(next_hop);
	     delete(next_hop);

	   } else {
	     // The line is present elsewhere. Send invalidate requests (and one
	     // invalidate-and-forward, if this were actual hardware).
	     inv_set = tile->directory[p.addr].present;

	     tile->directory[p.addr].present.clear();
	     tile->directory[p.addr].present.insert(pair<unsigned, unsigned>
						      (p.x_src, p.y_src));

	     broadcast_inv = new broadcast_inv_cp(tile, inv_set, p.addr, rid);
	     CONT_CALL(broadcast_inv);
	     delete broadcast_inv;
	   }

	   // Send RESP back to requestor.
	   next_hop = new sendpkt_cp();
	   next_hop->tile = tile;
	   next_hop->p.addr = p.addr;
	   next_hop->p.type = RESP;
	   next_hop->p.delta_x = p.x_src - tile->here_x;
	   next_hop->p.delta_y = p.y_src - tile->here_y;
	   next_hop->p.x_src = tile->here_x;
	   next_hop->p.y_src = tile->here_y;
	   next_hop->p.rid = rid;
	   CONT_CALL(next_hop);
	   p = next_hop->p;
	   delete next_hop;

	 } else if (p.type == READ_I) {
	   DBGTR << "READ_I(0x" << hex << p.addr << ")\n";
           tile->dir_reads++;
           tile->dir_writes++;
	   // If we can, send a C_REQ to a random tile that has this line.
	   if (tile->directory.find(p.addr) != tile->directory.end()) {
	     // Send a C_REQ to a random other tile that has this line.
	     req_dest = tile->rand_element(tile->directory[p.addr].present);
	     next_hop = new sendpkt_cp();
	     next_hop->tile = tile;
	     next_hop->p.addr = p.addr;
	     next_hop->p.type = C_REQ;
	     next_hop->p.x_src = tile->here_x;
	     next_hop->p.y_src = tile->here_y;
	     next_hop->p.delta_x = req_dest.first - tile->here_x;
	     next_hop->p.delta_y = req_dest.second - tile->here_y;
	     next_hop->p.rid = rid;

	     // Add the requestor to the present bitmap.
	     tile->directory[p.addr].present.insert(pair<unsigned, unsigned>
						      (p.x_src, p.y_src));
	     DBGTR << "READ_I(0x" << hex << p.addr << "): Doing C_REQ.\n";
	     show_dir_ent(p.addr, tile->directory[p.addr].present);
	     CONT_CALL(next_hop);
	     DBGTR << "READ_I(0x" << hex << p.addr << "): Finished C_REQ.\n";
	     delete next_hop;
	   } else {
	     // Add the requestor to the present bitmap.
	     tile->directory[p.addr].present.insert(pair<unsigned, unsigned> 
						      (p.x_src, p.y_src));

	     // Send a R_REQ to the memory controller since no one has this.
	     next_hop = new sendpkt_cp();
	     next_hop->tile = tile;
	     next_hop->p.addr = p.addr;
	     next_hop->p.type = R_REQ;
	     next_hop->p.x_src = tile->here_x;
	     next_hop->p.y_src = tile->here_y;
	     next_hop->p.delta_x = ROW_SIZE;
	     next_hop->p.delta_y = 0;
	     next_hop->p.rid = rid;
	     DBGTR << "READ_I(0x" << hex << p.addr << "): Doing R_REQ.\n";
	     CONT_CALL(next_hop);
	     DBGTR << "READ_I(0x" << hex << p.addr << "): Finished R_REQ.\n";
	     delete(next_hop);
	   }

	   // Send RESP/RESP_E back to requestor.
	   next_hop = new sendpkt_cp();
	   next_hop->tile = tile;
	   next_hop->p.addr = p.addr;
	   if (tile->directory[p.addr].present.size() == 1)
	     next_hop->p.type = RESP_E;
	   else
	     next_hop->p.type = RESP;
	   next_hop->p.x_src = tile->here_x;
	   next_hop->p.y_src = tile->here_y;
	   next_hop->p.delta_x = p.x_src - tile->here_x;
	   next_hop->p.delta_y = p.y_src - tile->here_y;
	   DBGTR << "READ_I(0x" << hex << p.addr << "): Doing RESP(E).\n";
	   next_hop->p.rid = rid;
	   CONT_CALL(next_hop);
	   DBGTR << "READ_I(0x" << hex << p.addr << "): Finished RESP(E).\n";
	   p = next_hop->p;
	   delete next_hop;

	 } else if (p.type == RESP) {
	   DBGTR << "RESP\n";
	   // Return immediately. Collapse the call chain.

	 } else if (p.type == FAIL_RESP) {
	   DBGTR << "FAIL_RESP\n";
	   // Same as RESP.

	 } else if (p.type == RESP_E) {
	   DBGTR << "RESP_E\n";
	   // Same as RESP.

	 } else if (p.type == INV) {
	   DBGTR << "INV(0x" << hex << p.addr << ")\n";
	   if (tile->whohas.find(p.addr) != tile->whohas.end()) {
	     DBGTR << "INV line was in whohas.\n";
	     if (tile->whohas[p.addr] == NULL) {
	       // Line is in the MSHR. Set invalidated flag and wait on it.
	       DBGTR << "Invalidating in MSHR; not waiting.\n";
	       tile->mshrs[p.addr].invalidated = true;
	       //mshr_wait.addr = p.addr; mshr_wait.tile = tile;
	       //CONT_CALL(&mshr_wait);
	     } else {
	       DBGTR << "Invalidating in array.\n";
	       // Line is in the array. Grab it and invalidate it.
	       tile->whohas[p.addr]->get(p.addr)->set_state('i');
	       tile->whohas.erase(p.addr);
	     }
	   }

	   // Now delete the line from this tile's whohas directory.
	   //tile->whohas.erase(p.addr);

	   // The MSHR will be deleted or re-reserved and cleared later.

	 } else if (p.type == CLEAR_P) {
	   DBGTR << "CLEAR_P(0x" << hex << p.addr << ")\n";
           tile->dir_reads++;
	   tile->dir_writes++;
	   // Erase the line from the bitmap.
	   if (tile->directory.find(p.addr) != tile->directory.end()) {
	     tile->directory[p.addr].present.erase(pair<unsigned, unsigned>(
						     p.x_src, p.y_src));
	     if (tile->directory[p.addr].present.size() == 0)
	       tile->directory.erase(p.addr);
	   }

	   // Send RESP back to requestor.
	   next_hop = new sendpkt_cp();
	   next_hop->p.addr = p.addr;
	   next_hop->tile = tile;
	   next_hop->p.type = RESP;
	   next_hop->p.delta_x = p.x_src - tile->here_x;
	   next_hop->p.delta_y = p.y_src - tile->here_y;
	   next_hop->p.rid = rid;
	   CONT_CALL(next_hop);
	   p = next_hop->p;
	   delete next_hop;

	 } else if (p.type == C_REQ) {
           // Really, this should be able to fail. If a line is invalidated
           // while our C_REQ is en route, it is currently magically forwarded,
           // as though a buffer keeps old lines around until they are 
           // guaranteably unneeded.

	   DBGTR << "CREQ(0x" << hex << p.addr << ")\n";
	   if (tile->whohas.find(p.addr) == tile->whohas.end()) {
	     DBGTR << "Line was invalidated before a CREQ could arrive.\n";
	     exit(1);
	   }

	   if (tile->whohas[p.addr] == NULL) {
	     mshr_wait.addr = p.addr; mshr_wait.tile = tile;
	     DBGTR << "Waiting on MSHR for 0x" << hex << p.addr << '\n';
	     CONT_CALL(&mshr_wait);
	     DBGTR << "Done waiting on MSHR for 0x" << hex << p.addr << '\n';

	     if (tile->mshrs.find(p.addr) != tile->mshrs.end())
	       s = tile->mshrs[p.addr].state;
	   } else {
	     s = tile->whohas[p.addr]->get(p.addr)->get_state();
	   }

	   if (tile->whohas.find(p.addr) != tile->whohas.end()) {
	     if (s == 'm') {
	       // Send write request to the memory controller.
	       next_hop = new sendpkt_cp();
	       next_hop->tile = tile;
	       next_hop->p.type = W_REQ;
	       next_hop->p.addr = p.addr;
	       next_hop->p.delta_x = ROW_SIZE;
	       next_hop->p.delta_y = 0;
	       next_hop->p.x_src = tile->here_x;
	       next_hop->p.y_src = tile->here_y;
	       next_hop->p.rid = rid;
	       CONT_CALL(next_hop);
	       delete(next_hop);
	     }
	   }

	   // If line is still here, change state to 's'
	   if (tile->whohas.find(p.addr) != tile->whohas.end()) {
	     if (tile->whohas[p.addr] == NULL) {
	       //mshr_wait.addr = p.addr; mshr_wait.tile = tile;
	       //CONT_CALL(&mshr_wait); // FIX?

               // I don't want to arbitrarily put a (possibly recently or 
               // modified) line in the shared state.
               // TODO: add a "most recent write" to a line so I can know 
               // whether the line being made shared should really be shared or
               // modified. 
	       tile->mshrs[p.addr].state = 's';
	     } else {
	       tile->whohas[p.addr]->get(p.addr)->set_state('s');
	     }
	   }

	   // Send RESP back to directory.
	   next_hop = new sendpkt_cp();
	   next_hop->tile = tile;
	   next_hop->p.addr = p.addr;
	   next_hop->p.type = RESP;
	   next_hop->p.delta_x = p.x_src - tile->here_x;
	   next_hop->p.delta_y = p.y_src - tile->here_y;
	   next_hop->p.x_src = tile->here_x;
	   next_hop->p.y_src = tile->here_y;
	   next_hop->p.rid = rid;
	   CONT_CALL(next_hop);
	   p = next_hop->p;
	   delete next_hop;

	 } else {
	   // R_REQ or W_REQ delivered to the wrong tile.
	   cerr << "RAM action delivered to tile. Error.\n";
	   exit(1);
	 }

	 if (tile->directory.find(p.addr) != tile->directory.end()) {
	   show_dir_ent(p.addr, tile->directory[p.addr].present);
	 }
       } else {
	 // Send packet along to destination.
	 // Temporary: Just give it a 5-cycle latency.
	 DBGTR << "Routing " << packet_type_s[p.type] << " packet from (" 
	       << dec << p.x_src << ", " << p.y_src << ")\n";

	 if (p.delta_x > 0 && tile->here_x == (ROW_SIZE-1)) {
	   CONT_CALL(&delay);

	   // Do the memory access and then send the RESP back to src.
	   mem_req.write = p.type == W_REQ;
	   mem_req.y = tile->here_y;
	   mem_req.addr = p.addr;
	   CONT_CALL(&mem_req);

	   // TODO: Model contention
	   // Temporary: Just give it a 5-cycle latency.

	   CONT_CALL(&delay);

	   next_hop = new sendpkt_cp();
	   next_hop->tile = tile;
	   next_hop->p = p;
	   next_hop->p.type = RESP;
	   next_hop->p.delta_x = p.x_src - tile->here_x;
	   next_hop->p.delta_y = p.y_src - tile->here_y;

	   CONT_CALL(next_hop);
	   p = next_hop->p;

	   delete next_hop;
	 } else {
	   next_hop = new sendpkt_cp();
	   next_hop->p = p;
	   if (p.delta_x > 0) {
	     next_hop->p.delta_x--;
	     next_hop->tile = tile->tile_e;
	     CONT_WAIT(tile->e_busy, false); tile->e_busy = true;
	     CONT_CALL(&proc_delay);
	     tile->e_busy = false;
	   } else if (p.delta_x < 0) {
	     next_hop->p.delta_x++;
	     next_hop->tile = tile->tile_w;
	     CONT_WAIT(tile->w_busy, false); tile->w_busy = true;
	     CONT_CALL(&proc_delay);
	     tile->w_busy = false;
	   } else if (p.delta_y > 0) {
	     next_hop->p.delta_y--;
	     next_hop->tile = tile->tile_s;
	     CONT_WAIT(tile->s_busy, false); tile->s_busy = true;
	     CONT_CALL(&proc_delay);
	     tile->s_busy = false;
	   } else if (p.delta_y < 0) {
	     next_hop->p.delta_y++;
	     next_hop->tile = tile->tile_n;
	     CONT_WAIT(tile->n_busy, false); tile->n_busy = true;
	     CONT_CALL(&proc_delay);
	     tile->n_busy = false;
	   }
	   CONT_CALL(&delay);
	   CONT_CALL(next_hop);
	   p = next_hop->p;
	   delete next_hop;
	 }
       }
       CONT_END();
     }
   };

   class broadcast_inv_cp;

   // One of the elements of a broadcast_inv_cp.
   class single_inv_cp : public cont_proc_t {public:
     // Parameters
     broadcast_inv_cp *my_broadcast_inv;
     pair<unsigned, unsigned> dest;

     // Local variables
     sendpkt_cp send;
     uint64_t rid;

     single_inv_cp(broadcast_inv_cp *mbi, unsigned x, unsigned y, uint64_t rid):
       my_broadcast_inv(mbi), dest(x, y), rid(rid) {}

     cont_proc_t *main() {
       CONT_BEGIN();
       send.tile = my_broadcast_inv->tile;
       send.p.type = INV;
       send.p.x_src = my_broadcast_inv->tile->here_x;
       send.p.y_src = my_broadcast_inv->tile->here_y;
       send.p.delta_x = dest.first - send.p.x_src;
       send.p.delta_y = dest.second - send.p.y_src;
       send.p.addr = my_broadcast_inv->addr;
       send.p.rid = rid;
       CONT_CALL(&send);

       // We've finished our invalidate. If they've all finished, tell the
       // broadcast_inv_cp that it's done.
       my_broadcast_inv->pending_invs--;
       if (my_broadcast_inv->pending_invs == 0) 
	 my_broadcast_inv->finished = true;
       CONT_END();
     }
   };

   // This sends invalidate packets to all of the owners of a line and waits for
   // all of the responses to come back.
   class broadcast_inv_cp : public cont_proc_t {public:
     // Parameters
     Tile      *tile;    // Source tile.
     set<pair<unsigned, unsigned> >*p_bits; // Present bits for given address.
     uint64_t   addr;
     uint64_t   rid;

     // Values accessed by single_inv_cp
     unsigned total_invs;
     unsigned pending_invs;
     cont_cond_t finished;

     // Must be constructed immediately before this proc is spawned.
     broadcast_inv_cp(Tile *t, set<pair<unsigned, unsigned> > &s, 
		      uint64_t a, uint64_t r) :
       tile(t), p_bits(&s), addr(a), rid(r) {}

     // Return Values

     // Local variables
     sendpkt_cp send;
     std::vector<single_inv_cp*> reqs; // The request cont_procs.
     set<pair<unsigned, unsigned> >::iterator it;
     unsigned i;

     cont_proc_t *main() {
       CONT_BEGIN();

       DBGTR << "Broadcasting INV for 0x" << hex << addr << '(' 
	     << dec << p_bits->size() << " destinations)\n";

       // If there's no one to send to, return immediately.
       if (p_bits->size() == 0) CONT_RET();

       // Set the pending_inv flag and count.
       total_invs = p_bits->size();
       pending_invs = total_invs;
       finished = false;

       // Initialize the req processes.
       reqs.resize(pending_invs);

       // Spawn all of the req processes.
       for (it = p_bits->begin(), i = 0; it != p_bits->end(); it++, i++) {
	 reqs[i] = new single_inv_cp(this, it->first, it->second, rid);
	 DBGTR << "New INV pkt for 0x" << hex << addr << " to (" << it->first 
	       << ", " << it->second << ").\n";
	 spawn_cont_proc(reqs[i]);
       }

       // Wait for them to finish.
       CONT_WAIT(finished, true);

       DBGTR << "INV packets have finished.\n";

       CONT_END();
     };
   };

   class dir_req_cp : public cont_proc_t {public:
     // Parameters
     Tile*         tile;
     packet_type_t req_type;
     uint64_t      addr;
     uint64_t      rid;

     // Return Values
     bool success;
     bool exclusive;

     // Local variables
     sendpkt_cp send;

     cont_proc_t *main() {
       CONT_BEGIN();
       send.tile = tile;

       tile->packets_sent++;

       // Assemble packet. First stop, directory.
       send.p.type = req_type;
       send.p.addr = addr;
       send.p.x_src = tile->here_x;
       send.p.y_src = tile->here_y;
       send.p.rid = rid; 

       // The destination is the directory unless it's a RAM request. We still
       // use dir_req to handle these because it sets everything else up nicely.
       if (req_type == R_REQ || req_type == W_REQ) {
	 send.p.delta_x = ROW_SIZE;
	 send.p.delta_y = 0;
       } else {
	 send.p.delta_x = 
	   (addr>>(BPB_LOG2+ROW_SIZE_L2)&(ROW_SIZE-1)) - tile->here_x;
	 send.p.delta_y = 
	   (addr>>BPB_LOG2 & (ROW_SIZE-1)) - tile->here_y;
       }

       DBG << "New packet: (" << dec << tile->here_x << ", " << tile->here_y 
	   << ")->0x" << hex << addr << '(' << dec 
	   << send.p.delta_x + tile->here_x << ", "
	   << send.p.delta_y + tile->here_y << ")\n";

       // Call sendpkt_cp from this tile with assembled packet.
       CONT_CALL(&send);

       // Extract our return values from the packet.
       exclusive = (send.p.type == RESP_E);
       success   = (send.p.type != FAIL_RESP);

       CONT_END();
     }
   };

   // B, C, D, E, G. Caches, Bus, and Coherency

   // This keeps other hardware threads from touching the L2 while an eviction 
   // is taking place. It's the equivalent of stalling the pipeline to prevent
   // hazards.
   cont_cond_t l2_locked;

   // Evict lines from cache as necessary to make room for a new one. Returns a
   // valid pointer to an invalid line in this_c.
   class evict_cp : public cont_proc_t { public:
     // Parameters
     Tile *tile;
     uint64_t addr;
     cache_t *this_c;

     // Return values.
     cache_t::line_t *line;

     // Local variables.
     cache_t::line_t *l2_line;
     dir_req_cp dir_req;
     uint64_t rid;

     cont_proc_t *main() {
       CONT_BEGIN();

       // Try to find a victim line in L1.
       this_c->choose_victim(&line, addr);

       DBGT << "Evict: 0x" << hex << line->addr << '(' << line->get_state() 
	    << ")\n";

       // If it's aready invalid we can return immediately.
       if (line->get_state() == 'i') {
	 CONT_RET();
       }

       // Lock the L2 cache so nothing gets swapped out from under us.
       DBGT << "Locking L2\n";
       CONT_WAIT(tile->l2_locked, false); tile->l2_locked = true;
       DBGT << "Locked L2\n";

       // Find a victim line in L2.
       tile->l2u.choose_victim(&l2_line, addr);
       DBG << "L2 evict: 0x" << hex << l2_line->addr << '(' 
	   << l2_line->get_state() << ")\n";

       // If it's valid, evict it from this tile.
       if (l2_line->get_state() != 'i') {

	 dir_req.tile = tile;
	 dir_req.addr = l2_line->addr;
	 dir_req.rid = rid;

	 // If it's been modified, write it back. We can guarantee no new writes
	 // will come in for this line because the L2 is locked.
	 if (l2_line->get_state() == 'm') {
	   tile->l2u.reads++;
	   tile->l2u.read_hits++;
	   dir_req.req_type = W_REQ;
	   CONT_CALL(&dir_req);
	 }

	 // Finish 'm' eviction / do 's' or 'e' eviction.
	 dir_req.req_type = CLEAR_P;
	 CONT_CALL(&dir_req);
	 tile->whohas.erase(l2_line->addr);
	 l2_line->set_state('i');
	 DBGT << "Just invalidated 0x" << hex << l2_line->addr << " in L2\n";
       }

       // Find a new victim line in L1 to evict to L2.
       this_c->choose_victim(&line, addr);

       // If this line is valid, copy it to L2, update whohas, and invalidate.
       if (line->get_state() != 'i') {
	 *l2_line = *line;
	 tile->whohas[line->addr] = &tile->l2u;
	 DBGTR << "whohas[" << hex << line->addr << "] = " << &tile->l2u 
               << ";\n";
	 line->set_state('i');
       }

       DBGT << "Eviction finished. Unlocking L2.\n";
       tile->l2_locked = false;

       CONT_END();
     }
   };

   class swap_cp : public cont_proc_t { public:
     // Parameters
     Tile *tile;
     cache_t *this_c, *that_c;
     uint64_t addr;

     // Local variables
     cache_t::line_t *incoming_line;
     cache_t::line_t *outgoing_line;
     cache_t::line_t tmp;
     delay_cpt delay;

     cont_proc_t *main() {
       CONT_BEGIN();

       tile->bus_transfers++;

       DBGT << "Performing swap for 0x" << hex << addr << '\n';

       // If I'm swapping with L2 I have to obtain the lock. Otherwise this
       // might interfere with an eviction from L2.
       if (that_c == &tile->l2u) {
	 DBGT << "  Locking L2.\n";
	 CONT_WAIT(tile->l2_locked, false); tile->l2_locked = true;
	 delay.t = L2_L1_SWAP_T;
       } else {
	 DBGT << "  Doing swap with other L1 cache.\n";
	 delay.t = D_I_SWAP_T;
       }

       incoming_line = that_c->get(addr);

       // Our other line may have been evicted from L2 while we were obtaining a
       // lock on L2. If so, return without performing the swap. Since this is
       // the only way it could have vanished, we will always unlock l2 without
       // checking.
       if (incoming_line == NULL) { tile->l2_locked = false; CONT_RET(); }

       // Select a victim line.
       this_c->choose_victim(&outgoing_line, addr);

       // Perform the swap.
       DBGT << "Swapping that_c-0x" << hex << incoming_line->addr << '(' 
	    << incoming_line->get_state() << ")/this_c-0x" << hex 
	    << outgoing_line->addr << '(' << outgoing_line->get_state()
	    << ")\n";
       tmp = *outgoing_line;
       *outgoing_line = *incoming_line;
       *incoming_line = tmp;
       if (outgoing_line->get_state() != 'i') { 
	 tile->whohas[outgoing_line->addr] = this_c;
         DBGT << "whohas[" << hex << outgoing_line->addr << "] = "
              << this_c << ";\n";
	 DBGT << "this_c now has 0x" << hex << outgoing_line->addr << '('
	      << outgoing_line->get_state() << ")\n";
	 this_c->reads++;
	 this_c->read_hits++;
       }
       if (incoming_line->get_state() != 'i') {
	 tile->whohas[incoming_line->addr] = that_c;
         DBGT << "whohas[" << hex << incoming_line->addr << "] = "
              << that_c << ";\n";
	 DBGT << "that_c now has 0x" << hex << incoming_line->addr << '(' 
	      << incoming_line->get_state() << ")\n";
	 that_c->writes++;
	 that_c->write_hits++;
       }

       if (that_c == &tile->l2u) {
	 tile->l2_locked = false;
	 DBGT << "  Unlocking L2\n";
       }

       CONT_CALL(&delay);

       CONT_END();
     }
   };

   // Access memory system through i-cache or d-cache.
   class generic_ac_cp 
     : public cont_proc_t 
   { public:
     // Instance Parameters
     bool write;
     bool is_d;

     // Per-call Parameters
     Tile *tile;
     cache_t *this_c, *that_c;
     uint64_t addr;
     uint64_t rid;

     // Local variables
     delay_cpt delay;
     swap_cp   swap;
     evict_cp  evict;
     cache_t *owner;
     mshr_wait_cp mshr_wait;
     dir_req_cp   dir_req;
     hw_thread_cp *my_thread;
     bool swapped_thread;

     generic_ac_cp(cache_t *this_c, cache_t *that_c, bool write) 
       : write(write), this_c(this_c), that_c(that_c) {}

     void thread_swap() {
       // Don't let the same access cause multiple thread-swaps.
       if (swapped_thread == true) return;
       swapped_thread = true;
       if (!tile->thread_q.empty()) tile->thread_q.pop();
       if (!tile->thread_q.empty()) {
	 tile->thread_q.front()->my_turn = true;
       }
     }

     void thread_putback() {
       if (swapped_thread) {
	 my_thread->my_turn = false;
	 tile->thread_q.push(my_thread);
	 tile->thread_q.front()->my_turn = true;
       }
     }

     cont_proc_t *main() {
       CONT_BEGIN();
       my_thread = static_cast<hw_thread_cp*>(_p);
       swapped_thread = false;
       dir_req.tile = tile;
       dir_req.rid = rid;

       // Truncate address to block address and discard LSBs.
       addr = addr & ~((1<<BPB_LOG2) - 1);

       // First, if line is in an MSHR, wait for it to become present.It may be
       // invalidated after its time in the MSHR,so we may very well still miss
       // here. Or we may hit in the MSHR.
       if (tile->whohas.find(addr)!=tile->whohas.end() && !tile->whohas[addr]){
	 //DBGTR << "Allowing other threads to run.\n";
	 //thread_swap();
	 DBGTR << "Waiting for 0x" << hex << addr << " in MSHR.\n";
	 mshr_wait.tile = tile; mshr_wait.addr = addr;
	 CONT_CALL(&mshr_wait);
       }

       // Count the access.
       (write?this_c->writes:this_c->reads)++;

       // Use whohas to determine status of line.
       if (tile->whohas.find(addr) != tile->whohas.end()) {
	 if (tile->whohas[addr] == NULL) {
	   // This is an MSHR hit.
	   (write?this_c->write_hits:this_c->read_hits)++;

	 } else if (tile->whohas[addr] == this_c) {
	   // This is a local hit.
	   (write?this_c->write_hits:this_c->read_hits)++;

	 } else {
	   // Must perform swap between caches on bus.That's a read hit for the
	   // other cache.
	   tile->whohas[addr]->reads++; tile->whohas[addr]->read_hits++;

	   // Give other threads a chance to run.
	   //thread_swap();

	   if (tile->whohas[addr]->get(addr)->get_state() == 'i') {
	     DBGTR << "DANGER: 0x" << hex << addr << " actually invalid.\n";
	     exit(1);
	   }

	   // Perform swap between caches.
	   swap.addr = addr; 
	   swap.this_c = this_c; 
	   swap.that_c = tile->whohas[addr]; 
	   swap.tile = tile;
	   CONT_CALL(&swap);
	 }
       }

       // We may have just performed a swap that took time. The line may have
       // been invalidated during that time. We have to check again before we
       // complete the access.
       if (tile->whohas.find(addr) != tile->whohas.end()) {
	 if ((tile->whohas[addr]&&tile->whohas[addr]->get(addr)->get_state() 
		== 'm')||
	     (!tile->whohas[addr]&&tile->mshrs[addr].state == 'm')) 
	 {
	   // I can perform reads and writes with no further action.
	   thread_putback(); CONT_WAIT(my_thread->my_turn, true);
	   CONT_RET();
	 } else if 
	    ((tile->whohas[addr]&&tile->whohas[addr]->get(addr)->get_state()
	      == 'e')||
	    (!tile->whohas[addr]&&tile->mshrs[addr].state == 'e')) 
	 {
	   // I can perform reads with no further action.
	   if (write) {
	     // For writes, I must set the state to 'm'
            if (tile->whohas[addr]) {
	      tile->whohas[addr]->get(addr)->set_state('m');
            } else {
	      tile->mshrs[addr].state = 'm';
	    }
	  }
	  thread_putback(); CONT_WAIT(my_thread->my_turn, true);
          CONT_RET();
        } else if 
	  ((tile->whohas[addr]&&tile->whohas[addr]->get(addr)->get_state()
              == 's')||
           (!tile->whohas[addr]&&tile->mshrs[addr].state == 's')) 
        {
          // I can perform reads with no further action.
          if (!write) {
            thread_putback(); CONT_WAIT(my_thread->my_turn, true);
            CONT_RET();
          }
          // Give other threads a chance to run.
          //thread_swap();

          // For writes we must first try a WRITE_S, but that can fail and then
          // we must perform a WRITE_I.
          dir_req.addr = addr; dir_req.req_type = WRITE_S;
	  CONT_CALL(&dir_req);
	  if (dir_req.success && tile->whohas.find(addr) != tile->whohas.end())
          {
            if (tile->whohas[addr]) {
              tile->whohas[addr]->get(addr)->set_state('m');
            } else {
	      tile->mshrs[addr].state = 'm';
            }
	    thread_putback(); CONT_WAIT(my_thread->my_turn, true);
            CONT_RET();
          }
	}
      }

      // By the time we have reached this point, for one reason or another our
      // line is now invalid and we have to make network requests to get it.
      
      // Create an MSHR entry for the line.
      tile->whohas[addr] = NULL;
      DBGT << "whohas[" << hex << addr << "] = NULL;\n";
      this_c->reserve_mshr(addr);

      // Perform appropriate action in the network.
      dir_req.addr = addr; dir_req.req_type = write?WRITE_I:READ_I;
      //DBGTR << "Allowing other threads to run.\n";
      //thread_swap();
      CONT_CALL(&dir_req);

      if (write) {
        tile->mshrs[addr].state = 'm';
        DBGTR << "0x" << hex << addr << " now in 'm'.\n";
      } else if (dir_req.exclusive) {
        tile->mshrs[addr].state = 'e';
        DBGTR << "0x" << hex << addr << " now in 'e'.\n";
      } else {
        tile->mshrs[addr].state = 's';
        DBGTR << "0x" << hex << addr << " now in 's'.\n";
      }

      this_c->signal_mshr(addr);

      if (tile->mshrs[addr].invalidated == false) {
        // Evict a line to make room for the line in the MSHR.
        DBGTR << "Finding a line to hold 0x" << hex << addr << '(' 
             << tile->mshrs[addr].state << ")\n";
        evict.tile = tile; evict.this_c = this_c; evict.addr = addr;
	evict.rid = rid;
        //thread_swap();
        CONT_CALL(&evict);
      }

      // May have been invalidated during eviction.
      if (tile->mshrs[addr].invalidated == false) {
        // Set up the new line.
        DBGTR << "Putting 0x" << hex << addr << "(" << tile->mshrs[addr].state 
              << ") in the array.\n";
        evict.line->addr = addr;
	evict.line->set_state(tile->mshrs[addr].state);
        tile->whohas[addr] = this_c;
        DBGTR << "whohas[" << hex << addr << "] = " << this_c << ";\n";
      } else {
	tile->whohas.erase(addr);
      }

      this_c->delete_mshr(addr);

      thread_putback(); CONT_WAIT(my_thread->my_turn, true);
      CONT_END();
    }
  };
};

void inst_cb(int hwt, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *) 
{
  // Figure out the corresponding tile.
  Tile *t = tiles[hwt/THREADS];

  // Set that tile's instruction.
  t->inst_addr = pa;
  t->inst_len = l;
}

// Simulating full/empty bits by converting readFE, writeEF operations to
// writes; operations which would have identical timing effects.
struct feb_support_t {
  enum thread_state_t {NORMAL = 0, ATOMIC_PREREAD, ATOMIC_PREWRITE};
  vector <thread_state_t> thread_state;
  vector <uint64_t>       op_count;
  map <uint64_t, bool>    empty;        // Actual simulated FEBs.

  feb_support_t(unsigned n_threads) : 
    thread_state(n_threads), op_count(n_threads) 
  {
    for (unsigned i = 0; i < n_threads; i++) {
      ostringstream oss;
      oss << dec << setw(2) << setfill('0') << i;
      register_counter("F/E Bit Support", "F/E Bit Ops " + oss.str(), 
                       op_count[i]);
    }
    
  }

  void return_fail(int hwt) { cd->set_reg(hwt, QSIM_RCX, 0); }
  void return_succ(int hwt) { cd->set_reg(hwt, QSIM_RCX, 1); }

  void feb_op(unsigned hwt, uint64_t pa) {
    op_count[hwt]++;
  }

  void mem_cb(int hwt, uint64_t va, uint64_t pa, uint8_t s, int type) {
    switch (thread_state[hwt]) {
    case ATOMIC_PREREAD:  type = 1; thread_state[hwt] = ATOMIC_PREWRITE; 
                          feb_op(hwt, pa); break;
    case ATOMIC_PREWRITE: type = 1; thread_state[hwt] = NORMAL; 
                          feb_op(hwt, pa); break;
    default: /* NORMAL; do nothing. */ ;
    }

    Tile *t = tiles[hwt/THREADS];
    t->cur_thread->mem_ops.push(memop_t(pa, type));
 }

 int magic_cb(int hwt, uint64_t rax) {
    // Only handle F/E bit related calls.
    if ( (rax & 0xffffff00) != 0xcd16fe00 ) return 0;

    Tile *t = tiles[hwt/THREADS];

    uint64_t rcx = cd->get_reg(hwt, QSIM_RCX);
    uint64_t rdx = cd->get_reg(hwt, QSIM_RDX);
    bool write;

    feb_op(hwt, rcx);

    switch (rax & 0xff) {
    case 0x00: /* status */
      rax = empty[rcx];
      cd->set_reg(hwt, QSIM_RAX, rax);
      return_succ(hwt);
      write = false;
      break;

    case 0x01: /* empty */
      empty[rcx] = true;
      return_succ(hwt);
      write = true;
      break;

    case 0x02: /* fill */
      empty.erase(rcx);
      return_succ(hwt);
      write = true;
      break;

    case 0x03: /* writeEF */
      if (empty[rcx] == true) {
	// If I can, do the write and set full.
	cd->mem_wr_virt(hwt, rdx, rcx);
	empty.erase(rcx);
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    case 0x05: /* writeF */
      cd->mem_wr_virt(hwt, rdx, rcx);
      empty.erase(rcx);
      return_succ(hwt);
      write = true;
      break;

    case 0x07: /* readFF */
      if (empty[rcx] == false) {
	// If I can, do the read.
	uint64_t new_rax;
	cd->mem_rd_virt(hwt, new_rax, rcx);
	cd->set_reg(hwt, QSIM_RAX, new_rax);
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    case 0x09: /* readFE */
      if (empty[rcx] == false) {
	// If I can, do the read and set the empty bit.
	uint64_t new_rax;
	cd->mem_rd_virt(hwt, new_rax, rcx);
	cd->set_reg(hwt, QSIM_RAX, new_rax);
	empty[rcx] = true;
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    default: 
      write = false;
      cout << "Invalid F/E call.\n";
      exit(1);
    }

    t->cur_thread->mem_ops.push(memop_t(rcx, write));

    return 0;
  }

  int int_cb(int hwt, uint8_t vector) {
    if (vector == 14 && 
	(thread_state[hwt] == ATOMIC_PREREAD || 
	 thread_state[hwt] == ATOMIC_PREWRITE)) 
    {
      // Current atomic operation cancelled by page fault.
      thread_state[hwt] = NORMAL;
    }
    return 0;
  }

  int atomic_cb(int hwt) {
    thread_state[hwt] = ATOMIC_PREREAD;
    return 0;
  }

} *feb_support;

void start_cb(int) { benchmark_running  = true;  }
void end_cb(int)   { simulation_running = false; 
                     _sstComponent->unregisterExit(); }

void print_usage(const char* command) {
  cout << "Usage:\n  " << command << " [image file]\n";
}

Mesmthi::Mesmthi(ComponentId_t id, Params_t& params) : IntrospectedComponent(id) {
  std::string clock_pd;
  // The parameters:
  //   log2_row_size:        log_2 of N for an NxN mehs
  //   threads:              number of threads per tile
  //   fetch_bytes:          number of bytes that can be fetched in a cycle
  //   log2_bytes_per_block: log_2 # bytes per cache block
  //   log2_sets:            log_2 # sets in L1/L2 caches
  //   i_ways:               # ways in L1 I-cache
  //   d_ways:               # ways in L1 D-cache
  //   l2_ways:              # ways in L2 cache
  //   d_i_swap_t:           penlaty for swap between L1 D$ and I$
  //   l2_l1_swap_t:         penalty for swap between L1 and L2
  //   cyc_per_tick:         sim cycles per tick of the interval timer
  //   paddr_bits:           log_2 of physical address space size
  //   net_latency:          # of cycles to move by one tile on the network
  //   net_cpp:              cycles per packet, 1/throughput
  read_param(params, "log2_row_size",        log2_row_size,        2);
  read_param(params, "threads",              threads,              2);
  read_param(params, "fetch_bytes",          fetch_bytes,          8);
  read_param(params, "log2_bytes_per_block", log2_bytes_per_block, 6);
  read_param(params, "log2_sets",            log2_sets,            9);
  read_param(params, "i_ways",               i_ways,               2);
  read_param(params, "d_ways",               d_ways,               2);
  read_param(params, "l2_ways",              l2_ways,             16);
  read_param(params, "d_i_swap_t",           d_i_swap_t,          20);
  read_param(params, "l2_l1_swap_t",         l2_l1_swap_t,         5);
  read_param(params, "cyc_per_tick",         cyc_per_tick,   4000000);
  read_param(params, "samp_per_tick",        samp_per_tick,      400);
  read_param(params, "paddr_bits",           paddr_bits,          32);
  read_param(params, "net_latency",          net_latency,          5);
  read_param(params, "net_cpp",              net_cpp,              1);
  read_param(params, "clock_pd",             clock_pd,       "500ps");
  read_param(params, "kernel_img",        kernel_img,"linux/bzImage");
  read_param(params, "push_introspector", pushIntrospector,"CountIntrospector");

  // The simulation won't end until we unregisterExit().
  registerExit();

  // Initialize the discrete event simulator.
  Slide::des_init(this, clock_pd);

  // Initialize our CDomain
  cd = new Qsim::CDomain((1<<log2_row_size)*(1<<log2_row_size)*threads, 
                         kernel_img,
                         (1<<(paddr_bits - 20)) - 512);
  cd->connect_console(std::cout);
  cd->set_app_start_cb(start_cb);

  // Fast forward to the beginning of the benchmark.
  while (!benchmark_running) {
    for (unsigned k = 0; k < 1000; k++) {
      for (unsigned j = 0; 
           j < (1<<log2_row_size)*(1<<log2_row_size)*threads; 
           j++) 
      {
        cd->run(j, 1000);
      }
    }
    cd->timer_interrupt();
  }

  // Set up full-empty bit support
  feb_support = new feb_support_t(threads*(1<<(log2_row_size*2)));

  // Set callbacks.
  cd->set_inst_cb(inst_cb);
  cd->set_mem_cb(feb_support, &feb_support_t::mem_cb);
  cd->set_app_end_cb(end_cb);
  cd->set_magic_cb(feb_support, &feb_support_t::magic_cb);
  cd->set_atomic_cb(feb_support, &feb_support_t::atomic_cb);
  cd->set_int_cb(feb_support, &feb_support_t::int_cb);

  // Instantiate the memory controller.
  mc = new MemoryController();

  // Instantiate the tiles.
  tiles.resize((1<<(log2_row_size*2)));
  for (unsigned i = 0; i < (1u<<(log2_row_size*2)); i++) 
    tiles[i] = new Tile(threads*i, i%(1<<log2_row_size), i>>log2_row_size);

  // Wire up our mesh
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++) {
    if (i >= (1<<log2_row_size))
      tiles[i]->connect_n(*tiles[i-(1<<log2_row_size)]);
    if (i%(1<<log2_row_size) != ((1<<log2_row_size)-1))
      tiles[i]->connect_e(*tiles[i+1]);
    if (i < (1<<log2_row_size)*((1<<log2_row_size)-1))
      tiles[i]->connect_s(*tiles[i+(1<<log2_row_size)]);
    if (i%(1<<log2_row_size) != 0)
      tiles[i]->connect_w(*tiles[i-1]);
  }

  // Do post wire-up initialization.
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++) tiles[i]->reset();

  // Create an interval timer to deliver interrupts.
  Slide::spawn_cont_proc(new timer_cp());

  clear_counters();

  //Introspection: register monitors (counters)
  ostringstream setname;
  registerMonitor("MCreads", new MonitorPointer<uint64_t>(&mc->reads));
  registerMonitor("MCwrites", new MonitorPointer<uint64_t>(&mc->writes));
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++){
        setname << "Tile[" << i << "]DIRreads";
        registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->dir_reads));
        setname << "Tile[" << i << "]DIRwrites";
        registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->dir_writes));

	setname << "Tile[" << i << "]L1Ireads";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1i.reads));
	setname << "Tile[" << i << "]L1Iwrites";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1i.writes));
	setname << "Tile[" << i << "]L1Ireadhits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1i.read_hits));
	setname << "Tile[" << i << "]L1Iwritehits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1i.write_hits));

	setname << "Tile[" << i << "]L1Dreads";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1d.reads));
	setname << "Tile[" << i << "]L1Dwrites";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1d.writes));
	setname << "Tile[" << i << "]L1Dreadhits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1d.read_hits));
	setname << "Tile[" << i << "]L1Dwritehits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l1d.write_hits));

	setname << "Tile[" << i << "]L2reads";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l2u.reads));
	setname << "Tile[" << i << "]L2writes";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l2u.writes));
	setname << "Tile[" << i << "]L2readhits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l2u.read_hits));
	setname << "Tile[" << i << "]L2writehits";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->l2u.write_hits));

	setname << "Tile[" << i << "]Instructions";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->instructions));
	setname << "Tile[" << i << "]BusTransfers";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->bus_transfers));
	setname << "Tile[" << i << "]NetworkTransactions";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->packets_sent));
	setname << "Tile[" << i << "]TotalPackets";
      registerMonitor(setname.str(), new MonitorPointer<uint64_t>(&tiles[i]->packets_total));
  }
  // TODO: Initialize links to/handlers for DRAMSim
}

extern "C" {
  Mesmthi *mesmthiAllocComponent(SST::ComponentId_t        id,
                                 SST::Component::Params_t& params) {
    return new Mesmthi(id, params);
  }
};

BOOST_CLASS_EXPORT(Mesmthi)
