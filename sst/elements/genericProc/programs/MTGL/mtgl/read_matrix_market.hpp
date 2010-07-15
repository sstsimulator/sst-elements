/*  _________________________________________________________________________
 *
 *  MTGL: The MultiThreaded Graph Library
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top MTGL directory.
 *  _________________________________________________________________________
 */

/****************************************************************************/
/*! \file read_matrix_market.hpp

    \brief Parses a matrix market graph file and creates an MTGL graph.

    \author Karen Devine (kddevin@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 2/26/2008
*/
/****************************************************************************/

#ifndef MTGL_READ_MATRIX_MARKET_HPP
#define MTGL_READ_MATRIX_MARKET_HPP

#include <cstdio>
#include <cstdlib>

#include <mtgl/util.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/merge_sort.hpp>
#include <mtgl/dynamic_array.hpp>

// The XMT implementation of strtod() had horrible performance in
// read_matrix_market() (where horrible means read_matrix_market() and not
// just the bad version of strtod() ran 10x slower), so I added a version
// that didn't suck.
#ifdef __MTA__
  #include <mtgl/strtod.hpp>
#endif

namespace mtgl {

/*! \brief Parses a matrix market graph file and creates an MTGL graph.

    \author Karen Devine (kddevin@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)
*/
template <class graph_adapter, typename WT>
void read_matrix_market(graph_adapter& g, char* filename,
                        dynamic_array<WT>& weights)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

#ifdef DEBUG
  mt_timer timer;
  mt_timer timer2;
  #pragma mta fence
  timer.start();
#endif

