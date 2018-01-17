
#include<iostream>
#include<cstdlib>
#include<cstdio>
#include<string>

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

void * ariel_mmap_mlm(int ID, size_t size, int level) { return malloc(size); }

}

#pragma GCC pop_options

int main()
{
   ariel_enable();
   int * x = (int *) malloc(sizeof(int) * NUM_INTS);
   int * y = (int *) mlm_malloc(sizeof(int) * NUM_INTS, 1);

   ariel_mmap_mlm(660066, 4096, 2);

   for (int i = 0; i < NUM_INTS; )
   {
      y[i] = i * 2;
      i = i + 1024;
   }

   for (int i = 0; i < NUM_INTS; i++)
      cout << "Value is: " << y[i] << endl;

   cout << "Test MLM Malloc" << endl;


   return 0;
}
