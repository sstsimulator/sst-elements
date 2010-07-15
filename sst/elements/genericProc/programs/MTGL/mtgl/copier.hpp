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
/*! \file copier.hpp

    \brief A class supporting the idiom of copying via visitation as opposed
           to copying with iterators.

    \author Jon Berry (jberry@sandia.gov)

    \date 4/22/2005
*/
/****************************************************************************/

#ifndef MTGL_COPIER_HPP
#define MTGL_COPIER_HPP

#include <climits>

#include <mtgl/xmt_hash_set.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

/// \brief A parent for all copy filter classes.  The latter contain
///        references to the destination structure.
template <typename t>
struct default_copy_filter {
   inline void copy_element(t&elem) { }
};

/*! \brief This class controls parallel copying from a structure of type
           FromMap containing elements of type elem_type, and filters the
           copy operations with copy_filter.

    \warning Copying is typically slow at the moment if the destination
             is a node-based structure like a hash table -- there are
             memory allocation issues.
*/
template <typename elem_type, typename FromMap,
          typename copy_filter = default_copy_filter<elem_type> >
class Copier {
public:
  Copier(FromMap& f, copy_filter& flt);
  Copier(const Copier& c) : filter(c.filter), from(c.from) {}

  void apply();
  void copy_one(elem_type e) const;

  copy_filter filter;
  FromMap& from;

  struct copy_visitor {
    const copy_filter filter;

    copy_visitor(copy_filter& vis) : filter(vis) {}

    void operator()(elem_type e) const { filter.copy_element(e); }
  };

  /*! \var typedef void (Algorithm::*cFunc)(elem_type, Env);
      \brief cFunc denotes a pointer to the function that will
            do the individual copy operations.
  */
  /*! \fn Copier(FromMap& f, copy_filter& flt);
      \brief Initial the members.
      \param f The source structure for the copy.
      \param flt The copy filter, which will have a reference to the
                 destination structure.
  */
  /*! \fn apply()
      \brief invoke the copy operation
  */
  /*! \fn copy_one(elem_type e, Env d)
      \brief This is called by the visit method of the FromMap (the copy
             source), typically in parallel.
      \param e The source element to be copied
      \param d Environment information passed by the closure.
  */
};

template <typename elem_type, typename FromMap,
          typename copy_filter = default_copy_filter<elem_type> >
class IndexedCopier : public Copier<elem_type, FromMap, copy_filter> {
public:
  IndexedCopier(FromMap& f, copy_filter& flt) :
    Copier<elem_type, FromMap, copy_filter>(f, flt) {}
  IndexedCopier(const IndexedCopier& c) :
    Copier<elem_type, FromMap, copy_filter>(c.filter, c.from) {}

  void apply();
};

template <typename elem_type, typename FromMap, typename copy_filter>
Copier<elem_type, FromMap, copy_filter>::Copier(FromMap& f,
                                                copy_filter& filt) :
    from(f), filter(filt)
{
}

template <typename elem_type, typename FromMap, typename copy_filter>
void Copier<elem_type, FromMap, copy_filter>::copy_one(elem_type e) const
{
  filter.copy_element(e);
}


template <typename elem_type, typename FromMap, typename copy_filter>
void Copier<elem_type, FromMap, copy_filter>::apply()
{
  copy_visitor helper(filter);
  from.visit(helper);
  // problem: if 'from' is DynArray, default
  // visit is serial (need visitPar)
}

template <typename elem_type, typename FromMap, typename copy_filter>
void IndexedCopier<elem_type, FromMap, copy_filter>::apply()
{
  typename Copier<elem_type, FromMap, copy_filter>::copy_visitor
    helper(Copier<elem_type, FromMap, copy_filter>::filter);
  Copier<elem_type, FromMap, copy_filter>::from.visit(helper);
  // problem: if 'from' is DynArray, default
  // visit is serial (need visitPar)
}

template <typename elem>
class hash_set_to_dynamic_array_filter : public default_copy_filter<elem> {
public:
  hash_set_to_dynamic_array_filter(dynamic_array<elem>& tm, int& r) :
    dest(tm), rank(r) {}

  inline void copy_element(elem&e)  const
  {
    int myrank = mt_incr(rank, 1);
    dest.push_back(e);
  }

private:
  dynamic_array<elem>& dest;
  int& rank;
};

template <typename elem>
class to_array_filter : public default_copy_filter<elem> {
public:
  to_array_filter(elem* tm, int& r) : dest(tm), rank(r) {}

  inline void copy_element(elem&e)  const
  {
    int r = mt_incr(rank,1);
    dest[r] = e;
  }

private:
  elem *dest;
  int& rank;
};

template <class elem>
void copy(dynamic_array<elem>& da, xmt_hash_set<elem>& hs)
{
  int rank = 0;
  hash_set_to_dynamic_array_filter<elem> hf(da, rank);

  Copier<elem, xmt_hash_set<elem>,
         hash_set_to_dynamic_array_filter<elem> > copier(hs, hf);
  copier.apply();
}

template<typename elem>
void copy(elem* res, dynamic_array<elem>& hs)
{
  elem *store = hs.get_data();
  int sz = hs.size();

  #pragma mta assert nodep
  for (int i = 0; i < sz; ++i) res[i] = store[i];
}

template<typename elem>
void copy(elem* res, xmt_hash_set<elem>& hs)
{
  int rank = 0;
  to_array_filter<elem> taf(res, rank);

  Copier<elem, xmt_hash_set<elem>, to_array_filter<elem> > copier(hs, taf);
  copier.apply();
}

// Specializations.

/*
template <typename elem_type, typename FromMap, typename copy_filter>
class Copier<elem_type, FromMap, copy_filter, int> {
public:
  typedef void (Algorithm::*cFunc2)(elem_type);
  Copier(FromMap& f, copy_filter& flt);

  void apply();
  void copy_one(elem_type);

  copy_filter filter;
  closure<elem_type> copyClosure;
  FromMap& from;
  int rank;
};

template <typename elem_type, typename FromMap, typename copy_filter>
Copier<elem_type, FromMap, copy_filter, int>::Copier(FromMap& f,
                                                     copy_filter& filt) :
  from(f), filter(filt), rank(0),
  copyClosure(this,
              (cFunc2) &eldorado::Copier<elem_type, FromMap,
                                         copy_filter>::copy_one)
{
}

template <typename elem_type, typename FromMap, typename copy_filter>
void Copier<elem_type, FromMap, copy_filter, int>::copy_one(elem_type e)
{
  filter.copy_element(e);
}

template <typename elem_type, typename FromMap, typename copy_filter>
void Copier<elem_type, FromMap, copy_filter, int>::apply()
{
  from.visit(copyClosure);  // problem: if 'from' is DynArray, default
                            // visit is serial
}
*/

}

#endif