  int buflen;
  char* buf = read_array<char>(filename, buflen);

#ifdef DEBUG
  #pragma mta fence
  timer.stop();
  fprintf(stderr, "                 File read time: %10.6f\n",
          timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta fence
  timer2.start();
#endif

  // Find the length of the header line and put a null character at the
  // end of it.  This is necessary because sscanf expects it.
  int pll;
  for (pll = 0; pll < buflen - 1 && buf[pll] != '\n'; ++pll);
  buf[pll] = '\0';

  char mtx[256];
  char crd[256];
  char data_type[256];
  char storage_scheme[256];

  int ret = sscanf(buf, "%%%%MatrixMarket %s %s %s %s", mtx, crd, data_type,
                   storage_scheme);

  // Convert to lower case.
  for (char* p = mtx; *p != '\0'; *p = tolower(*p), ++p);
  for (char* p = crd; *p != '\0'; *p = tolower(*p), ++p);
  for (char* p = data_type; *p != '\0'; *p = tolower(*p), ++p);
  for (char* p = storage_scheme; *p != '\0'; *p = tolower(*p), ++p);

  if (ret != 4)
  {
    fprintf(stderr, "\nError: Header line misformatted.\n");
    exit(1);
  }

  if (strcmp(mtx, "matrix") != 0)
  {
    fprintf(stderr, "\nError: Can only read 'matrix' objects.\n");
    exit(1);
  }

  if (strcmp(crd, "coordinate") != 0)
  {
    fprintf(stderr, "\nError: Can only read 'coordinate' format.\n");
    exit(1);
  }

  if (strcmp(data_type, "real") != 0 &&
      strcmp(data_type, "integer") != 0 &&
      strcmp(data_type, "pattern") != 0)
  {
    fprintf(stderr, "\nError: Can only read 'real', 'integer', or 'pattern' "
            "matrix entries.\n");
    exit(1);
  }

  if (strcmp(storage_scheme, "general") != 0 &&
      strcmp(storage_scheme, "symmetric") != 0 &&
      strcmp(storage_scheme, "skew-symmetric") != 0)
  {
    fprintf(stderr, "\nError: Can only read 'general', 'symmetric', and "
            "'skew-symmetric' symmetry\n       types.\n");
    exit(1);
  }

  if (strcmp(data_type, "pattern") == 0 &&
      strcmp(storage_scheme, "skew-symmetric") == 0)
  {
    fprintf(stderr, "\nError: Can't have a pattern skew-symmetric graph.\n");
    exit(1);
  }

  // Skip comment and empty lines in input (beginning with '%' or '\n').
  size_type num_intro_lines = 1;
  int pls = 0;

  while (buf[pls] == '%' || buf[pls] == '\n')
  {
    while (buf[pls] != '\n') ++pls;    // Skip the line.

    ++num_intro_lines;
    ++pls;                            // Move to next line
  }

  // Read size of sparse matrix.

  // Find the length of the problem line and put a null character at the
  // end of the problem line.  This is necessary because sscanf expects it.
  for (pll = 0; pll < buflen - 1 - pls && buf[pll + pls] != '\n'; ++pll);
  buf[pll + pls] = '\0';

  size_type num_vertices;
  size_type ncol;
  size_type num_entries;

  ret = sscanf(&buf[pls], "%lu %lu %lu", &num_vertices, &ncol, &num_entries);

  if (ret != 3)
  {
    fprintf(stderr, "\nError: Problem line misformatted (expected <num_rows> "
            "<num_cols> <num_nonzeros>).\n");
    exit(1);
  }

  if (num_vertices != ncol)
  {
    fprintf(stderr, "\nError: The matrix must be square %lu x %lu\n",
            num_vertices, ncol);
    exit(1);
  }

  pls += pll + 1;
  ++num_intro_lines;

#ifdef DEBUG
  #pragma mta fence
  timer2.stop();
  fprintf(stderr, "Problem line find and read time: %10.6f\n",
         timer2.getElapsedSeconds());

  #pragma mta fence
  timer2.start();
#endif

  // Find start of each edge line.
  int num_lines = 1;
  int* start_positions = (int*) malloc(sizeof(int) * num_entries);
  start_positions[0] = pls;

  #pragma mta assert nodep
  for (int i = pls; i < buflen - 1; ++i)
  {
    if (buf[i] == '\n' && buf[i+1] != '%')
    {
      int pos = mt_incr(num_lines, 1);
      start_positions[pos] = i + 1;
    }
  }

  if (num_lines != num_entries)
  {
    fprintf(stderr, "\nERROR: Number of edges in file doesn't match number "
            "from problem description\n       line.\n");
    exit(1);
  }

#ifdef DEBUG
  #pragma mta fence
  timer2.stop();
  fprintf(stderr, "      Start positions find time: %10.6f\n",
          timer2.getElapsedSeconds());
#endif

#ifdef __MTA__
#ifdef DEBUG
  #pragma mta fence
  timer2.start();
#endif

  merge_sort(start_positions, num_entries);

#ifdef DEBUG
  #pragma mta fence
  timer2.stop();
  fprintf(stderr, "      Start positions sort time: %10.6f\n",
          timer2.getElapsedSeconds());
#endif
#endif

#ifdef DEBUG
  #pragma mta fence
  timer2.start();
#endif

  // Allocate storage for the sources, destinations, and weights.
  size_type* edge_heads;
  size_type* edge_tails;

  if (strcmp(storage_scheme, "general") == 0)
  {
    edge_heads = (size_type*) malloc(sizeof(size_type) * num_entries);
    edge_tails = (size_type*) malloc(sizeof(size_type) * num_entries);

    if (strcmp(data_type, "pattern") != 0) weights.resize(num_entries);
  }
  else
  {
    // Skew-symmetric type has exactly num_entries * 2 edges.  Symmetric
    // type has at most num_entries * 2 edges.
    edge_heads = (size_type*) malloc(sizeof(size_type) * num_entries * 2);
    edge_tails = (size_type*) malloc(sizeof(size_type) * num_entries * 2);

    if (strcmp(data_type, "pattern") != 0) weights.resize(num_entries * 2);
  }

  size_type num_edges = num_entries;
  if (strcmp(storage_scheme, "skew-symmetric") == 0)
  {
    num_edges = 2 * num_entries;
  }

  // Read edges from matrix-market file.

  if (strcmp(storage_scheme, "general") == 0)
  {
    if (strcmp(data_type, "real") == 0)
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        double weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtod(a, &b);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        edge_heads[i] = static_cast<size_type>(from);
        edge_tails[i] = static_cast<size_type>(to);
        weights[i] = static_cast<WT>(weight);
      }
    }
    else if (strcmp(data_type, "integer") == 0)
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        long weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtol(a, &b, 10);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        edge_heads[i] = static_cast<size_type>(from);
        edge_tails[i] = static_cast<size_type>(to);
        weights[i] = static_cast<WT>(weight);
      }
    }
    else
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        edge_heads[i] = static_cast<size_type>(from);
        edge_tails[i] = static_cast<size_type>(to);
      }
    }
  }
  else if (strcmp(storage_scheme, "skew-symmetric") == 0)
  {
    if (strcmp(data_type, "real") == 0)
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        double weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtod(a, &b);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (from == to)
        {
          fprintf(stderr, "\nError on line %lu: Can't have diagonal entry in "
                  "skew-symmetric matrix.\n", i + num_intro_lines + 1);
          exit(1);
        }

        edge_heads[2 * i] = static_cast<size_type>(from);
        edge_tails[2 * i] = static_cast<size_type>(to);
        weights[2 * i] = static_cast<WT>(weight);

        edge_heads[2 * i + 1] = static_cast<size_type>(to);
        edge_tails[2 * i + 1] = static_cast<size_type>(from);
        weights[2 * i + 1] = static_cast<WT>(-weight);
      }
    }
    else if (strcmp(data_type, "integer") == 0)
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        long weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtol(a, &b, 10);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (from == to)
        {
          fprintf(stderr, "\nError on line %lu: Can't have diagonal entry in "
                  "skew-symmetric matrix.\n", i + num_intro_lines + 1);
          exit(1);
        }

        edge_heads[2 * i] = static_cast<size_type>(from);
        edge_tails[2 * i] = static_cast<size_type>(to);
        weights[2 * i] = static_cast<WT>(weight);

        edge_heads[2 * i + 1] = static_cast<size_type>(to);
        edge_tails[2 * i + 1] = static_cast<size_type>(from);
        weights[2 * i + 1] = static_cast<WT>(-weight);
      }
    }
  }
  else
  {
    if (strcmp(data_type, "real") == 0)
    {
      int num_diags = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        double weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtod(a, &b);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        if (to == from)
        {
          int pos = mt_incr(num_diags, 1);
          edge_heads[pos] = static_cast<size_type>(from);
          edge_tails[pos] = static_cast<size_type>(to);
          weights[pos] = static_cast<WT>(weight);
        }
      }

      int num_offdiag = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        double weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtod(a, &b);

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (to != from)
        {
          int pos = mt_incr(num_offdiag, 1);
          edge_heads[num_diags + 2 * pos] = static_cast<size_type>(from);
          edge_tails[num_diags + 2 * pos] = static_cast<size_type>(to);
          weights[num_diags + 2 * pos] = static_cast<WT>(weight);
          edge_heads[num_diags + 2 * pos + 1] = static_cast<size_type>(to);
          edge_tails[num_diags + 2 * pos + 1] = static_cast<size_type>(from);
          weights[num_diags + 2 * pos + 1] = static_cast<WT>(weight);
        }
      }

      num_edges = num_diags + (num_entries - num_diags) * 2;
    }
    else if (strcmp(data_type, "integer") == 0)
    {
      int num_diags = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        long weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtol(a, &b, 10);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        if (to == from)
        {
          int pos = mt_incr(num_diags, 1);
          edge_heads[pos] = static_cast<size_type>(from);
          edge_tails[pos] = static_cast<size_type>(to);
          weights[pos] = static_cast<WT>(weight);
        }
      }

      int num_offdiag = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;
        long weight;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);
        weight = strtol(a, &b, 10);

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (to != from)
        {
          int pos = mt_incr(num_offdiag, 1);
          edge_heads[num_diags + 2 * pos] = static_cast<size_type>(from);
          edge_tails[num_diags + 2 * pos] = static_cast<size_type>(to);
          weights[num_diags + 2 * pos] = static_cast<WT>(weight);
          edge_heads[num_diags + 2 * pos + 1] = static_cast<size_type>(to);
          edge_tails[num_diags + 2 * pos + 1] = static_cast<size_type>(from);
          weights[num_diags + 2 * pos + 1] = static_cast<WT>(weight);
        }
      }

      num_edges = num_diags + (num_entries - num_diags) * 2;
    }
    else
    {
      int num_diags = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);

        if (a == b)
        {
          fprintf(stderr, "\nError on line %lu: Too few parameters when "
                  "describing edge.\n", i + num_intro_lines + 1);
          exit(1);
        }

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (from < 0 || static_cast<size_type>(from) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: First vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }
        else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
        {
          fprintf(stderr, "\nError on line %lu: Second vertex id is invalid.\n",
                  i + num_intro_lines + 1);
          exit(1);
        }

        if (to == from)
        {
          int pos = mt_incr(num_diags, 1);
          edge_heads[pos] = static_cast<size_type>(from);
          edge_tails[pos] = static_cast<size_type>(to);
        }
      }

      int num_offdiag = 0;

      #pragma mta assert parallel
      for (size_type i = 0; i < num_entries; ++i)
      {
        long from;
        long to;

        char* a = &buf[start_positions[i]];
        char* b = NULL;

        from = strtol(a, &b, 10);
        to = strtol(b, &a, 10);

        // Matrix Market vertex ids are 1-based.  We need them to be 0-based.
        --from;
        --to;

        if (to != from)
        {
          int pos = mt_incr(num_offdiag, 1);
          edge_heads[num_diags + 2 * pos] = static_cast<size_type>(from);
          edge_tails[num_diags + 2 * pos] = static_cast<size_type>(to);
          edge_heads[num_diags + 2 * pos + 1] = static_cast<size_type>(to);
          edge_tails[num_diags + 2 * pos + 1] = static_cast<size_type>(from);
        }
      }

      num_edges = num_diags + (num_entries - num_diags) * 2;
    }
  }

  if (strcmp(storage_scheme, "symmetric") == 0 &&
      strcmp(data_type, "pattern") != 0)
  {
    weights.resize(num_edges);
  }

#ifdef DEBUG
  #pragma mta fence
  timer2.stop();
  fprintf(stderr, "                 Edge read time: %10.6f\n",
          timer2.getElapsedSeconds());

  #pragma mta fence
  timer.stop();
  fprintf(stderr, "                File parse time: %10.6f\n",
          timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
#endif

  init(num_vertices, num_edges, edge_heads, edge_tails, g);

#ifdef DEBUG
  #pragma mta fence
  timer.stop();
  fprintf(stderr, "            Graph creation time: %10.6f\n",
          timer.getElapsedSeconds());
#endif

  free(start_positions);
  free(edge_heads);
  free(edge_tails);
  free(buf);
}

template <class graph_adapter, typename WT>
void read_matrix_market(graph_adapter& g, const char* filename,
                        dynamic_array<WT>& weights)
{
  return read_matrix_market(g, const_cast<char*>(filename), weights);
}

}

#endif
