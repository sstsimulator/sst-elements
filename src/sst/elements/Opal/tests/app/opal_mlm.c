
#include<iostream>
#include<cstdlib>
#include<cstdio>
#include<string>

#include <sys/mman.h>


#define NUM_INTS 1000000

using namespace std;

#pragma GCC push_options
#pragma GCC optimize ("O0")

extern "C" {

void* mlm_malloc(size_t size, int level)
{
	if(size == 0)
      {
		printf("ZERO BYTE MALLOC\n");
		void* bt_entries[64];
		exit(-1);
	}

	printf("Performing a mlm Malloc for size %llu\n", size);

	return malloc(size);
}

void ariel_enable() { }

void * ariel_mmap_mlm(int ID, size_t size, int level) { return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, ID, 0); }

}

#pragma GCC pop_options

int main()
{
   ariel_enable();
   int * x = (int *) malloc(sizeof(int) * NUM_INTS);
   int * y = (int *) mlm_malloc(sizeof(int) * NUM_INTS, 1);

   int * z = (int *) ariel_mmap_mlm(660066, sizeof(int) * NUM_INTS, 2);

   for (int i = 0; i < NUM_INTS; )
   {
      cout << "*** " << i << " ***\n";
      x[i] = i * 2;
      y[i] = i * 2;
      z[i] = i * 2;
      i = i + 1024;
   }

   for (int i = 0; i < NUM_INTS; )
   {
      cout << "Value is: " << x[i] << ", " << y[i] << ", " << z[i] << endl;
      i = i + 1024;
   }

   cout << "Test MLM Malloc" << endl;


   return 0;
}
