/****************************************************
 * (C) IBM Research - 2011
 *
 * (Sequential) Complex Discrete Fourier Transform
 *
 * This code is just to validate the correctness of FFT
 * implementations. Alternatively, one can use IFFT.
 * 
 */
 
 #ifndef INCL_STD_MATH
 #include <math.h>
 #define INCL_STD_MATH
 #endif
 
 /*
  * data structure representing a complex number with double
  * precision components
  */
 typedef struct {
	double real;
	double imag;
 } d_complex;
 
 /*
  *  Sequential DFT of a sequence of double precision complex numbers of
  *  length n.
  */
 int serialDFT(int, d_complex*, d_complex*);

/**
 * Compute the base-2 logarithm of an integeer.
 * Returns the logarithm if the input integer is indeed
 * a power of 2 or else -1.
 */
int log_2(int n);

/**
 * Generate N complex numbers and assign them to the supplied input vector.
 * The function assumes that memory for N complex number starting from the
 * input pointer x has already been allocated by the caller.
 */
void rndComplexVector(int, d_complex*);
