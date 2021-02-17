#include <stdio.h>

float squareRoot(float n) {
	float i = 0;
	float precision = 0.00001;

	for(i = 1; i*i <=n; ++i);
  	for(--i; i*i < n; i += precision);

   	return i;
}

int main( int argc, char* argv[] ) {
   	int n = 24;

	for( int i = 1; i < n; ++i ) {
		printf("Square root of %d = %30.12f\n", i,
			squareRoot( (float) i));
		fflush(stdout);
	}

	return 0;
}
