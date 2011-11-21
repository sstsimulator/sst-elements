/****************************************************
 * (C) IBM Research - 2011
 *
 * (Sequential) Complex Discrete Fourier Transform
 *
 * This code is just to validate the correctness of FFT
 * implementations. Alternatively, one can use IFFT.
 * 
 */
 
 #include "dft.h"

 #ifndef INCL_STD_LIB
 #include <stdlib.h>
 #define INCL_STD_LIB
 #endif


 #ifndef INCL_TIME
 #include <time.h>
 #define INCL_TIME
 #endif
 
 /*
  * * Sequential DFT of a sequence of double precision complex numbers of
  * * length n.
  * * n: cardinality of sequence complex numbers (length of the transformation)
  * * x: vector of n complex numbers whose DFT will be calculated
  * * y: vector of n complex containing the DFT of the input sequence
  * * 
  * * Returns 0 if no error occured and -1 otherwise.
  */
 int serialDFT(int n, d_complex* x, d_complex* z){
	int k,i;
	double common_phase;
	
	for (k=0;k<n;k++){
		/* initialize output vector element */
		z[k].real = 0;
		z[k].imag = 0;
		/* part of the phase is common for each element, so compute once for each element */
		common_phase = (-1) * 2.0 * M_PI * (double)k / (double)n;
		
		/* sum of n weighted roots of unity */
		for (i=0;i<n;i++){
			z[k].real += x[i].real*cos(common_phase*i) - x[i].imag*sin(common_phase*i);
			z[k].imag += x[i].real*sin(common_phase*i) + x[i].imag*cos(common_phase*i);
		}	
	}
	return 0;
 }

int log_2(int n){
  int log_2_n=0;
  double temp_n=(double)n;
  while (temp_n>1.0){
    temp_n = temp_n/2;
    log_2_n++;
  }
  if (temp_n<1.0)
    return(-1);
  return log_2_n;
}

void rndComplexVector(int n, d_complex* vector){
  int i;
  double maxampl = 10.0;
  srand((unsigned)(time(0)));
  for (i=0;i<n;i++){
    vector[i].real = ( rand() / (double)(RAND_MAX) ) * maxampl;
    vector[i].imag = ( rand() / (double)(RAND_MAX) ) * maxampl;
  }
}


