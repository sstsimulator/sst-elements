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
/*! \file read_cluster_graph.hpp

    \brief Functions to parse a DIMACS graph file and create an MTGL graph.

    \author Joe Crobak

    \date 5/20/2006
*/
/****************************************************************************/

#ifndef MTGL_READ_CLUSTER_GRAPH_HPP
#define MTGL_READ_CLUSTER_GRAPH_HPP

#include <cstdio>
#include <cstring>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mtgl/graph.hpp>
#include <mtgl/randfill.h>
#include <mtgl/util.hpp>

namespace mtgl {

template <class graph>
class gen_cluster_graph {
public:
  gen_cluster_graph(char* fn_, int*& weights_, int& source_,
                    char*& problemDesc_) :
    fn(fn_), weights(weights_), source(source_), problemDesc(problemDesc_) {}

  void input_error(char* error_msg, int line)
  {
    fprintf(stderr, "line %i: %s\n", line, error_msg);
    exit(1);
  }

  graph* run()
  {
    int nodes, edges;

    FILE* file;
    struct stat info;
    problemDesc = NULL;
    /* open file */
    printf("ReadDIMACSgraph: fn: %s\n", fn);
    if ((file = fopen(fn, "r")) == NULL)
    {
      printf("Cannot open file %s", fn);
      exit(1);
    }
    /*get size of file */
    stat(fn, &info);
    int numChars = info.st_size;
    printf("ReadDIMACSgraph: numChars: %d\n", numChars);
    char* buf = (char*) malloc((numChars + 1) * sizeof(char));

    /* read file */
    fread(buf, sizeof(char), numChars, file);

    // find number of newlines
    int numLines = 0;
    for (int i = 0; i < numChars; i++)
    {
      if (buf[i] == '\n') numLines++;
    }
    int* starts = (int*) malloc(sizeof(int) * (numLines + 1));
    starts[0] = 0;
    int x = 1;
    // find start of each line
    for (int i = 0; i < numChars; i++)
    {
      if (buf[i] == '\n') starts[x++] = i + 1;
    }
    starts[numLines] = numChars;

    // look for the line with the problem size
    int lineNum = 0;
    while (1)
    {
      if (buf[starts[lineNum]] == 'p') break;
      lineNum++;
    }

    int retVal;
    // line looks like p sp <vertices> <edges>
    if (2 != (retVal = sscanf(buf + starts[lineNum], "%*c %*2s %d %d",
                              &nodes, &edges)))
    {
      input_error("number of vertices and/or edges not supplied", lineNum);
    }

    // edges /= 2;              Commented out JWB 12/2006
    //printf ("%i vertices %i edges\n",nodes,edges);
    graph* g = new graph(UNDIRECTED);
    int* vertices = new int[nodes];
    int* heads = (int*) malloc(sizeof(int) * edges);
    int* tails = (int*) malloc(sizeof(int) * edges);
    weights = (int*) malloc(sizeof(int) * edges);
    source = -1;
    int edgesSeen = 0;
    int arcStart = -1;

    for (int i = 0; i < nodes; i++) vertices[i] = i;

    for (int i = lineNum + 1; i < numLines; i++)
    {
      //for (int j= starts[i]; j< starts[i+1]; j++)
      //    printf("%c",buf[j]);
      switch (buf[ starts[i] ])
      {
        // skip empty lines
        case 'c':
        case '\n':
        case '\0':
          break;

        // problem desciption
        case 't':
          problemDesc = (char*) malloc(sizeof(char) *
                                       (starts[i + 1] - starts[i]));
          strncpy(problemDesc, buf + starts[i] + 2,
                  starts[i + 1] - starts[i] - 3);
          break;

        // source description
        case 'n':
          if (source != -1)
          {
            input_error("multiple source vertices specified", i);
          }
          if (1 != sscanf(buf + starts[i], "%*c %d", &source))
          {
            input_error("source vertex id not supplied", i);
          }
          if (source < 1 || source > nodes)
          {
            input_error("invalid source vertex id", i);
          }
          //printf("source = %i\n",source);
          break;

        // edge (arc) description
        case 'a':
          arcStart = i;
          break;
      }

      if (arcStart != -1) break;
    }

    int num = (numLines - arcStart); // / 2;  Commented out JWB 12/2006

    #pragma mta assert parallel
    for (int i = 0; i < num; i++)
    {
      int i2 = i; //  *2; Commented out JWB 12/2006
      int buf_offset = starts[arcStart + i2];
      int to, from, weight;

      if (3 != sscanf(buf + buf_offset, "%*c %d %d %d", &from, &to, &weight))
      {
        input_error("too few parameters when describing arc", arcStart + i2);
      }

      --from;
      --to;

      if (from < 0 || from >= nodes)
      {
        input_error("first vertex id is invalid", arcStart + i2);
      }
      if (to < 0 || to >= nodes)
      {
        input_error("second vertex id is invalid", arcStart + i2);
      }
      if (weight < 0)
      {
        input_error("negative weight supplied", arcStart + i2);
      }

      //printf("edge (%i,%i), %i\n",from,to,weight);
      int index = i;
      heads[index]   = vertices[from];
      tails[index]   = vertices[to];
      weights[index] = weight;
    }

    init(nodes, edges, heads, tails, g);

/*
 #ifndef __MTA__
    munmap(buf,info.st_size);
 #endif
 */

    free(heads);
    free(tails);
    free(starts);
    free(buf);

    return g;
  }

private:
  char* fn;
  int*& weights;
  int& source;
  char*& problemDesc;
};

class read_cluster_graph {
public:
  read_cluster_graph(char* fn_, int*& weights_, int& source_,
                     char*& problemDesc_) :
    fn(fn_), weights(weights_), source(source_), problemDesc(problemDesc_) {}

