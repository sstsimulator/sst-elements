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
/*! \file allocation_pool.hpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 3/25/2010
*/
/****************************************************************************/

#ifndef MTGL_ALLOCATION_POOL_HPP
#define MTGL_ALLOCATION_POOL_HPP

#include <cstdlib>
#include <cstdio>
#include <cmath>

#pragma mta once
#include <mta_rng.h>
#include <machine/mtaops.h>
#include <mtgl/random_number_generator.hpp>

#define MALLOC_STYLE 0
#define NEW_STYLE    1

#define MAX_RAND_VAL_SIZE 1048576

#define MAX_DOMAIN 256 * 512

namespace mtgl {

//TODO: Memory leak when sections run out and we create things on the fly.
template <typename T>
class allocation_pool {
public:

  /// \brief Takes the number of sections and rounds it up to the next power
  ///        of two.  Size is made divisible by the number of sections.
  allocation_pool(unsigned int in_size, unsigned int inNumSections, int style)
  {
    size = in_size;

    // Setting numSections to a power of two.
    double doubleNumSections = log(inNumSections) / log(2);

    // Now numSections = log_2 of the number of sections.

    if (doubleNumSections - floor(doubleNumSections) > 0)
    {
      // Not a power of 2, rounding up to the next one.
      numSections = pow(2, floor(doubleNumSections) + 1);
    }
    else
    {
      // A power of two.
      numSections = pow(2, doubleNumSections);
    }

    sectionMask = numSections - 1;

    // Now numSections should be a power of 2, greater than or equal to
    // inNumSections.

    // Making size divisible by numSections.
    double increment = (double) size / (double) numSections;
    if (increment - floor(increment) > 0)
    {
      increment = floor(increment) + 1;
      size = increment * numSections;
    }

    section_pointers = new unsigned int[numSections];

    printf("pool size %d pool numsections %d\n", size, numSections);

    // Make it one larger so that we don't need special logic for the last
    // section.
    orig_positions = new unsigned int[numSections + 1];

    #pragma mta assert nodep
    for (int i = 0; i < numSections; i++)
    {
      section_pointers[i] = i * increment;
      orig_positions[i] = i * increment;
    }
    orig_positions[numSections] = size;

    if (style == MALLOC_STYLE)
    {
      malloc_pool = (T*) malloc(sizeof(T) * size);
      new_pool = NULL;
    }
    else if (style == NEW_STYLE)
    {
      new_pool = new T *[size];

      #pragma mta assert nodep
      for (int i = 0; i < size; i++) new_pool[i] = new T();

      malloc_pool = NULL;
    }
    else
    {
      fprintf(stderr, "Unrecognized style specification in allocation_pool\n");
    }

    rng = new random_number_generator<unsigned int>();
  }

  ~allocation_pool()
  {
    if (malloc_pool != NULL) free(malloc_pool);

    if (new_pool != NULL)
    {
      for (int i = 0; i < size; i++)
      {
        if (new_pool[i] != NULL) delete (new_pool[i]);
      }

      delete new_pool;
    }

    delete rng;
    delete orig_positions;
    delete section_pointers;
  }

  // size_block is ignored when NEW_STYLE is being used.
  T* get(unsigned int size_block, int retry = 0)
  {
    unsigned int randNum = rng->get_random_number();
    unsigned int section = randNum & sectionMask;
    return get_block(section, size_block, retry);
  }

  T* get_block(unsigned int section, unsigned int size_block, int retry)
  {
    unsigned int index = mt_incr(section_pointers[section], size_block);

//    printf("section %u index %u size_block %u \n", section, index,
//           size_block);

    if (index + size_block <= orig_positions[section + 1])
    {
      if (style == NEW_STYLE)
      {
        return new_pool[index];
      }
      else
      {
        return &malloc_pool[index];
      }
    }
    else
    {
      if (retry < 2) return get(size_block, ++retry);

      //printf("*******************************888\n");
      //#pragma mta trace "Created new object in allocation pool"
      if (style == NEW_STYLE)
      {
        return new T();
      }
      else
      {
        return (T*) malloc(size_block * sizeof(T));
      }
    }
  }

  //#pragma mta inline
  int getSize() { return size; }

  //#pragma mta inline
  int get_num_sections() { return numSections; }

private:
  int size;        // The size of the pool.
  T** new_pool;    // The pool of objects to draw from created by new.
  T* malloc_pool;  // The pool of objects to draw from create by malloc.
  unsigned int* section_pointers;  // Keeps track of where we are in the pool.
  unsigned int* orig_positions;    // The original positions of the section
                                   // pointers.
  unsigned int numSections;       // The number of sections in the pool.
  unsigned int sectionMask;       // Used to do moding.
  int style;       // Signifies whether we are using malloc or new.
  random_number_generator<unsigned int>* rng;  // Allows us to select randomly
                                               // from the sections
};

}

#endif
