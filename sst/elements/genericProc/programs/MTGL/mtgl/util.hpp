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
/*! \file util.hpp

    \brief Common definitions for the mtgl namespace.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_UTIL_HPP
#define MTGL_UTIL_HPP

#include <cstdio>
#include <cassert>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <algorithm>  // For min and max.
#include <map>
#include <iostream>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#elif defined(WIN32)
#include <ctime>
#else
#include <ctime>
#include <sys/time.h>
#include <cstring>
#endif

#ifdef HAVE_CONFIG_H
#include <mtgl/mtgl_config.h>
#endif

#include <mtgl/snap_util.h>
#include <mtgl/graph_traits.hpp>

#ifdef USING_QTHREADS
#include <qthread/qthread.h>
#endif

// Disable the warning about unknown pragmas.
#ifndef __MTA__
#pragma warning ( disable : 4068 )
#endif

#define NO_ONTOLOGY false
#define USE_ONTOLOGY true

#if defined(sun) || defined(__sun)
typedef uint64_t u_int64_t;
#endif

// For windows compatibility
#if defined(WIN32) || defined(_WIN32)
typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

namespace mtgl {

#define BUILD_STATE (-1)

// For specifying algorithm return types.
enum { VERTICES = 0, EDGES };

// Filter types.
const int NO_FILTER =   0;
const int AND_FILTER =  1;
const int PURE_FILTER = 2;

// How edges should be treated.
const int UNDIRECTED = 0;
const int DIRECTED =   1;
const int REVERSED =   2;

// How we treat the parallelization of the mask row and each vert's neighbors.
const int SKIP_ROW = -1;
const int SERIAL = 0;
const int PARALLEL = 1;
const int PARALLEL_FUTURE = 2;

// Graph types.
const int MMAP   = -1;
const int DIMACS =  0;
const int RMAT   =  2;
const int PLOD   =  3;
const int RAND   =  4;
const int MESH   =  5;
const int MATRIXMARKET = 6;
const int SNAPSHOT     = 7;

// Graph struct types.
const int DYN     = 0;
const int CSR     = 1;
const int STATIC  = 2;
const int BIDIR   = 3;
const int ALGRAPH = 4;

/// \brief Templated function to initialize memory in a scalable way.
#ifdef USING_QTHREADS
template <typename T, int val>
void mt_init_inner(qthread_t* me, const size_t startat,
                   const size_t stopat, void* arg)
{
  if (sizeof(T) == sizeof(char) || val == 0)
  {
    memset(((T*) arg) + startat, val, sizeof(T) * (stopat - startat));
  }
  else
  {
    for (size_t i = startat; i < stopat; i++) ((T*) arg)[i] = val;
  }
}
#endif

template <typename T, int val>
void mt_init(T* const ptr, const size_t count)
{
#ifdef USING_QTHREADS
  qt_loop_balance(0, count, mt_init_inner<T, val>, ptr);
#else
  if (sizeof(T) == sizeof(char) || val == 0)
  {
    memset(ptr, val, count * sizeof(T));
  }
  else
  {
    for (size_t i = 0; i < count; i++) ptr[i] = val;
  }
#endif
}

template <typename T>
void mt_init(T* const ptr, const size_t count)
{
#ifdef USING_QTHREADS
  qt_loop_balance(0, count, mt_init_inner<T, 0>, ptr);
#else
  memset(ptr, 0, count * sizeof(T));
#endif
}

/// \brief Templated function to make access to readfe more convenient.
template <typename T>
T mt_readfe(T& toread)
{
#ifdef __MTA__
  return readfe(&(toread));
#elif USING_QTHREADS
  T ret;
  qthread_readFE(qthread_self(), &ret, &toread);
  return ret;
#else
  return toread;
#endif
}

/// \brief Templated function to make access to readff more convenient.
template <typename T>
T mt_readff(T& toread)
{
#ifdef __MTA__
  return readff(&(toread));
#elif USING_QTHREADS
  T ret;
  qthread_readFF(qthread_self(), &ret, &toread);
  return ret;
#else
  return toread;
#endif
}

/// \brief Templated function to make access to writeef more convenient.
template <typename T, typename T2>
void mt_write(T& target, const T2& towrite)
{
#ifdef __MTA__
  writeef(&(target), towrite);
#elif USING_QTHREADS
  qthread_writeEF_const(qthread_self(), (aligned_t*) &target,
                        (aligned_t) towrite);
#else
  target = towrite;
#endif
}

/// \brief Templated function to make access to int_fetch_add more convenient.
template <typename T, typename T2>
T mt_incr(T* target, T2 inc)
{
#ifdef __MTA__
  T res = int_fetch_add((int*)(target), inc);
#elif USING_QTHREADS
  T res = qthread_incr(target, inc);
#else
  T res = *target;
  *target = (*target) + inc;
#endif

  return res;
}

template <typename T, typename T2>
T mt_incr(T& target, T2 inc)
{
#ifdef __MTA__
  T res = int_fetch_add((int*) &(target), inc);
#elif USING_QTHREADS
  T res = qthread_incr(&(target), inc);
#else
  T res = target;
  target = (target) + inc;
#endif

  return res;
}

template <>
inline double mt_incr<double,double>(double& target, double inc)
{
#ifdef __MTA__
  double res = mt_readfe(target);
  mt_write(target, res+inc);
#elif USING_QTHREADS
  double res = qthread_dincr(&(target), inc);
#else
  double res = target;
  target = (target) + inc;
#endif

  return res;
}

template <typename T, typename T2>
T mt_incr(T& target, T2 inc, bool synch)
{
#ifdef __MTA__
  T res;
  if (synch)
  {
    res = int_fetch_add((sync T*) (int*) & (target), inc);
  }
  else
  {
    res = int_fetch_add((int*) &(target), inc);
  }
#elif USING_QTHREADS
  T res;
  if (synch)
  {
    res = qthread_incr(&(target), inc);
    qthread_fill(qthread_self(), &(target));
  }
  else
  {
    res = qthread_incr(&(target), inc);
  }
#else
  T res = target;
  target = (target) + inc;
#endif

  return res;
}

/// \brief Templated function to make access to int_fetch_add more convenient.
#ifdef __MTA__
template <typename T>
T mt_incr(sync T& target, T inc)
{
  T res = int_fetch_add((int*) &(target), inc);
  return res;
}

#elif USING_QTHREADS
template <typename T, typename T2>
T mt_incr_sync(T& target, T2 inc)
{
  T res = qthread_incr(&(target), inc);
  qthread_fill(qthread_self(), &(target));
  return res;
}

#endif

/// \brief Templated function to make access to 'purge' more convenient.
template <typename T>
void mt_purge(T& target)
{
#ifdef __MTA__
  purge(&(target));
#elif USING_QTHREADS
  qthread_empty(qthread_self(), &(target));
  target = 0;
#else
  target = 0;
#endif
}

/// \brief Templated function to make implicit fill bits more convenient.
template <typename T, typename T2>
void mt_writeF(T& target, const T2& thingy)
{
#ifdef USING_QTHREADS
  qthread_writeF_const(qthread_self(), (aligned_t*)&target, thingy);
#else
  target = thingy;
#endif
}

template <typename t>
void mt_lock(t& lock)
{
#ifdef __MTA__
  (void) readfe(&lock);
#elif USING_QTHREADS
  qthread_readFE(qthread_self(), &lock);
#endif
}

template <typename t>
void mt_unlock(t& lock)
{
#ifdef __MTA__
  writeef(&lock, 1);
#elif USING_QTHREADS
  qthread_writeEF(qthread_self(), &lock);
#endif
}

#ifdef __MTA__
template <typename vistype, typename dtype>
void mt_future_loop(int* x, int ulimit, vistype vis, dtype data)
{
  int i = mt_incr(*x, 1);

  if (i < ulimit)
  {
    future void $ Left;
    future void $ Right;
    future $ Left(x, ulimit, vis, data)
    {
      mt_future_loop(x, ulimit, vis, data);
    }
    future $ Right(x, ulimit, vis, data)
    {
      mt_future_loop(x, ulimit, vis, data);
    }
    do
    {
      vis(data[i]);
      i = mt_incr(*x, 1);
    } while (i < ulimit);
    $ Left;
    $ Right;
  }
}

#endif

class mt_timer {
public:
#ifdef __MTA__
  mt_timer() : ticks(0) { freq = mta_clock_freq(); }
#else
  mt_timer() {}
#endif

#ifdef __MTA__
  void start() { ticks = mta_get_clock(0); }
  void stop() { ticks = mta_get_clock(ticks); }
  double getElapsedSeconds() { return ticks / freq; }
  long getElapsedTicks() { return ticks; }

#elif defined(WIN32)
  void start() { start_time = std::clock(); }
  void stop() { stop_time = std::clock(); }

  double getElapsedSeconds()
  {
    std::clock_t ticks = stop_time - start_time;
    return ticks / (double) CLOCKS_PER_SEC;
  }

  std::clock_t getElapsedTicks()
  {
    std::clock_t ticks = stop_time - start_time;
    return ticks;
  }

#else
  void start() { gettimeofday(&start_time, NULL); }
  void stop() { gettimeofday(&stop_time, NULL); }

  long getElapsedTicks()
  {
    return (long) (getElapsedSeconds() * CLOCKS_PER_SEC);
  }

  double getElapsedSeconds()
  {
    double start_t = start_time.tv_sec + start_time.tv_usec * 1e-6;
    double stop_t = stop_time.tv_sec + stop_time.tv_usec * 1e-6;
    return stop_t - start_t;
  }
#endif

private:
#ifdef __MTA__
  int start_time;
  int stop_time;
  int ticks;
#elif defined(WIN32)
  std::clock_t start_time;
  std::clock_t stop_time;
#else
  struct timeval start_time, stop_time;
#endif
  double freq;
};

//void unstable_counting_sort(unsigned *src, unsigned *dst,
//                            unsigned n, unsigned universe) {
//
///* Algorithm:
//        1) Accumulate, in count(k), the number of keys with value k.
//        2) Compute a starting location, start[k], for each possible
//           key value k
//        3) Compute the location of each key value in the destination vector
//*/
//
//#pragma mta assert noalias *src, *dst
//  unsigned buckets = universe;
//  unsigned *start = new unsigned[buckets];
//  unsigned *count = new unsigned[buckets];
//
///* Zero elements of Count */
//  for ( unsigned i = 0; i < buckets; i++){
//    count[i] = 0;
//  }
//
///* Build histogram of src in count */
//  mt_timer timer;
//  timer.start();
//  for ( unsigned i = 0; i < n; i++)
//    count[src[i]]++;
//  printf("count[5]: %d\n", count[5]);
//  timer.stop();
//  printf("histogram: %f\n", timer.getElapsedSeconds());
//  fflush(stdout);
//
///* Find starting location for each bucket in start */
//  start[0] = 0;
//  for ( unsigned i = 1; i < buckets; i++)
//    start[i] = start[i - 1] + count[i - 1];
//
///* Place the elements of src into their buckets in dst */
//
///* Note that by running this loop in parallel, we can no longer guarentee
//   that elements with identical keys are left in their original order, thus
//   this approach is considered an unstable counting algorithm */
//
///* Since updates to start are atomic using int_fetch_add, we can now use
//   assert nodep pragma to let the compiler know it is safe to run this
//   loop in parallel. We could also accomplish this using an assert parallel
//   on the loop, but this, in general is less optimal. */
//
//#pragma mta assert nodep
//  for ( unsigned i = 0; i < n; i++) {
//  /* Value in start[k] indicates the first location used by bucket k */
//  /* This gets updated as each element is copied into place */
//    unsigned loc = int_fetch_add(start + src[i], 1);
//    dst[loc] = src[i];
//  }
//
//  delete [] count;
//  delete [] start;
//}

