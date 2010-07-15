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
/*! \file mtgl_io.hpp

    \author Greg Mackey (gemacke@sandia.gov)
    \author Eric Goodman (elgoodm@sandia.gov)

    \date 2/11/2010
*/
/****************************************************************************/

#ifndef MTGL_MTGL_IO_HPP
#define MTGL_MTGL_IO_HPP

#include <cstdlib>
#include <cstdio>

#include <mtgl/snap_util.h>
#include <mtgl/read_binary.hpp>
#include <mtgl/write_binary.hpp>

namespace mtgl {

/*! \brief A convenience method that given a file, it returns the size of
           the file.  Returns -1 if something goes wrong.  Assumes that
           snap_init has already been called.  snap_util.h should hopefully
           make this compatible with non XMT systems.

    \param file The file that will be checked.

    \author Eric L. Goodman
    \date March 2010
*/
inline
int64_t get_file_size(char *file)
{
  luc_error_t err;
  int64_t snap_error = 0;

  // Get the endpoint for the file service worker.
  char *fsw = getenv("SWORKER_EP");
  luc_endpoint_id_t endpoint = 0;
  if (fsw != NULL) endpoint = strtoul(fsw, NULL, 16);

#ifdef DEBUG
  fprintf(stderr, "endpoint: %lu\n", (unsigned long) endpoint);
#endif

  // Used to store the results of the stat call.
  snap_stat_buf stat_buf;
  err = snap_stat(file, endpoint, &stat_buf, &snap_error);

  if (SNAP_ERR_OK != err)
  {
    fprintf(stderr, "Failed to retrieve info on file %s. Error %d.\n",
            file, err);
    return -1;
  }

  return stat_buf.st_size;
}

/*! \brief Reads a binary file and puts the contents of the file into a
           buffer of type array_t.

    \param filename The name of the file to be read in via snap_restore
    \param size The number of array_t elements in the returned array.  It
                is set to -1 if there are problems.

    \returns Returns an array of array_t elements.

    \author Eric Goodman refactored the snapshot_array method
    \date March 2010
*/
template <typename array_t>
inline
array_t* read_array(char* filename, int& array_size)
{
  array_t* array;
  array_size = -1;

  // Initializing snapshot libraries.
  if (snap_init() != SNAP_ERR_OK)
  {
    perror("Can't initialize libsnapshot.\n");
    return NULL;
  }

  // Getting the size of the file.
  int64_t file_size = get_file_size(filename);

  // Creating the buffer to store the data from the file.
  void* buffer = (void*) malloc(file_size);

  if (buffer == NULL)
  {
    perror("Failed to malloc snapshot buffer.\n");
    return NULL;
  }
 
  if (file_size > 0)
  {
    if (snap_restore(filename, buffer, file_size, NULL) != SNAP_ERR_OK)
    {
      perror("Couldn't snap_restore file.\n");
      free(buffer);
      return NULL;
    }

    // Setting the size of the array.
    array_size = file_size / sizeof(array_t);
    
    // Casting the buffer to be the appropriate type.
    array = (array_t*) buffer;
    
    return array;
  }

  return NULL;
}

template <typename array_t>
inline
array_t* read_array(const char* filename, int& array_size)
{
  return read_array<array_t>(const_cast<char*>(filename), array_size);
}

/*! \brief Writes a binary file with the contents of the given array.

    \param filename The name of the file to be written to via snap_snapshot
    \param array The array to written
    \param size The number of array_t elements in array.

    \author Eric Goodman 

    \date March 2010
*/
template <typename array_t>
inline
void write_array(char* filename, array_t* array, int array_size)
{
  // Initializing snapshot libraries.
  if (snap_init() != SNAP_ERR_OK)
  {
    perror("Can't initialize libsnapshot.\n");
  }

  int file_size = array_size * sizeof(array_t);
  if (snap_snapshot(filename, array, file_size, NULL) != SNAP_ERR_OK)
  {
    perror("Couldn't snap_snapshot file.\n");
  }
}

template <typename array_t>
inline
void write_array(const char* filename, array_t* array, int array_size)
{
  write_array(const_cast<char*>(filename), array, array_size);
}

}

#endif
