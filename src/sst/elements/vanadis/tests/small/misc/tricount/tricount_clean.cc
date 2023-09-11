// Written by Courtenay T. Vaughan - cleaned of extras - version 7/8/23
#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>

int main(int argc, char *argv[]) {
  uint64_t i, j;
  uint64_t tmp, src, dst, tct;
  int k, l, a, at, al, b, bt, bl;

  if (argc != 2) {printf("Usage: <fname>\n"); return 1;}

  std::string filename(argv[1]);
  std::ifstream ifile(filename, std::ios::in | std::ios::binary);
  uint64_t num_indices;
  ifile >> num_indices;

  uint64_t *index = new uint64_t[num_indices];
  for (i = 0; i < num_indices; i++) {
    ifile >> index[i];
  }

  uint64_t edge;
  ifile >> edge;

  uint64_t *edges = new uint64_t[edge];
  for (i = 0; i < edge; i++) {
    ifile >> edges[i];
  }
  ifile.close();

  uint64_t num_vertices = num_indices - 1;
  printf("num_verticies %d\n", num_vertices);
  printf("num_edges %d\n",  edge);

  // Count number of triangles
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
          // printf("triangle at: (i, j, k)=(%d,%d,%d)\n", i, edges[j], edges[a]);
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