#pragma mta debug level none
template <typename T>
void countingSort(T* array, unsigned long asize, T maxval)  // size_type?
{
  T nbin = maxval + 1;
  T* count  = (T*) calloc(nbin, sizeof(T));
  T* result = (T*) calloc(asize, sizeof(T));

  for (unsigned long i = 0; i < asize; i++) count[array[i]]++;

  T* start = (T*) malloc(nbin * sizeof(T));
  start[0] = 0;

  for (unsigned long i = 1; i < nbin; i++)
  {
    start[i] = start[i - 1] + count[i - 1];
  }

  #pragma mta assert nodep
  for (unsigned long i = 0; i < asize; i++)
  {
    T loc = mt_incr<T>(start[array[i]], 1);
    result[loc] = array[i];
  }

  for (unsigned long i = 0; i < asize; i++) array[i] = result[i];

  free(count);
  free(start);
  free(result);
}

#pragma mta debug level default
#pragma mta debug level none

template <typename T, typename T2>
void bucket_sort(T* array, unsigned asize, T maxval, T2* data)
{
  typedef struct ss {
    T key;
    T2 data;
    struct ss* next;
  } bnode;

  int num_b = maxval + 1;
  bnode** buckets = (bnode**) calloc(num_b, sizeof(bnode*));
  T* count = (T*) calloc(num_b, sizeof(T));
  long* b_locks = (long*) calloc(num_b, sizeof(long));
  bnode* nodes = (bnode*) malloc(sizeof(bnode) * asize);
  int pos = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < asize; i++)
  {
    int p = mt_incr(pos, 1);
    T key = array[i];
    nodes[p].key = key;
    nodes[p].data = data[i];
    mt_readfe(b_locks[key]);
    nodes[p].next = buckets[key];
    buckets[key] = &nodes[p];
    mt_write(b_locks[key], 1);
    mt_incr(count[key], 1);
  }

  T* start = (T*) malloc(num_b * sizeof(T));
  start[0] = (T) 0;

  for (unsigned i = 1; i < num_b; i++) start[i] = start[i - 1] + count[i - 1];

  T incr = (T) 1;

  #pragma mta assert parallel
  for (unsigned i = 0; i < num_b; i++)
  {
    bnode* tmp = buckets[i];
    int loc = start[i];

    while (tmp)
    {
      array[loc] = tmp->key;
      data[loc] = tmp->data;
      tmp = tmp->next;
      loc = loc + 1;
    }
  }

  free(count);
  free(start);
  free(buckets);
  free(nodes);
  free(b_locks);
}

