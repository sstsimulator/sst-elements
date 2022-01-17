
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int value = 1234567;

	if(argc > 1) {
		value = atoi(argv[1]);
	}

	double value_dbl = (double) value;
	float  value_flt = (float) value;

	long long int value_lli = (long long int) value;
   unsigned int value_ui = (unsigned int) value;
	unsigned long long int value_uli = (unsigned long long int) value;

	printf("value (int) is %d\n", value);
	printf("value (dbl) is %f\n", value_dbl);
	printf("value (flt) is %f\n", value_flt);
	printf("value (lli) is %lld\n", value_lli);
	printf("value (usi) is %u\n", value_ui);
   printf("value (uli) is %llu\n", value_uli);

   return 0;
}
