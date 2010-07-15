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
/*! \file mtgl_boost_property.hpp

    \brief This file contains stubs of Boost Graph Library functionality for
           property maps.  We implement a lightweight version of these in the
           MTGL.

    \author Jon Berry (jberry@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_MTGL_BOOST_PROPERTY_HPP
#define MTGL_MTGL_BOOST_PROPERTY_HPP

#include <mtgl/dynamic_array.hpp>
#include <mtgl/xmt_hash_table.hpp>

namespace mtgl {

/****************************************************************************/
/*
    The following code fragments are extracted from boost 1_33_1.  This file
    may go away in the future if mtgl can use the boost code explicitly.

    <include copyright>

*/
/****************************************************************************/

struct readable_property_map_tag {};
struct writable_property_map_tag {};
struct read_write_property_map_tag : public readable_property_map_tag,
                                     public writable_property_map_tag {};

struct lvalue_property_map_tag : public read_write_property_map_tag {};

template <typename PropertyMap>
struct property_traits {
  typedef typename PropertyMap::key_type key_type;
  typedef typename PropertyMap::value_type value_type;
};

template <typename T>
struct property_traits<T*> {
  typedef typename T::key_type key_type;
  typedef typename T::value_type value_type;
};

template <typename T>
struct property_traits<const T*> {
  typedef typename T::key_type key_type;
  typedef typename T::value_type value_type;
};

template <class Reference, class LvaluePropertyMap>
struct put_get_helper {};

template <class PropertyMap, class Reference, class K>
inline Reference
get(const put_get_helper<Reference, PropertyMap>& pa, const K& k)
{
  Reference v = static_cast<const PropertyMap&>(pa)[k];
  return v;
}

template <class PropertyMap, class Reference, class K, class V>
inline void
put(const put_get_helper<Reference, PropertyMap>& pa, const K& k, const V& v)
{
  static_cast<const PropertyMap&>(pa)[k] = v;
}

template <typename E, typename IdMap>
class array_property_map :
  public put_get_helper<E&, array_property_map<E, IdMap> > {
public:
  typedef typename property_traits<IdMap>::key_type key_type;
  typedef E value_type;
  array_property_map(E* a, IdMap id) : data(a), id_map(id) {}
  array_property_map(dynamic_array<E>& a, IdMap id) :
    data(a.get_data()), id_map(id) {}

  value_type& operator[] (key_type k) const { return data[get(id_map, k)]; }
  E* get_data() const { return data; }       // cheating for XMT!

private:
  E* data;
  IdMap id_map;
};

template <typename E, typename IdMap>
class map_property_map {
public:
  typedef typename property_traits<IdMap>::key_type key_type;
  typedef E value_type;
  map_property_map(xmt_hash_table<typename property_traits<IdMap>::value_type,
                                  E>& m,
                   IdMap id) : data(m), id_map(id) {}

  value_type& operator[] (key_type k) const { return data[get(id_map, k)]; }

  xmt_hash_table<typename property_traits<IdMap>::value_type, E>& data;
  IdMap id_map;
};

template <typename E, typename IdMap>
inline
E
get(const map_property_map<E, IdMap>& pa,
    const typename property_traits<IdMap>::key_type& k)
{
  E val;
  pa.data.lookup(get(pa.id_map, k), val);
  return val;
}

template <typename E, typename IdMap, typename K>
inline void
put(const map_property_map<E, IdMap>& pa,
    const typename property_traits<IdMap>::key_type& k, const K& v)
{
  pa.data.update_insert(get(pa.id_map, k), v);
}

template <typename E, typename IdMap>
inline array_property_map<E, IdMap>
make_array_property_map(E* a, IdMap id)
{
  return array_property_map<E, IdMap>(a, id);
}

}

#endif