#pragma mta debug level default
#pragma mta debug level none

template <typename T, typename T2>
void bucket_sort_par_cutoff(T* array, unsigned asize, T maxval, T2* data,
                            int par_cutoff)
{
  typedef struct ss {
    T key;
    T2 data;
    struct ss* next;
  } bnode;

  int num_b = maxval + 1;
  bnode** buckets = (bnode**) calloc(num_b, sizeof(bnode*));
  T* count = (T*) calloc(num_b, sizeof(T));
  long* b_locks = (long*) calloc(num_b, sizeof(long));
  bnode* nodes = (bnode*) malloc(sizeof(bnode) * asize);
  int pos = 0;

  if (asize > par_cutoff)
  {
    #pragma mta assert parallel
    for (unsigned i = 0; i < asize; i++)
    {
      int p = mt_incr(pos, 1);
      T key = array[i];
      nodes[p].key = key;
      nodes[p].data = data[i];
      mt_readfe(b_locks[key]);
      nodes[p].next = buckets[key];
      buckets[key] = &nodes[p];
      mt_write(b_locks[key], 1);
      mt_incr(count[key], 1);
    }
  }
  else
  {
    for (unsigned i = 0; i < asize; i++)
    {
      int p = mt_incr(pos, 1);
      T key = array[i];
      nodes[p].key = key;
      nodes[p].data = data[i];
      mt_readfe(b_locks[key]);
      nodes[p].next = buckets[key];
      buckets[key] = &nodes[p];
      mt_write(b_locks[key], 1);
      mt_incr(count[key], 1);
    }
  }

  T* start = (T*) malloc(num_b * sizeof(T));
  start[0] = (T) 0;

  for (unsigned i = 1; i < num_b; i++) start[i] = start[i - 1] + count[i - 1];

  T incr = (T) 1;

  if (num_b > par_cutoff)
  {
    #pragma mta assert parallel
    for (unsigned i = 0; i < num_b; i++)
    {
      bnode* tmp = buckets[i];
      int loc = start[i];

      while (tmp)
      {
        array[loc] = tmp->key;
        data[loc] = tmp->data;
        tmp = tmp->next;
        loc = loc + 1;
      }
    }
  }
  else
  {
    for (unsigned i = 0; i < num_b; i++)
    {
      bnode* tmp = buckets[i];
      int loc = start[i];

      while (tmp)
      {
        array[loc] = tmp->key;
        data[loc] = tmp->data;
        tmp = tmp->next;
        loc = loc + 1;
      }
    }
  }

  free(count);
  free(start);
  free(buckets);
  free(nodes);
  free(b_locks);
}

