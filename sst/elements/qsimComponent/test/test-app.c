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
