// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <typeinfo>

#include <chdl/chdl.h>
#include <chdl/trisimpl.h>

using namespace chdl;
using namespace std;

map<nodeid_t, node> gna;  // Global node array

// This has to be in chdl:: because it seems to be the only way to get
// memory_internal and memory_add_read_port
namespace chdl {
  void gen_syncram(unsigned asize, unsigned dsize, const vector<nodeid_t> &x) {
    // Prototype of CHDL internal memory function as used by CHDL headers. We'll
    // use it here too since we can't use the fixed-sized templates.
    vector<node> memory_internal(
      vector<node> &qa, vector<node> &d, vector<node> &da,
      node w, string filename, bool sync, size_t &id
    );

    std::vector<node> memory_add_read_port(
      size_t id, std::vector<node> &qa
    );

    size_t id;
    unsigned pos = 0;
    vector<node> qa(asize), q, da(asize), d(dsize);

    for (unsigned i = 0; i < asize; ++i) da[i] = gna[x[pos++]];
    for (unsigned i = 0; i < dsize; ++i) d[i] = gna[x[pos++]];
    node w = gna[x[pos++]];
    for (unsigned i = 0; i < asize; ++i) qa[i] = gna[x[pos++]];
    q = memory_internal(qa, d, da, w, "", true, id);
    for (unsigned i = 0; i < dsize; ++i) gna[x[pos++]] = q[i];

    // Additional ports
    while (pos < x.size()) {
      vector<node> xqa(asize), xq;
      for (unsigned i = 0; i < asize; ++i) xqa[i] = gna[x[pos++]];
      xq = memory_add_read_port(id, xqa);
      for (unsigned i = 0; i < dsize; ++i) gna[x[pos++]] = xq[i];
    }
  }
}

void gen_tristate(const vector<nodeid_t> &x) {
  tristatenode t;
  for (unsigned i = 0; i < x.size() - 1; i += 2)
    t.connect(gna[x[i]], gna[x[i+1]]);
  gna[x[x.size()-1]] = t;
}

void gen(string name, const vector<unsigned> &params, const vector<nodeid_t> &i)
{
  if      (name == "lit0")    gna[i[0]] = Lit(0);
  else if (name == "lit1")    gna[i[0]] = Lit(1);
  else if (name == "reg")     gna[i[1]] = Reg(gna[i[0]]);
  else if (name == "inv")     gna[i[1]] = Inv(gna[i[0]]);
  else if (name == "nand")    gna[i[2]] = Nand(gna[i[0]], gna[i[1]]);
  else if (name == "tri")     gen_tristate(i);
  else if (name == "syncram") gen_syncram(params[0], params[1], i);

  else {
    cout << "Could not interpret device type \"" << name << "\"." << endl;
    abort();
  }
}

void read_design_line(istream &in) {
  string dev;
  in >> dev;

  while (in.peek() == ' ') in.get();

  if (dev == "") return;

  vector<unsigned> params;
  if (in.peek() == '<') {
    in.get();
    while (in.peek() != '>') {
      unsigned n;
      in >> n;
      params.push_back(n);
    }
    in.get();
  }

  vector<nodeid_t> nv;
  while (in.peek() != '\n') {
    nodeid_t n;
    in >> n;
    nv.push_back(n);
  }
  in.get();

  gen(dev, params, nv);
}

void read_taps(istream &in, map<string, vector<node> > &outputs, bool tap_io) {
  while (in.peek() == ' ') {
    string tapname;
    in >> tapname;
    while (in.peek() != '\n') {
      nodeid_t n;
      in >> n;
      outputs[tapname].push_back(gna[n]);
      if (tap_io) tap(tapname, gna[n]);
    }
    in.get();
  }
}

void read_inputs(istream &in, map<string, vector<node> > &inputs, bool tap_io) {
  while (in.peek() == ' ') {
    string name;
    in >> name;
    while (in.peek() != '\n') {
      nodeid_t n;
      in >> n;
      inputs[name].push_back(gna[n]);
      if (tap_io) tap(name, gna[n]);
    }
    in.get();
  }
}

void read_inout(istream &in, map<string, vector<tristatenode> > &inout,
                bool tap_io)
{
  while (in.peek() == ' ') {
    string name;
    in >> name;    
    while (in.peek() != '\n') {
      nodeid_t n;
      in >> n;
      inout[name].push_back(tristatenode(gna[n]));
      if (tap_io) tap(name, gna[n]);
    }
    in.get();
  }
}

void read_design(istream &in) {
  while (!!in) read_design_line(in);
}

void read_netlist(istream &in,
                  map<string, vector<node> > &outputs,
                  map<string, vector<node> > &inputs,
                  map<string, vector<tristatenode> > &inout,
                  bool tap_io)
{
  string eat;
  in >> eat; in.get();
  if (eat != "inputs") { cerr << "'inputs' expected.\n"; return; }

  read_inputs(in, inputs, tap_io);

  in >> eat; in.get();
  if (eat != "outputs") { cerr << "'outputs' expected.\n"; return; }

  read_taps(in, outputs, tap_io);

  in >> eat; in.get();
  if (eat != "inout") { cerr << "'inout' expected.\n"; return; }

  read_inout(in, inout, tap_io);

  in >> eat;
  if (eat != "design") { cerr << "'design' expected.\n"; return; }

  read_design(in);
}

void ldnetl(map<string, vector<node> > &outputs,
            map<string, vector<node> > &inputs,
            map<string, vector<tristatenode> > &inout, 
            string filename, bool tap_io)
{
  ifstream infile(filename);
  read_netlist(infile, outputs, inputs, inout, tap_io);

  gna.clear();
}