#pragma mta debug level default
#pragma mta debug level none

template <typename T, typename T2>
void bucket_sort(T* array, unsigned asize, T maxval, T2* data, T*& start)
{
  typedef struct ss {
    T key;
    T2 data;
    struct ss* next;
  } bnode;

  unsigned int num_b = maxval + 1;
  bnode** buckets = (bnode**) calloc(num_b, sizeof(bnode*));
  T* count = (T*) calloc(num_b, sizeof(T));
  long* b_locks = (long*) calloc(num_b, sizeof(long));

  bnode* nodes = (bnode*) malloc(sizeof(bnode) * asize);
  long pos = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < asize; i++)
  {
    long p = mt_incr(pos, 1);
    T key = array[i];
    nodes[p].key = key;
    nodes[p].data = data[i];
    mt_readfe(b_locks[key]);
    nodes[p].next = buckets[key];
    buckets[key] = &nodes[p];
    mt_write(b_locks[key], 1);
    mt_incr(count[key], 1);
  }

  T* start_local = (T*) malloc(num_b * sizeof(T));
  start_local[0] = (T) 0;

  for (unsigned i = 1; i < num_b; i++)
  {
    start_local[i] =  start_local[i - 1] + count[i - 1];
  }

  start = start_local;
  start_local = 0;

  #pragma mta assert nodep
  for (unsigned i = 0; i < num_b; i++)
  {
    bnode* tmp = buckets[i];
    int loc = start[i];

    while (tmp)
    {
      array[loc] = tmp->key;
      data[loc] = tmp->data;
      tmp = tmp->next;
      loc = loc + 1;
    }
  }

  free(buckets);
  free(nodes);
  free(count);
  free(b_locks);
}

#pragma mta debug level default
#pragma mta debug level none

template <typename T, typename T2, typename T3>
void bucket_sort(T* array, unsigned asize, T maxval, T2* data, T3* data2)
{
  typedef struct ss {
    T key;
    T2 data;
    T3 data2;
    struct ss* next;
  } bnode;

  int num_b = maxval + 1;
  bnode** buckets = (bnode**) calloc(num_b, sizeof(bnode*));
  T* count = (T*) calloc(num_b, sizeof(T));
  int* b_locks = (int*) calloc(num_b, sizeof(int));
  bnode* nodes = (bnode*) malloc(sizeof(bnode) * asize);
  int pos = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < asize; i++)
  {
    int p = mt_incr(pos, 1);
    T key = array[i];
    nodes[p].key = key;
    nodes[p].data = data[i];
    nodes[p].data2 = data2[i];
    mt_readfe(b_locks[key]);
    nodes[p].next = buckets[key];
    buckets[key] = &nodes[p];
    mt_write(b_locks[key], 1);
    mt_incr(count[key], 1);
  }

  T* start = (T*) malloc(num_b * sizeof(T));
  start[0] = (T) 0;

  for (unsigned i = 1; i < num_b; i++) start[i] = start[i - 1] + count[i - 1];

  T incr = (T) 1;

  #pragma mta assert parallel
  for (unsigned i = 0; i < num_b; i++)
  {
    bnode* tmp = buckets[i];
    int loc = start[i];

    while (tmp)
    {
      array[loc] = tmp->key;
      data[loc] = tmp->data;
      data2[loc] = tmp->data2;
      tmp = tmp->next;
      loc = loc + 1;
    }
  }

  free(count);
  free(start);
  free(buckets);
  free(nodes);
  free(b_locks);
}

