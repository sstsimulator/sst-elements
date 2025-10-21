//===------------------------------------------------------------*- c++ -*-===//
//
//                                     shad
//
//      the scalable high-performance algorithms and data structure library
//
//===----------------------------------------------------------------------===//
//
// copyright 2018 battelle memorial institute
//
// licensed under the apache license, version 2.0 (the "license"); you may not
// use this file except in compliance with the license. you may obtain a copy
// of the license at
//
//     http://www.apache.org/licenses/license-2.0
//
// unless required by applicable law or agreed to in writing, software
// distributed under the license is distributed on an "as is" basis, without
// warranties or conditions of any kind, either express or implied. see the
// license for the specific language governing permissions and limitations
// under the license.
//
//===----------------------------------------------------------------------===/

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <ostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <random>
#include <fstream>
#include <iostream>

int scale;

struct RMAT_args_t {
  uint64_t seed;
  uint64_t scale;
  double A;
  double B;
  double C;
  double D;
};

using edges_t = std::map< uint64_t, std::set<uint64_t> >;
edges_t edges;

void RMAT(const RMAT_args_t & args, uint64_t ndx) {
  std::mt19937_64 gen64(args.seed + ndx);
  uint64_t max_rand = gen64.max();
  uint64_t src = 0, dst = 0;

  for (uint64_t i = 0; i < args.scale; i ++) {
    double rand = (double) gen64() / (double) max_rand;
    if      (rand <= args.A)                            { src = (src << 1) + 0; dst = (dst << 1) + 0; }
    else if (rand <= args.A + args.B)                   { src = (src << 1) + 0; dst = (dst << 1) + 1; }
    else if (rand <= args.A + args.B + args.C)          { src = (src << 1) + 1; dst = (dst << 1) + 0; }
    else if (rand <= args.A + args.B + args.C + args.D) { src = (src << 1) + 1; dst = (dst << 1) + 1; }
  }

  if (src != dst) {                          // do not include self edges
    if (src > dst) std::swap(src, dst);     // make src less than dst
    edges[src].insert( dst );
  }
}

int main(int argc, char **argv){
  if (argc != 8) {printf("Usage: <seed> <scale> <edge ratio> <A> <B> <C> <filename>\n"); return 1;}

  uint64_t seed = std::stoll(argv[1]);
  uint64_t scale = std::stoll(argv[2]);
  uint64_t num_vertices = 1 << std::stoll(argv[2]);
  uint64_t target_edges = num_vertices * std::stoll(argv[3]);
  uint64_t num_edges = 0;
  std::string filename(argv[7]);

  double A = std::stod(argv[4]) / 100.0;
  double B = std::stod(argv[5]) / 100.0;
  double C = std::stod(argv[6]) / 100.0;
  double D = 1.0 - A - B - C;

  if ( (A <= 0.0) || (B <= 0.0) || (C <= 0.0) || (A + B + C >= 1.0) )
     {printf("sector probablities must be greater than 0.0 and sum to 1.0\n"); return 1;}

  RMAT_args_t rmat_args = {seed, scale, A, B, C, D};

  for (int i=0; i<target_edges; ++i)
    RMAT(rmat_args,i);

  // Compute total number of edges
  for (auto& it: edges) {
    num_edges += it.second.size();
  }

//  for (auto& it: edges) {
//    std::cout << it.first;
//    for (auto& it2: it.second)
//      std::cout << " " << it2;
//    std::cout << std::endl;
//  }

  // Set Vertices[i] to start index of vertex i's edges
  std::vector<int64_t> Vertices(num_vertices);
  Vertices[0]=0;
  for (int i=1; i<num_vertices; ++i) {
    Vertices[i] = Vertices[i-1] + edges[i-1].size();
  }

  std::ofstream outfile;
  outfile.open(filename, std::ios::out);
  outfile << num_edges << std::endl;
  for (int i=0; i < num_vertices; ++i) {
    outfile << i << " " << Vertices[i] << std::endl;
  }
  outfile.close();

  return 0;
}
