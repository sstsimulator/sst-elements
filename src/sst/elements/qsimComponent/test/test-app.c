// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

#define SIZE 5000

int array[SIZE];

int main(int argc, char** argv) {
  int i, j;

  // Initialize the array.
  array[0] = array[1] = 0;
  for (i = 2; i < SIZE; ++i) array[i] = i;

  // Perform the sieve.
  for (i = 2; i < SIZE; ++i)
    if (array[i])
      for (j = i*i; j < SIZE; j += i)
        array[j] = 0;

  // Print the results.
  for (i = 0; i < SIZE; ++i) 
    if (array[i])
      printf("%d\n", array[i]);

  return 0;
}