#pragma mta debug level default
#pragma mta debug level none

template <typename T>
void bucket_sort(T* array, unsigned asize, T maxval)
{
  typedef struct ss {
    T key;
    struct ss* next;
  } bnode;

  int num_b = maxval + 1;
  bnode** buckets = (bnode**) calloc(num_b, sizeof(bnode*));
  T* count = (T*) calloc(num_b, sizeof(T));
  int* b_locks = (int*) calloc(num_b, sizeof(int));
  bnode* nodes = (bnode*) malloc(sizeof(bnode) * asize);
  int pos = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < asize; i++)
  {
    int p = mt_incr(pos, 1);
    T key = array[i];
    nodes[p].key = key;
    mt_readfe(b_locks[key]);
    nodes[p].next = buckets[key];
    buckets[key] = &nodes[p];
    mt_write(b_locks[key], 1);
    mt_incr(count[key], 1);
  }

  T* start = (T*) malloc(num_b * sizeof(T));
  start[0] = (T) 0;

  for (unsigned i = 1; i < num_b; i++) start[i] = start[i - 1] + count[i - 1];

  T incr = (T) 1;

  #pragma mta assert parallel
  for (unsigned i = 0; i < num_b; i++)
  {
    bnode* tmp = buckets[i];
    int loc = start[i];

    while (tmp)
    {
      array[loc] = tmp->key;
      tmp = tmp->next;
      loc = loc + 1;
    }
  }

  free(count);
  free(start);
  free(buckets);
  free(nodes);
  free(b_locks);
}

#pragma mta debug level default

template <typename T>
void insertion_sort(T* array, unsigned asize)
{
  T value;
  int j;

  for (int i = 1; i < asize; i++)
  {
    value = array[i];
    j = i;

    while ((j > 0) && (array[j - 1] > value))
    {
      array[j] = array[j - 1];
      j = j - 1;
    }

    array[j] = value;
  }
}

template <typename T, typename T2>
void insertion_sort(T* array, unsigned asize, T2* data = 0)
{
  T value;
  T2 d;
  int j;

  if (data == 0)
  {
    for (int i = 1; i < asize; i++)
    {
      value = array[i];
      j = i;

      while ((j > 0) && (array[j - 1] > value))
      {
        array[j] = array[j - 1];
        j = j - 1;
      }

      array[j] = value;
    }
  }
  else
  {
    for (int i = 1; i < asize; i++)
    {
      value = array[i];
      d = data[i];
      j = i;

      while ((j > 0) && (array[j - 1] > value))
      {
        array[j] = array[j - 1];
        data[j] = data[j - 1];
        j = j - 1;
      }

      array[j] = value;
      data[j]  = d;
    }
  }
}

template <typename T>
bool binary_search(T* array, int array_size, T value, int& position)
{
  int low = 0;
  int high = array_size - 1;
  int mid;

  while (low <= high)
  {
    mid = (low + high) / 2;
    if (array[mid] > value)
    {
      high = mid - 1;
    }
    else if (array[mid] < value)
    {
      low = mid + 1;
    }
    else
    {
      position = mid;
      return true;
    }
  }

  return false;
}

template <typename T>
bool binary_search(T* array, T* array1, int array_size, T value, T value1,
                   int& position)
{
  int low = 0;
  int high = array_size - 1;
  int mid;

  while (low <= high)
  {
    mid = (low + high) / 2;

    if (array[mid] > value)
    {
      high = mid - 1;
    }
    else if (array[mid] < value)
    {
      low = mid + 1;
    }
    else
    {
      if (array1[mid] > value1)
      {
        high = mid - 1;
      }
      else if (array1[mid] < value1)
      {
        low = mid + 1;
      }
      else
      {
        position = mid;
        return true;
      }
    }
  }

  return false;
}

template <typename T> class dynamic_array;

template <typename T>
void findClasses(T* attr, int size, dynamic_array<T>& classes)
{
  unsigned* rcount = (unsigned*) calloc(size, sizeof(unsigned));

  #pragma mta assert nodep
  for (unsigned i = 0; i < size; i++)
  {
    int rep = mt_incr<unsigned>(rcount[attr[i]], 1);
  }

  int next = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < size; i++)
  {
    if (rcount[i] > 0) classes.push_back(i);
  }

  free(rcount);
}

