// Written by Courtenay T. Vaughan - cleaned of extras - version 7/8/23
#include <iostream>
#include <random>
#include <algorithm>

int main(int argc, char *argv[]) {
  uint64_t i, j;
  uint64_t tmp, src, dst, edge = 0, tct;
  int k, l, a, at, al, b, bt, bl;

  if (argc != 7) {printf("Usage: <seed> <scale> <edge ratio> <A> <B> <C>\n"); return 1;}

  uint64_t seed = std::stoll(argv[1]);
  uint64_t scale = std::stoll(argv[2]);
  uint64_t num_vertices = 1 << std::stoll(argv[2]);
  uint64_t num_edges = num_vertices * std::stoll(argv[3]);

  double A = std::stod(argv[4]) / 100.0;
  double B = std::stod(argv[5]) / 100.0;
  double C = std::stod(argv[6]) / 100.0;
  double D = 1.0 - A - B - C;

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

  // Graph generation is above - below is the counting and depends on having
  // the list of edges (edges), the index into the edges (index), and the
  // number of vertices in the graph

  tct = 0;
  for (i = 0; i < num_vertices; i++) {
    // going over all of the vertices: i is the vertex number
    at = index[i];
    al = index[i+1];

    if (al > edge) {
      printf("WARNING: Detected malformed input: Index out of edge range\n");
      printf("         Vertex: %u, Edges: %u Index range: [%u,%u)\n",
              i, edge, at, al);
    }

    // at is the pointer into the edges array for i
    for (j = at; j < al && j < edge; j++) {
      // Begin TASK
      // j is the pointer in the edges array for b, which is second vertex
      bt = index[edges[j]]; bl = index[edges[j]+1];

      // now search through a edges for match with b edges, a triangle
      a = j + 1; // Start search at next possible third vertex
      b = bt;
      while(a < al && b < bl) {
        if (edges[a] < edges[b]) {
          a++;
        } else if (edges[a] > edges[b]) {
          b++;
        } else {
         //  printf("triangle at: (i, j, k)=(%d,%d,%d)\n", i, edges[j], edges[a]);
          a++; b++; tct++;
        }
      }
      // End TASK
    }
  }
  printf("total triangles %ld\n", tct);
  delete[] index;
  delete[] edges;
}
