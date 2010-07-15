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
/*! \file array2d.hpp

    \brief Some simple array classes used in s-t searches.

    \author William McLendon (wcmclen@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_ARRAY2D_HPP
#define MTGL_ARRAY2D_HPP

#include <cstdio>
#include <cstdlib>
#include <cassert>

namespace mtgl {

/// Implements a simple 1-dimensional array class.
template <typename T>
class array1d {
  friend void print_array1d(array1d<int>& a, const char* tag);
  friend void print_array1d(array1d<double>& a, const char* tag);
  friend void print_array1d_masked(array1d<double>& a,
                                   const char* tag,
                                   double mask);
  friend void print_array1d_masked(array1d<int>& a, const char* tag, int mask);

public:
  array1d(int _asize) : asize(_asize)
  {
#ifdef DEBUG
    assert(asize > 0);
#endif

    data = (T*) malloc(sizeof(T) * asize);

#ifdef DEBUG
    assert(data != NULL);
#endif
  }

  ~array1d()
  {
    if (data)
    {
      asize = 0;
      free(data);
    }
  }

  void init(T value)
  {
    #pragma mta assert nodep
    for (int i = 0; i < asize; i++) data[i] = value;
  }

  int size(void) { return(asize); }
  T* ptr(void) { return(data); }

  T& operator[](int index)
  {
#ifdef DEBUG
    assert(index < asize);
#endif

    return(data[index]);
  }

private:
  T* data;
  int asize;
};


void print_array1d(array1d<int>& a, const char* tag = NULL)
{
  if (tag) printf("%s ", tag);
  printf("[");
  for (int i = 0; i < a.size(); i++)
  {
    printf(" %5d", a.data[i]);
  }
  printf("  ]\n");
}


void print_array1d(array1d<double>& a, const char* tag = NULL)
{
  if (tag) printf("%s ", tag);
  printf("[");
  for (int i = 0; i < a.size(); i++)
  {
    printf(" %5.3f", a.data[i]);
  }
  printf("  ]\n");
}

void print_array1d_masked(array1d<int>& a, const char* tag = NULL, int mask = 0)
{
  if (tag) printf("%s ", tag);
  printf("[");
  for (int i = 0; i < a.size(); i++)
  {
    if (a.data[i] == mask) printf(" %5s", "-");
    else printf(" %5d", a.data[i]);
  }
  printf(" ]\n");
}

void print_array1d_masked(array1d<double>& a, const char* tag = NULL,
                          double mask = 0.0)
{
  if (tag) printf("%s ", tag);
  printf("[");
  for (int i = 0; i < a.size(); i++)
  {
    if (a.data[i] == mask) printf(" %5s", "-");
    else printf(" %5.3f", a.data[i]);
  }
  printf(" ]\n");
}


/// \brief Implements a simple 2-dimensional array class by creating
///        an array of pointers to array1d objects.
template <typename T>
class array2d {
public:

  array2d(int _dim1, int _dim2) : dim1(_dim1), dim2(_dim2)
  {
    data = (array1d<T>**)malloc(sizeof(array1d<T>*) * dim1);

    #pragma mta assert nodep
    for (int i = 0; i < dim1; i++) data[i] = new array1d<T>(dim2);
  }

  ~array2d()
  {
    #pragma mta assert nodep
    for (int i = 0; i < dim1; i++) delete data[i];

    free(data);
    dim1 = -1;
    dim2 = -1;
  }

  void init(T value)
  {
    #pragma mta assert nodep
    for (int i = 0; i < dim1; i++)
    {
      for (int j = 0; j < dim2; j++) (*data[i])[j] = value;
    }
  }

  array1d<T>& operator[](int index)
  {
#ifdef DEBUG
    assert(index < dim1);
#endif

    return(*data[index]);
  }

  void print(const char* tag = NULL)
  {
    if (tag) printf("%s:\n", tag);

    for (int i = 0; i < dim1; i++)
    {
      char s[65];
      sprintf(s, "%5d", i);
      print_array1d(*data[i], s);
    }
  }

  void print_masked(const char* tag = NULL, T mask = 0)
  {
    if (tag) printf("%s:\n", tag);

    for (int i = 0; i < dim1; i++)
    {
      char s[65];
      sprintf(s, "%5d", i);
      print_array1d_masked(*data[i], s, mask);
    }
  }

  void print_ordered_masked(int* row_order, const char* tag = NULL, T mask = 0)
  {
    if (tag) printf("%s:\n", tag);

    for (int i = 0; i < dim1; i++)
    {
      int j = row_order[i];
      char s[65];
      sprintf(s, "%5d", j);
      print_array1d_masked(*data[j], s, mask);
    }
  }

private:
  array1d<T>** data;
  int dim1, dim2;
};

}

#endif