template <typename T>
void findClasses(T* attr, int asz, int arange, dynamic_array<T>& classes)
{
  unsigned* rcount = (unsigned*) calloc(arange, sizeof(unsigned));

  #pragma mta assert nodep
  for (unsigned i = 0; i < asz; i++)
  {
    int rep = mt_incr<unsigned>(rcount[attr[i]], 1);
  }

  int next = 0;

  #pragma mta assert parallel
  for (unsigned i = 0; i < arange; i++)
  {
    if (rcount[i] > 0) classes.push_back(i);
  }

  free(rcount);
}

template <typename graph>
void extract_subproblem(int size, int* subproblem, int my_subproblem,
                        int*& active, int& num_active)
{
  int* count = (int*) malloc(sizeof(int) * size);
  count[0] = (subproblem[0] == my_subproblem);

  for (int i = 1; i < size; i++)
  {
    count[i] = count[i - 1] + (subproblem[i] == my_subproblem);
  }

  num_active = count[size - 1];
  free(count);

  active = (int*) malloc(sizeof(int) * num_active);

  int current = 0;

  #pragma mta assert nodep
  for (int i = 0; i < size; i++)
  {
    if (subproblem[i] == my_subproblem) active[mt_incr(current, 1)] = i;
  }
}

template <typename graph>
void extract_subproblem(int size, bool* subproblem,
                        int*& active, int& num_active)
{
  int* count = (int*) malloc(sizeof(int) * size);
  count[0] = subproblem[0];

  for (int i = 1; i < size; i++) count[i] = count[i - 1] + subproblem[i];

  num_active = count[size - 1];
  free(count);

  active = (int*) malloc(sizeof(int) * num_active);
  int current = 0;

  #pragma mta assert nodep
  for (int i = 0; i < size; i++)
  {
    if (subproblem[i]) active[mt_incr(current, 1)] = i;
  }
}

template <typename V, typename T>
void prefix_sums(V* value, T* result, int size)
{
  result[0] = value[0];

  for (int i = 1; i < size; i++) result[i] = result[i - 1] + value[i];
}

#pragma mta debug level default

template <typename T>
void init_mta_counters(mt_timer& timer, T& issues, T& memrefs, T& concur,
                       T& streams)
{
#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES);
  memrefs = mta_get_task_counter(RT_MEMREFS);
  concur = mta_get_task_counter(RT_CONCURRENCY);
  streams = mta_get_task_counter(RT_STREAMS);
#endif
  timer.start();
}

///
template <typename T>
void init_mta_counters(mt_timer& timer, T& issues, T& memrefs, T& concur,
                       T& streams, T& traps)
{
#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES);
  memrefs = mta_get_task_counter(RT_MEMREFS);
  concur = mta_get_task_counter(RT_CONCURRENCY);
  streams = mta_get_task_counter(RT_STREAMS);
  traps = mta_get_task_counter(RT_TRAP);
#endif
  timer.start();
}

///
template <typename T>
void sample_mta_counters(mt_timer& timer, T& issues, T& memrefs, T& concur,
                         T& streams)
{
#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  concur = mta_get_task_counter(RT_CONCURRENCY) - concur;
  streams = mta_get_task_counter(RT_STREAMS) - streams;
#endif
  timer.stop();
}

///
template <typename T>
void sample_mta_counters(mt_timer& timer, T& issues, T& memrefs, T& concur,
                         T& streams, T& traps)
{
#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  concur = mta_get_task_counter(RT_CONCURRENCY) - concur;
  streams = mta_get_task_counter(RT_STREAMS) - streams;
  traps = mta_get_task_counter(RT_TRAP) - traps;
#endif
  timer.stop();
}

///
template <typename T>
void print_mta_counters(mt_timer& timer, int m,
                        T& issues, T& memrefs, T& concur, T& streams)
{
#ifdef __MTA__
  fprintf(stderr, "secs: %lf, issues: %d, memrefs: %d, "
         "concurrency: %lf, streams: %lf\n",
         timer.getElapsedSeconds(), issues, memrefs,
         concur / (double) timer.getElapsedTicks(),
         streams / (double) timer.getElapsedTicks());
  printf("memrefs/edge: %lf\n",  memrefs / (double) m);
#else
  printf("secs: %lf\n", timer.getElapsedSeconds());
#endif
}

///
template <typename T>
void print_mta_counters(mt_timer& timer, int m,
                        T& issues, T& memrefs, T& concur, T& streams, T& traps)
{
#ifdef __MTA__
  printf("secs: %lf, issues: %d, memrefs: %d, "
         "concurrency: %lf, streams: %lf, traps: %d\n",
         timer.getElapsedSeconds(), issues, memrefs,
         concur / (double) timer.getElapsedTicks(),
         streams / (double) timer.getElapsedTicks(),
         traps);
  printf("memrefs/edge: %lf\n",  memrefs / (double) m);
#else
  printf("secs: %lf\n", timer.getElapsedSeconds());
#endif
}

#ifdef USING_QTHREADS

