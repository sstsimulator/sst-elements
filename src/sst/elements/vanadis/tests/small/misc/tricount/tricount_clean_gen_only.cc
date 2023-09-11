// Written by Courtenay T. Vaughan - cleaned of extras - version 7/8/23
#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>

int main(int argc, char *argv[]) {
  uint64_t i, j;
  uint64_t tmp, src, dst, edge = 0, tct;
  int k, l, a, at, al, b, bt, bl;

  if (argc != 8) {printf("Usage: <seed> <scale> <edge ratio> <A> <B> <C> <fname>\n"); return 1;}

  uint64_t seed = std::stoll(argv[1]);
  uint64_t scale = std::stoll(argv[2]);
  uint64_t num_vertices = 1 << std::stoll(argv[2]);
  uint64_t num_edges = num_vertices * std::stoll(argv[3]);

  double A = std::stod(argv[4]) / 100.0;
  double B = std::stod(argv[5]) / 100.0;
  double C = std::stod(argv[6]) / 100.0;
  double D = 1.0 - A - B - C;

  std::string filename(argv[7]);

  std::mt19937_64 gen64(seed + num_edges);
  uint64_t max_rand = gen64.max();

  uint64_t *index = new uint64_t[num_vertices+1];
  std::fill(index, index + num_vertices + 1, 0);

  uint64_t *edges = new uint64_t[num_edges];
  for (i = 0; i < num_edges; i++) {

    // Generate new edge src and dst ids
    src = dst = 0;
    for (j = 0; j < scale; j++) {
      double rand = (double) gen64() / (double) max_rand;

      src <<= 1;
      if (rand > A + B) { src += 1; }

      dst <<= 1;
      if (rand > A && rand <= A + B || rand > A + B + C) { dst += 1; }
    }

    if (src == dst) continue;  // do not include self edges

    // Edge inclusion
    // Order edge definition
    if (src > dst) {
      tmp = dst; dst = src; src = tmp;
    }

    // Determine whether edge list update is needed
    // index[src] - index[src+1] is the edge index range for the src vertex
    tmp = 1;
    for (k = index[src]; k < index[src+1]; k++) {
      if (edges[k] >= dst) {
        if (edges[k] == dst) {
          tmp = 0;
        }
        break;
      }
    }

    // Insert the new edge into the list and update structures
    // Find location for insertion (dsts are ordered by vertex id)
    if (tmp) {
      if (index[num_vertices] > 0)
        for (l = index[num_vertices]; l >= k; l--)
          edges[l+1] = edges[l];
      edges[k] = dst;
      for (k = num_vertices; k > src; k--)
        index[k]++;
      edge++;
    }
  }

  printf("num_verticies %d\n", num_vertices);
  printf("num_edges %d\n",  edge);

  std::ofstream ofile(filename, std::ios::out | std::ios::binary);
  ofile << num_vertices + 1 << std::endl;
  for (i = 0; i < num_vertices + 1; i++) {
    ofile << index[i] << " ";
  }
  ofile << std::endl;

  ofile << edge << std::endl;
  for (i = 0; i < edge; i++) {
    ofile << edges[i] << " ";
  }
  ofile << std::endl;
  ofile.close();

  delete[] index;
  delete[] edges;
}
