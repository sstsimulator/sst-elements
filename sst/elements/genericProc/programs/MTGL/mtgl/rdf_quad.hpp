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
/*! \file qalloc.h

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 3/25/2010
*/
/****************************************************************************/

#ifndef MTGL_RDF_QUAD_HPP
#define MTGL_RDF_QUAD_HPP

namespace mtgl {

template <typename T>
class rdf_quad {
public:
  rdf_quad(T in_object, T in_predicate, T in_subject, T in_context) :
    object_id(in_object), predicate_id(in_predicate),
    subject_id(in_subject), context_id(in_context) {}

  T object_id;
  T predicate_id;
  T subject_id;
  T context_id;
};

}

#endif