extern "C" inline aligned_t setaffin(qthread_t* me, void* arg)
{
#ifdef CPU_ZERO
  cpu_set_t* mask = (cpu_set_t*) malloc(sizeof(cpu_set_t));
  int cpu = *(int*) arg;

  CPU_ZERO(mask);
  CPU_SET(cpu, mask);

  if (sched_setaffinity(0, sizeof(cpu_set_t), mask) < 0)
  {
    perror("sched_setaffinity");
    free(mask);
    return -1;
  }

  free(mask);
#endif

  return 0;
}

inline void mtgl_qthread_init(int threads = 1)
{
  static bool initialized = false;
  if (!initialized)
  {
    qthread_init(threads);

#ifdef CPU_ZERO
    aligned_t rets[threads];
    int args[threads];

    for (int i = 0; i < threads; i++)
    {
      args[i] = i;
      qthread_fork_to(setaffin, args + i, rets + i, i);
    }

    for (int i = 0; i < threads; i++)
    {
      qthread_readFF(NULL, rets + i, rets + i);
    }
#endif
  }
}

#endif

// Note: We can't include <utility> to get std::pair because of MTA issues;
//       instead, we duplicate in our own namespace.
template <typename T1, typename T2>
class pair {

public:
  typedef T1 first_type;
  typedef T2 second_type;

  pair() : first(T1()), second(T2()) {}
  pair(const T1& x, const T2& y) : first(x), second(y) {}
  pair(const pair<T1, T2>& p) : first(p.first), second(p.second) {}

  template <typename U, typename V>
  pair(const pair<U, V>& p) : first(p.first), second(p.second) {}

  int operator==(const pair<T1, T2>& p)  const
  {
    return (first == p.first && second == p.second);
  }

  friend std::ostream& operator<<(std::ostream& os, const pair& p)
  {
    os << "[" << p.first << "," << p.second << "]";

    return os;
  }

public:
  T1 first;
  T2 second;
};

template <typename T1, typename T2>
class pair<T1&, T2&> {

public:
  typedef T1 first_type;
  typedef T2 second_type;

  pair() : first(T1()), second(T2()) {}
  pair(T1& x, T2& y) : first(x), second(y) {}
  pair(pair<T1, T2>& p) : first(p.first), second(p.second) {}

  template <typename U, typename V>
  pair(pair<U, V>& p) : first(p.first), second(p.second) {}

  pair<T1&, T2&>& operator=(const pair<T1&, T2&>& a)
  {
    first = a.first;
    second = a.second;
    return *this;
  }

  pair<T1&, T2&>& operator=(const pair<T1, T2>& a)
  {
    first = a.first;
    second = a.second;
    return *this;
  }

  int operator==(pair<T1, T2>& p)  const
  {
    return (first == p.first && second == p.second);
  }

  friend std::ostream& operator<<(std::ostream& os, const pair& p)
  {
    os << "[" << p.first << "," << p.second << "]";

    return os;
  }

public:
  T1& first;
  T2& second;
};

template <typename T1, typename T2, typename T3>
class triple {

public:
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;

  triple() : first(T1()), second(T2()), third(T3()) {}

  triple(const T1& x, const T2& y, const T3& z) :
    first(x), second(y), third(z) {}

  triple(const triple<T1, T2, T3>& p) :
    first(p.first), second(p.second), third(p.third) {}

  template <typename U, typename V, typename W>
  triple(const triple<U, V, W>& p) :
    first(p.first), second(p.second), third(p.third) {}

  int operator==(const triple<T1, T2, T3>& p)  const
  {
    return (first == p.first && second == p.second && third == p.third);
  }

  friend std::ostream& operator<<(std::ostream& os, const triple& p)
  {
    os << "[" << p.first << "," << p.second << "," << p.third << "]";

    return os;
  }

public:
  T1 first;
  T2 second;
  T3 third;
};

template <typename T1, typename T2, typename T3>
class triple<T1&, T2&, T3&> {

public:
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;

  triple() : first(T1()), second(T2()), third(T3()) {}

  triple(T1& x, T2& y, T3& z) : first(x), second(y), third(z) {}

  triple(triple<T1, T2, T3>& p) :
    first(p.first), second(p.second), third(p.third) {}

  template <typename U, typename V, typename W>
  triple(triple<U, V, W>& p) :
    first(p.first), second(p.second), third(p.third) {}

  triple<T1&, T2&, T3&>& operator=(const triple<T1&, T2&, T3&>& a)
  {
    first = a.first;
    second = a.second;
    third = a.third;
    return *this;
  }

  triple<T1&, T2&, T3&>& operator=(const triple<T1, T2, T3>& a)
  {
    first = a.first;
    second = a.second;
    third = a.third;
    return *this;
  }

  int operator==(triple<T1, T2, T3>& p)  const
  {
    return (first == p.first && second == p.second && third == p.third);
  }

  friend std::ostream& operator<<(std::ostream& os, const triple& p)
  {
    os << "[" << p.first << "," << p.second << "," << p.third << "]";

    return os;
  }

public:
  T1& first;
  T2& second;
  T3& third;
};

template <typename T1, typename T2, typename T3, typename T4>
class quadruple {

public:
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  typedef T4 fourth_type;

  quadruple() : first(T1()), second(T2()), third(T3()), fourth(T4()) {}

