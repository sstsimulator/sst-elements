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
/*! \file hash_defs.hpp

    \brief Definition of default hash functions, key equality functions,
           and update function objects for hash tables.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/31/2010
*/
/****************************************************************************/

#ifndef MTGL_HASH_DEFS_HPP
#define MTGL_HASH_DEFS_HPP

#include <mtgl/util.hpp>

namespace mtgl {

/// \brief  Function object that serves as the default equality comparison
///         for hash sets and hash tables.
template <typename T>
struct default_eqfcn {
  bool operator()(const T& t1, const T& t2) const { return t1 == t2; }
};

template <>
struct default_eqfcn<char*> {
  bool operator()(const char* t1, const char* t2) const
  { return strcmp(t1, t2) == 0; }
};

template <>
struct default_eqfcn<const char*> {
  bool operator()(const char* t1, const char* t2) const
  { return strcmp(t1, t2) == 0; }
};

/// Definition for size_type used by hash sets and hash tables.
typedef unsigned long hash_size_type;

/// Default hash function for strings, char arrays, etc.
template <typename T>
inline
hash_size_type string_hash_func(const char* key, const T& key_len)
{
  hash_size_type hash = 0;

  for (T i = 0; i < key_len; ++i)
  {
    hash = key[i] + (hash << 6) + (hash << 16) - hash;
  }

  return hash;
}

/// Default hash function for all integer types.
template <typename T>
inline
hash_size_type integer_hash_func(const T& key)
{
/*
    hash_size_type k = static_cast<hash_size_type>(key);

    k = (~k) + (k << 21); // k = (k << 21) - k - 1;
    k = k ^ (k >> 24);
    k = (k + (k << 3)) + (k << 8); // k * 265
    k = k ^ (k >> 14);
    k = (k + (k << 2)) + (k << 4); // k * 21
    k = k ^ (k >> 28);
    k = k + (k << 31);

    return k;
*/

//    return static_cast<hash_size_type>(key);
    return static_cast<hash_size_type>(key) * 31280644937747LL;
//    return static_cast<hash_size_type>(key) * 19922923;
}

/// \brief  Function object that serves as the default hash function for a
///         variety of key types for hash_sets and hash_tables.
template <typename T>
struct default_hash_func {};

template <>
struct default_hash_func<short>
{
  hash_size_type operator()(short key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<unsigned short>
{
  hash_size_type operator()(unsigned short key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<int>
{
  hash_size_type operator()(int key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<unsigned int>
{
  hash_size_type operator()(unsigned int key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<long>
{
  hash_size_type operator()(long key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<unsigned long>
{
  hash_size_type operator()(unsigned long key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<long long>
{
  hash_size_type operator()(long long key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<unsigned long long>
{
  hash_size_type operator()(unsigned long long key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<std::string> {
public:
  hash_size_type operator()(const std::string& key) const
  { return string_hash_func(key.c_str(), key.size()); }
};

template <>
struct default_hash_func<const std::string> {
public:
  hash_size_type operator()(const std::string& key) const
  { return string_hash_func(key.c_str(), key.size()); }
};

template <>
struct default_hash_func<char*> {
public:
  hash_size_type operator()(const char* key) const
  { return string_hash_func(key, strlen(key)); }
};

template <>
struct default_hash_func<const char*> {
public:
  hash_size_type operator()(const char* key) const
  { return string_hash_func(key, strlen(key)); }
};

template <>
struct default_hash_func<char> {
public:
  hash_size_type operator()(char key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<unsigned char> {
public:
  hash_size_type operator()(unsigned char key) const
  { return integer_hash_func(key); }
};

template <>
struct default_hash_func<signed char> {
public:
  hash_size_type operator()(signed char key) const
  { return integer_hash_func(key); }
};

/// Function object for update functions that performs an mt_incr.
template <typename T>
struct hash_mt_incr
{
  void operator()(T& v, const T& value) const
  { mt_incr(v, value); }
};

/// \brief Function object for update functions that updates a value only if
///        the new value is less than the current value.
template <typename T>
struct hash_min
{
  void operator()(T& v, const T& value) const
  {
    if (value < v)
    {
      T probe = mt_readfe(v);
      if (value < v)
      {
        mt_write(v, value);
      }
      else
      {
        mt_write(v, probe);
      }
    }
  }
};

};

#endif
