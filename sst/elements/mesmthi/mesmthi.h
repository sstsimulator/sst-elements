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

// For the purpose of our model, a FEB operation of any kind is identical to a
// write to the same address.

#ifndef _MESHTHI_H
#define _MESMTHI_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/introspectedComponent.h>
#include <string>
#include <sstream>
#include <qsim.h>

#include <sst/elements/include/memoryDev.h>
#include <sst/elements/include/memMap.h>

#define SLIDE_SST
#include "include/des.h"
#undef SLIDE_SST
#include "include/cont_proc.h"

using namespace SST;

// The parameters set these global variables.
unsigned log2_row_size;
unsigned threads;
unsigned fetch_bytes;
unsigned log2_bytes_per_block;
unsigned log2_sets;
unsigned i_ways;
unsigned d_ways;
unsigned l2_ways;
unsigned d_i_swap_t;
unsigned l2_l1_swap_t;
unsigned cyc_per_tick;
unsigned samp_per_tick;
unsigned paddr_bits;
unsigned net_latency;
unsigned net_cpp;

class Tile;
std::vector <Tile*> tiles;

Qsim::CDomain *cd;

bool benchmark_running  = false; // can be used to fast-forward past bootstrap
bool simulation_running = true;  // lets us know when simulation is done

void print_counters();
void clear_counters();

void show_oldest_reqs();

class timer_cp : public Slide::cont_proc_t {public:
  Slide::delay_cpt d;
  unsigned i;
  Slide::cont_proc_t *main() {
    CONT_BEGIN();
    i = 0;
    d.t = cyc_per_tick/samp_per_tick;
    while (simulation_running) {
      CONT_CALL(&d);
      std::cout << "===T/" << std::dec << samp_per_tick << "===\n";
      //below is substituted by introspection
      //print_counters();
      //clear_counters();
      show_oldest_reqs();
      if (++i == samp_per_tick) {
        cd->timer_interrupt();
        std::cout << "===Timer Interrupt===\n";
	i = 0;
      }
    }
    CONT_END();
  }
};

void inst_cb (int, uint64_t, uint64_t, uint8_t, const uint8_t *);
void mem_cb  (int, uint64_t, uint64_t, uint8_t, int);
void start_cb(int);
void end_cb  (int);

// Useful utility function; should probably live in a utility library:
//   Read parameter s from p; if it's not there, set the default value.
template <typename P, typename T, typename U> 
  void read_param(P& p, std::string s, T& val, const U d) 
{
  typename P::iterator it = p.find(s);
  if (it == p.end()) {
    val = d;
  } else {
    std::istringstream iss(it->second);
    iss >> val;
  }
  std::cout << "Got parameter '" << s << "' = " << val << ".\n"; 
}

class Mesmthi : public SST::IntrospectedComponent {
 public:
  Mesmthi(ComponentId_t id, Params_t& params);

  int Setup() {
    // This is called after wire-up.
    // TODO: Initialize communication with DRAMSim

    Slide::run_ready_procs();

    //introspection
    registerIntrospector(pushIntrospector);
    return 0;
  }

  int Finish() {  
    //std::cout << "below is from mesmthi finish" << std::endl;
    //print_counters();
    //std::cout << "below is from mesmthi triggerUpdate" << std::endl;
    //Introspection: ask introspector to pull and print counter values 
    triggerUpdate();
    return 0;
  }


 private:
//  Mesmthi(const Mesmthi &c) {}
    Mesmthi(const Mesmthi &c);
    Mesmthi() :  IntrospectedComponent(-1) {}
 

  std::string kernel_img;
  std::string pushIntrospector;
  


  // Note: This won't work. This simulator is not serializable.
  friend class boost::serialization::access;
  template<class Archive> void serialize(Archive &ar, const unsigned int ver) {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }
  
};
#endif