  quadruple(const T1& w, const T2& x, const T3& y, const T4& z) :
    first(w), second(x), third(y), fourth(z) {}

  quadruple(const quadruple<T1, T2, T3, T4>& q) :
    first(q.first), second(q.second), third(q.third), fourth(q.fourth) {}

  template <typename U, typename V, typename W, typename X>
  quadruple(const quadruple<U, V, W, X>& q) :
    first(q.first), second(q.second), third(q.third), fourth(q.fourth) {}

  int operator==(const quadruple<T1, T2, T3, T4>& q) const
  {
    return (first == q.first && second == q.second && third == q.third &&
            fourth == q.fourth);
  }

  friend std::ostream& operator<<(std::ostream& os, const quadruple& q)
  {
    os << "[" << q.first << "," << q.second << "," << q.third << ","
        << q.fourth << "]";

    return os;
  }

public:
  T1 first;
  T2 second;
  T3 third;
  T4 fourth;
};

template <typename T1, typename T2, typename T3, typename T4>
class quadruple<T1&, T2&, T3&, T4&> {

public:
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  typedef T4 fourth_type;

  quadruple() : first(T1()), second(T2()), third(T3()), fourth(T4()) {}

  quadruple(T1& w, T2& x, T3& y, T4& z) :
    first(w), second(x), third(y), fourth(z) {}

  quadruple(quadruple<T1, T2, T3, T4>& p) :
    first(p.first), second(p.second), third(p.third), fourth(p.fourth) {}

  template <typename U, typename V, typename W, typename X>
  quadruple(quadruple<U, V, W, X>& p) :
    first(p.first), second(p.second), third(p.third), fourth(p.fourth) {}

  quadruple<T1&, T2&, T3&, T4&>&
  operator=(const quadruple<T1&, T2&, T3&, T4&>& a)
  {
    first = a.first;
    second = a.second;
    third = a.third;
    fourth = a.fourth;
    return *this;
  }

  quadruple<T1&, T2&, T3&, T4&>& operator=(const quadruple<T1, T2, T3, T4>& a)
  {
    first = a.first;
    second = a.second;
    third = a.third;
    fourth = a.fourth;
    return *this;
  }

  int operator==(quadruple<T1, T2, T3, T4>& p)  const
  {
    return (first == p.first && second == p.second && third == p.third &&
            fourth == p.fourth);
  }

  friend std::ostream& operator<<(std::ostream& os, const quadruple& p)
  {
    os << "[" << p.first << "," << p.second << "," << p.third << ","
        << p.fourth << "]";

    return os;
  }

public:
  T1& first;
  T2& second;
  T3& third;
  T4& fourth;
};

template <typename T1, typename T2>
inline pair<T1&, T2&> tie(T1& t1, T2& t2)
{
  return pair<T1&, T2&> (t1, t2);
}

template <typename T1, typename T2, typename T3>
inline triple<T1&, T2&, T3&> tie(T1& t1, T2& t2, T3& t3)
{
  return triple<T1&, T2&, T3&> (t1, t2, t3);
}

template <typename T1, typename T2, typename T3, typename T4>
inline quadruple<T1&, T2&, T3&, T4&> tie(T1& t1, T2& t2, T3& t3, T4& t4)
{
  return quadruple<T1&, T2&, T3&, T4&> (t1, t2, t3, t4);
}

template <typename T>
static void order_pair(T& a, T& b)
{
  if (a > b)
  {
    T tmp = a;
    a = b;
    b = tmp;
  }
}

template <typename graph>
void
init_identity_vert_array(graph& g,
                         typename graph_traits<graph>::vertex_descriptor* a)
{
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  size_type n = num_vertices(g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert nodep
  for (size_type i = 0; i < n; i++) a[i] = verts[i];
}

template <typename graph>
void
init_identity_vert_map(graph& g,
                       std::map<typename graph_traits<graph>::vertex_descriptor,
                       typename graph_traits<graph>::vertex_descriptor>& m)
{
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  size_type n = num_vertices(g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert nodep
  for (size_type i = 0; i < n; i++) m[verts[i]] = verts[i];
}

/*! \brief This function is useful when using the forall streams construct
           and you want to partition an array into blocks.

    \param num_elements Total number of elements that need to be paritioned.
    \param num_streams Total number of streams available.
    \param stream_id The stream_id of the current stream.
    \param beg This is set to the index of the first element in the stream's
               partition.
    \param end This is set to the index of the last element in the stream's
               partition.

    \author Eric L. Goodman
    \date May 20, 2010
*/
template <typename T>
inline
void determine_beg_end(T num_elements, T num_streams, T stream_id,
                       T& begin, T& end)
{
  begin = static_cast<T>((static_cast<double>(num_elements) / num_streams) *
                         stream_id);

  if (stream_id + 1 < num_streams)
  {
    end = static_cast<T>((static_cast<double>(num_elements) / num_streams) *
                         (stream_id + 1));
  }
  else
  {
    end = num_elements;
  }
}

}

#endif