  void input_error(char* error_msg, int line)
  {
    fprintf(stderr, "line %i: %s\n", line, error_msg);
    exit(1);
  }

  graph* run()
  {
    int nodes, edges;

    FILE* file;
    struct stat info;
    problemDesc = NULL;
    /* open file */
    printf("ReadDIMACSgraph: fn: %s\n", fn);
    if ((file = fopen(fn, "r")) == NULL)
    {
      printf("Cannot open file %s", fn);
      exit(1);
    }
    /*get size of file */
    stat(fn, &info);
    int numChars = info.st_size;
    printf("ReadDIMACSgraph: numChars: %d\n", numChars);
    char* buf = (char*) malloc((numChars + 1) * sizeof(char));

    /* read file */
    fread(buf, sizeof(char), numChars, file);

    // find number of newlines
    int numLines = 0;
    for (int i = 0; i < numChars; i++)
    {
      if (buf[i] == '\n') numLines++;
    }
    int* starts = (int*) malloc(sizeof(int) * (numLines + 1));
    starts[0] = 0;
    int x = 1;
    // find start of each line
    for (int i = 0; i < numChars; i++)
    {
      if (buf[i] == '\n') starts[x++] = i + 1;
    }
    starts[numLines] = numChars;

    // look for the line with the problem size
    int lineNum = 0;
    while (1)
    {
      if (buf[ starts[lineNum] ] == 'p') break;
      lineNum++;
    }

    int retVal;
    // line looks like p sp <vertices> <edges>
    if (2 != (retVal = sscanf(buf + starts[lineNum], "%*c %*2s %d %d",
                              &nodes, &edges)))
    {
      input_error("number of vertices and/or edges not supplied", lineNum);
    }

    // edges /= 2; Commented out JWB 12/2006
    //printf ("%i vertices %i edges\n",nodes,edges);
    graph* g = new graph();
    vertex** vertices = g->addVertices(0, nodes - 1);
    vertex** heads = (vertex**) malloc(sizeof(vertex*) * edges);
    vertex** tails = (vertex**) malloc(sizeof(vertex*) * edges);
    weights = (int*) malloc(sizeof(int) * edges);
    source = -1;
    int edgesSeen = 0;
    int arcStart = -1;
    for (int i = lineNum + 1; i < numLines; i++)
    {

      //for (int j= starts[i]; j< starts[i+1]; j++)
      //    printf("%c",buf[j]);
      switch (buf[ starts[i] ])
      {
        // skip empty lines
        case 'c':
        case '\n':
        case '\0':
          break;

        // problem desciption
        case 't':
          problemDesc = (char*) malloc(sizeof(char) *
                                       (starts[i + 1] - starts[i]));
          strncpy(problemDesc, buf + starts[i] + 2,
                  starts[i + 1] - starts[i] - 3);
          break;

        // source description
        case 'n':
          if (source != -1)
          {
            input_error("multiple source vertices specified", i);
          }
          if (1 != sscanf(buf + starts[i], "%*c %d", &source))
          {
            input_error("source vertex id not supplied", i);
          }
          if (source < 1 || source > nodes)
          {
            input_error("invalid source vertex id", i);
          }
          //printf("source = %i\n",source);
          break;

        // edge (arc) description
        case 'a':
          arcStart = i;
          break;
      }

      if (arcStart != -1) break;
    }

    int num = (numLines - arcStart); // / 2;  Commented out JWB 12/2006

    #pragma mta assert parallel
    for (int i = 0; i < num; i++)
    {
      int i2 = i; //  *2; Commented out JWB 12/2006
      int buf_offset = starts[arcStart + i2];
      int to, from, weight;

      if (3 != sscanf(buf + buf_offset, "%*c %d %d %d", &from, &to, &weight))
      {
        input_error("too few parameters when describing arc", arcStart + i2);
      }

      --from;
      --to;

      if (from < 0 || from >= nodes)
      {
        input_error("first vertex id is invalid", arcStart + i2);
      }
      if (to < 0 || to >= nodes)
      {
        input_error("second vertex id is invalid", arcStart + i2);
      }
      if (weight < 0)
      {
        input_error("negative weight supplied", arcStart + i2);
      }

      //printf("edge (%i,%i), %i\n",from,to,weight);
      int index = i;
      heads[index]   = vertices[from];
      tails[index]   = vertices[to];
      weights[index] = weight;
    }

    g->addedges(heads, tails, num, true);

/*
 #ifndef __MTA__
    munmap(buf,info.st_size);
 #endif
 */

    free(heads);
    free(tails);
    free(starts);
    free(buf);

    return g;
  }

private:
  char* fn;
  int*& weights;
  int& source;
  char*& problemDesc;
};

}

#endif
