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

// This code is licensed under the Common Public License
// It originated from COIN-OR
//
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This is an implementation of the Volume algorithm for uncapacitated
// facility location problems. See the references
// F. Barahona, R. Anbil, "The Volume algorithm: producing primal solutions
// with a subgradient method," IBM report RC 21103, 1998.
// F. Barahona, F. Chudak, "Solving large scale uncapacitated facility
// location problems," IBM report RC 21515, 1999.

// Significant modifications and additions are Copyright (C) 2006, 2007,
// Sandia Corporation.
//
// Sparse version by Erik Boman and Jon Berry, Oct 2006
// p-median option added by Erik Boman, Oct 2006.
// Optimized sparse matrix handling by Jon Berry.
// Side constraints by Cindy Phillips and Erik Boman, April 2007.
// MTA compatibility by Jon Berry, Fall, 2007.

#include <cstdio>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>
#include <climits>
#include <iostream>

#ifndef WIN32
#include <sys/times.h>
#endif

#include <mtgl/ufl.h>

using namespace std;

int newDummyID;
map<int, int> newID;
map<int, int>::iterator newIDiter;
vector<int> origID;
double gap;     // e.g. 0.1 --> stop when within 10% of optimal

//****** UFL_parms
// reading parameters specific to facility location
UFL_parms::UFL_parms(const char* filename) : fdata(""), h_iter(10)
{
  char s[500];
  FILE* file = fopen(filename, "r");
  if (!file)
  {
    printf("Failure to open ufl datafile: %s\n ", filename);
    abort();
  }

  while (fgets(s, 500, file))
  {
    const int len = strlen(s) - 1;
    if (s[len] == '\n') s[len] = 0;
    std::string ss;
    ss = s;

    if (ss.find("fdata") == 0)
    {
      int j = ss.find("=");
      int j1 = ss.length() - j + 1;
      fdata = ss.substr(j + 1, j1);

    }
    else if (ss.find("dualfile") == 0)
    {
      int j = ss.find("=");
      int j1 = ss.length() - j + 1;
      dualfile = ss.substr(j + 1, j1);

    }
    else if (ss.find("dual_savefile") == 0)
    {
      int j = ss.find("=");
      int j1 = ss.length() - j + 1;
      dual_savefile = ss.substr(j + 1, j1);

    }
    else if (ss.find("int_savefile") == 0)
    {
      int j = ss.find("=");
      int j1 = ss.length() - j + 1;
      int_savefile = ss.substr(j + 1, j1);

    }
    else if (ss.find("h_iter") == 0)
    {
      int i = ss.find("=");
      h_iter = atoi(&s[i + 1]);
    }
  }

  fclose(file);
}

/**
 * This function returns a pseudo-random number for each invocation.
 * This C implementation, is adapted from a FORTRAN 77 adaptation of
 * the "Integer Version 2" minimal standard number generator whose
 * Pascal code described by
 * \if GeneratingLaTeX Park and Miller~\cite{ParMil88}. \endif
 * \if GeneratingHTML [\ref ParMil88 "ParMil88"]. \endif
 *
 * This code is portable to any machine that has a
 * maximum integer greater than, or equal to, 2**31-1.  Thus, this code
 * should run on any 32-bit machine.  The code meets all of the
 * requirements of the "minimal standard" as described by
 * \if GeneratingLaTeX Park and Miller~\cite{ParMil88}. \endif
 * \if GeneratingHTML [\ref ParMil88 "ParMil88"]. \endif
 * Park and Miller note that the execution times for running the portable
 * code were 2 to 50 times slower than the random number generator supplied
 * by the vendor.
 */

struct triple_struct {
  int loc, cust;
  double cost;
};

int cmp_triples(const void* p1, const void* p2)
{
  struct triple_struct* tp1 = (struct triple_struct*) p1;
  struct triple_struct* tp2 = (struct triple_struct*) p2;
  if (tp1->loc < tp2->loc) return -1;
  else if (tp1->loc > tp2->loc)
    return 1;
  else
    return tp1->cust - tp2->cust;
}

/* Preprocessing: Write to a temporary file to save memory.
   TODO: Make this feature optional. */
char* UFL_preprocess_data(const char* fname, int p)
{

  FILE* file = fopen(fname, "r");
  if (!file)
  {
    printf("Failure to open ufl datafile: %s\n ", fname);
    abort();
  }

  int nloc, ncust, nnz;
  double cost;
  double* fcost;
  char s[500];

  if (!fgets(s, 500, file))
  {
    printf("ERROR: UFL_preprocess_data: fgets failed\n");
  }

  int len = strlen(s) - 1;
  if (s[len] == '\n') s[len] = 0;
  // read number of locations, number of customers, no. of nonzeros
  sscanf(s, "%d%d%d", &nloc, &ncust, &nnz);

  if (p == 0)  // UFL
  {  // read location costs
    fcost = (double*) malloc(sizeof(double) * nloc);

    for (int i = 0; i < nloc; ++i)
    {
      if (!fgets(s, 500, file))
      {
        printf("ERROR: UFL_preprocess_data: fgets failed\n");
      }

      len = strlen(s) - 1;

      if (s[len] == '\n') s[len] = 0;

      sscanf(s, "%lf", &cost);
      fcost[i] = cost;
    }
  }
  struct triple_struct* triples =
    (struct triple_struct*) malloc(nnz * sizeof(struct triple_struct));

  int count = 0, loc, cust;

  for (int i = 0; i < nnz; i++)
  {
    if (!fgets(s, 500, file))
    {
      printf("ERROR: UFL_preprocess_data: fgets failed\n");
    }

    len = strlen(s) - 1;

    if (s[len] == '\n') s[len] = 0;

    int k = sscanf(s, "%d%d%lf", &loc, &cust, &cost);
    assert(k == 3);
    triples[count].loc = loc;
    triples[count].cust = cust;
    triples[count].cost = cost;
    count++;
  }

  assert(count == nnz);
  qsort(triples, nnz, sizeof(triple_struct), cmp_triples);
  char* tmpfname = (char*) malloc(strlen(fname) + 5);
  strcpy(tmpfname, fname);
  strcat(tmpfname, ".tmp");
  FILE* tfile = fopen(tmpfname, "w");

  if (!tfile)
  {
    printf("Failure to open tmp ufl datafile: %s\n ", tmpfname);
    abort();
  }

  fprintf(tfile, "%d %d %d\n", nloc, ncust, nnz);

  if (p == 0)  // UFL
  {
    fcost = (double*) malloc(sizeof(double) * nloc);

    for (int i = 0; i < nloc; ++i)
    {
      fprintf(tfile, "%lf\n", fcost[i]);
    }

    free(fcost);
  }

  for (int i = 0; i < nnz; i++)
  {
    fprintf(tfile, "%d %d %lf\n", triples[i].loc, triples[i].cust,
            triples[i].cost);
  }

  fclose(tfile);

  return tmpfname;
}

//############################################################################

//###### USER HOOKS
// compute reduced costs
/*
   int
   UFL::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
   {
   int i,j,k=0;
   for ( i=0; i < nloc; i++){
     rc[i]=fcost[i];
     // for (j = 0; j < ncust; ++j) {
     std::map<int,double>::iterator col_it = _dist[i].begin();
     for (; col_it!=_dist[i].end(); col_it++) {
       std::pair<int,double> p = *col_it;
       int j = p.first;
       double distij = p.second;
       //rc[nloc+k]= dist[k] - u[j];
       rc[nloc+k]= distij - u[j];
 ++k;
     }
   }
   return 0;
   }
 */
