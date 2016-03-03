/***********************************************************/
/*(C) IBM Research - 2011                                  */
/*                                                         */
/* FFT Decimation in Time Radix-2 Implementation on MPI    */
/*                                                         */
/*   parFFTDitRadix2: parallel implementation of the 1D    */
/*   Decimation in Time Radix-2 FFT with twiddle factors   */
/*   on a sequence of complex numbers using MPI            */
/*							   */
/* n: cardinality of sequence complex numbers              */
/* (length of the transformation) , n must be a power of 2 */
/*                                                         */
/* x: vector of n complex numbers whose DFT will be        */
/*    calculated. The output is stored in-place, i.e. the  */
/*    pointer x will contain the frequency-domain          */
/*    components when the function returns.                */
/***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#if 0
#define DEBUG_L1 0
#define DEBUG_L1_PHASE3 0
#define DEBUG_L1_PHASE4 0
#define DEBUG_MPI_L1 0
#define DEBUG_MPI_L1_PHASE2
#define DEBUG_L1_PHASE4_RANKDATASWAP 0
#define DEBUG_L1_PHASE4_TWIDDLES
#define OUTPUT_FREQ_VECTOR
#define DEBUG_PHASE3_OUTPUT1
#define DEBUG_PHASE4_OUTPUT1
#define DEBUG_PHASE4_DATAMOVEMENT
#define DEBUG_PHASE4_BFLYDIRECTION
#endif
#define TIMING_MEASUREMENTS_ON
#if 0
#define PRINT_TIMING_MEASUREMENTS_RANKS
#define PRINT_TIMING_MEASUREMENTS_SUMMARY
#endif
#include "dft.h"
#include "timestats_structures.h"
#include "pfft_r2_dit.h"


/* Local functions */
static int validateFFT(int n,d_complex* fft_in,d_complex* x);

/* Constants */
#define TRUE 1
#define FALSE 0
#define EPSILON 0.0000001

time_stat* pfft_r2_dit(int N, int validation,int numranks,int myrank,int rc){
  int rankelems;
  int i,j,k,stage,strides_init,block_stride;
  double temp1,temp2,a,e,c,s,time1,time2,time3;
  double* x_real;
  double* x_imag;
  d_complex* x= NULL;
  d_complex* x_original= NULL;
  MPI_Status stat;  

  double* subvector_real;
  double* subvector_imag;
  d_complex t1;
  int n2;
  int n1;
  d_complex* subvector;
  double* subvector_real_send;
  double* subvector_imag_send;
  int rankblocks;
  int ranksperblock;
  int inblockstride;
  int dstrank;
  #ifdef DEBUG_L1_PHASE4_RANKDATASWAP
  int myblock;
  #endif
  int tempoff;
  int temp;
  double* FFT_x_real= NULL;
  double* FFT_x_imag= NULL;
  int validation_result;
 
  #ifdef TIMING_MEASUREMENTS_ON
  double phase1_compute_t =0.0;
  double phase2_compute_t =0.0;
  double phase3_compute_t =0.0;
  double phase4_compute_t =0.0;
  double phase5_compute_t =0.0;
  double phase1_comm_t =0.0;
  double phase2_comm_t =0.0;
  double phase3_comm_t =0.0;
  double phase4_comm_t =0.0;
  double phase5_comm_t =0.0;
  double total_t = 0.0;
  double validate_t = 0.0;
  #endif
  
  time_stat* measurements = (time_stat*)malloc(sizeof(time_stat));
  measurements->status=0;
  
  /* //////////////////////////////////////// */
  /* //// FFT hits the road */
  
  #ifdef TIMING_MEASUREMENTS_ON
  time3 = MPI_Wtime();
  #endif

  rankelems = N/numranks;
  
  subvector_real = (double*)malloc(rankelems * sizeof(double));
  if (subvector_real==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_real ...exiting\n");
    measurements->status=-1;
    return measurements;
  }
  subvector_imag = (double*)malloc(rankelems * sizeof(double));
  if (subvector_imag==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_imag ...exiting\n");
    measurements->status=-1;
    return measurements;
  }

/* ////////////// PHASE-0: Input Generation //////////////// */
/* ///////////////////////////////////////////////////////// */
  if (myrank==0){

    /* generate input vector */
    x = (d_complex*)malloc(N * sizeof(d_complex));
    if (x==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for DFT input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }
    rndComplexVector(N,x);

    #ifdef DEBUG_L1
    printf("Input Vector BEFORE bit-reversal\n");
    for (i=0;i<N;i++)
      printf("x[%d].Re=%f,x[%d].Im=%f\n",i,x[i].real,i,x[i].imag);
    printf("\n\n");
    #endif

    /* maintain an unchanged copy of the original input for validation purposes */
    x_original = (d_complex*)malloc(N * sizeof(d_complex));
    if (x_original==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for (original) input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }
    for (i=0;i<N;i++){
      x_original[i].real = x[i].real;
      x_original[i].imag = x[i].imag;
    }  
    
/* ////////////// PHASE-1: Bit-Reversal  // //////////////// */
/* ///////////////////////////////////////////////////////// */
 
    #ifdef TIMING_MEASUREMENTS_ON
    time1 = MPI_Wtime();
    #endif
    /* dummy complex */
    j = 0; /* bit-reverse */
    n2 = N/2;
    for (i=1; i < N - 1; i++)
    {
      n1 = n2;
      while ( j >= n1 )
      {
        j = j - n1;
        n1 = n1/2;
      }
      j = j + n1;

      if (i < j)
      {
        t1 = x[i];
        x[i] = x[j];
        x[j] = t1;
      }
    }

    #ifdef TIMING_MEASUREMENTS_ON
    phase1_compute_t += MPI_Wtime()-time1;
    #endif

    #ifdef DEBUG_L1
    printf("Input Vector AFTER bit-reversal\n");
    for (i=0;i<N;i++)
      printf("x[%d].Re=%f,x[%d].Im=%f\n",i,x[i].real,i,x[i].imag);
    printf("\n\n");
    #endif

/* ////////////// PHASE-2: Input Data Scattering ///////////// */
/* /////////////////////////////////////////////////////////// */

    /* scatter sub-vectors to ranks */
    /* re-format the input vector of complex numbers to two vectors containing doubles, */
    /* since native MPI Scatter works only for MPI native datatypes */
    x_real = (double*)malloc(N * sizeof(double));
    if (x_real==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for temp input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }

    x_imag = (double*)malloc(N * sizeof(double));
    if (x_imag==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for temp input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }
    for (i=0;i<N;i++){
      x_real[i] = x[i].real;
      x_imag[i] = x[i].imag;
    }
    
    #ifdef TIMING_MEASUREMENTS_ON
    time2 = MPI_Wtime();
    #endif
    
    MPI_Scatter(x_real,rankelems,MPI_DOUBLE,subvector_real,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);
    MPI_Scatter(x_imag,rankelems,MPI_DOUBLE,subvector_imag,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);
    
    #ifdef TIMING_MEASUREMENTS_ON
    phase2_comm_t += MPI_Wtime() - time2;
    #endif
  }
  else{

    x_real = (double*)malloc(N * sizeof(double));
    if (x_real==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for temp input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }

    x_imag = (double*)malloc(N * sizeof(double));
    if (x_imag==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for temp input vector...exiting\n");
      measurements->status=-1;
      return measurements;
    }

    #ifdef TIMING_MEASUREMENTS_ON
    time2 = MPI_Wtime();
    #endif

    MPI_Scatter(x_real,rankelems,MPI_DOUBLE,subvector_real,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);
    MPI_Scatter(x_imag,rankelems,MPI_DOUBLE,subvector_imag,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);

    #ifdef TIMING_MEASUREMENTS_ON
    phase2_comm_t += MPI_Wtime() - time2;
    #endif
  }

  /* reformat the subvectors from two separate double array to a single complex array */
  subvector = (d_complex*)malloc(rankelems * sizeof(d_complex));
  if (subvector==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvector...exiting\n");
    measurements->status=-1;
    return measurements;

  }
  for (i=0;i<rankelems;i++){
    subvector[i].real = subvector_real[i];
    subvector[i].imag = subvector_imag[i];
  }
  
  #ifdef DEBUG_MPI_L1_PHASE2
  if (myrank==3){
    printf ("I am rank = %d\n", myrank);
    for (i=0;i<rankelems;i++){
      printf("Local subvector[%d] = %f + %fj\n",i,subvector[i].real,subvector[i].imag);
    }
  }
  #endif

  free(subvector_real);
  free(subvector_imag);
  if (myrank==0){
    free(x_real);
    free(x_imag);
  }

/* ////////////// PHASE-3: Local Butterfly Stages /////////////////// */
/* ////////////////////////////////////////////////////////////////// */

  strides_init = 0;
  block_stride = 1;

  for (stage=0;stage<log_2(N)-log_2(numranks);stage++){
    #ifdef TIMING_MEASUREMENTS_ON
    time1 = MPI_Wtime();
    #endif

    strides_init=block_stride;
    block_stride*=2;
    e=(-1.0)*2.0*M_PI/(double)block_stride;
    a=0.0;

    #ifdef DEBUG_L1_PHASE3
    if (myrank==0){
      printf("---------------------------------------------------------------------------\n");
      printf("Stage-%d\n",stage);
      printf("Stride_init=%d, Block_stride = %d\n",strides_init,block_stride);
      printf("---------------------------------------------------------------------------\n");
    }
    #endif
    /* Flies with butter served per stage */
    for (j=0;j<strides_init;j++){
      /* compute twiddle factors */
      c=cos(a);
      s=sin(a);
      a+=e;
      
      for (k=j;k<rankelems;k=k+block_stride){
        #ifdef DEBUG_L1
        if (myrank==0){
          printf("Butterfly %d\t%d\n",k,k+strides_init);
        }
        #endif
        /* multiplying the second butterfly input with the twiddle factor */
        temp1 = c*subvector[k+strides_init].real-s*subvector[k+strides_init].imag;
        temp2 = s*subvector[k+strides_init].real+c*subvector[k+strides_init].imag;
        /* butterfly additions with in=place store */
        subvector[k+strides_init].real=subvector[k].real-temp1;
        subvector[k+strides_init].imag=subvector[k].imag-temp2;
        subvector[k].real=subvector[k].real+temp1;
        subvector[k].imag=subvector[k].imag+temp2;
      }
    }

    #ifdef TIMING_MEASUREMENTS_ON
    phase3_compute_t += MPI_Wtime()-time1;
    #endif    

    #ifdef DEBUG_PHASE3_OUTPUT1
    if (myrank==3){
      printf("SubVector at Rank-%d after Stage-%d:\n\n",myrank,stage);
      for (i=0;i<rankelems;i++)
        printf("%f+%fj\n",subvector[i].real,subvector[i].imag);
      printf("--------------------------------------------\n\n");
    }
    #endif
  }
  
/* ////////////// PHASE-4: Parallel Butterfly Stages //////////////// */
/* ////////////////////////////////////////////////////////////////// */

  /* Allocate send buffer vectors */
  subvector_real_send = (double*)malloc(rankelems * sizeof(double));
  if (subvector_real==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_real ...exiting\n");
    measurements->status=-1;
    return measurements;

  }
  subvector_imag_send = (double*)malloc(rankelems * sizeof(double));
  if (subvector_imag==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_imag ...exiting\n");
    measurements->status=-1;
    return measurements;

  }

  #ifdef TIMING_MEASUREMENTS_ON
  time1 = MPI_Wtime();
  #endif
  /* Allocate rcv buffer vectors */
  subvector_real = (double*)malloc(rankelems * sizeof(double));
  if (subvector_real==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_real ...exiting\n");
    measurements->status=-1;
    return measurements;
  }
  subvector_imag = (double*)malloc(rankelems * sizeof(double));
  if (subvector_imag==NULL){
    printf("Fatal Error in file mpi_FFT: Unable to allocate memory for subvecotr_imag ...exiting\n");
    measurements->status=-1;
    return measurements;
  }
  #ifdef TIMING_MEASUREMENTS_ON
  phase4_compute_t += MPI_Wtime()-time1;
  #endif

  for (stage=0;stage<log_2(numranks);stage++){
    
    strides_init=block_stride;
    block_stride*=2;
    e=(-1.0)*2.0*M_PI/(double)block_stride;
    a=0.0;

    rankblocks = numranks/(2<<stage);
    ranksperblock = numranks/rankblocks;
    inblockstride = ranksperblock/2;
     
    #ifdef DEBUG_L1_PHASE4_RANKDATASWAP
    myblock = ((myrank-(myrank%ranksperblock))/ranksperblock)+1;
    printf("---------------------------------------------------------------------------\n");
    printf("Stage-%d\n",stage+log_2(rankelems));
    printf("I am rank-%d, I belong to block %d \n",myrank, myblock);
    #endif
    if (myrank%ranksperblock<inblockstride){
      #ifdef DEBUG_L1_PHASE4_RANKDATASWAP
      printf("I am rank-%d, I will swap data with rank-%d \n",myrank, myrank+inblockstride);
      printf("I will do butterflies on following elements:\n");
      for (i=0;i<rankelems;i++)
        printf("Butterfly %d\t%d\n\n",(myrank*rankelems)+i,((myrank+inblockstride)*rankelems)+i); 
      #endif
      dstrank = myrank+inblockstride;
    }else{
      #ifdef DEBUG_L1_PHASE4_RANKDATASWAP
      printf("I am rank-%d, I will swap data with rank-%d \n",myrank, myrank-inblockstride);
      printf("I will do butterflies on following elements:\n");
      for (i=0;i<rankelems;i++)
        printf("Butterfly %d\t%d\n\n",(myrank*rankelems)+i,((myrank-inblockstride)*rankelems)+i); 
      #endif
      dstrank = myrank-inblockstride;
    }
    #ifdef DEBUG_L1_PHASE4_RANKDATASWAP
    printf("---------------------------------------------------------------------------\n");
    #endif
 
    /* reformat data for sending */
    for (i=0;i<rankelems;i++){
      subvector_real_send[i]=subvector[i].real;
      subvector_imag_send[i]=subvector[i].imag;
    }
 
    #ifdef TIMING_MEASUREMENTS_ON
    time2 = MPI_Wtime();
    #endif

    /* ///// PER STAGE COMM */
    MPI_Sendrecv (subvector_real_send,rankelems,MPI_DOUBLE,dstrank,dstrank,subvector_real,rankelems,MPI_DOUBLE,dstrank,myrank,MPI_COMM_WORLD,&stat);
    MPI_Sendrecv (subvector_imag_send,rankelems,MPI_DOUBLE,dstrank,dstrank+1,subvector_imag,rankelems,MPI_DOUBLE,dstrank,myrank+1,MPI_COMM_WORLD,&stat);
    /* ///// END PER STAGE COMM   */

    #ifdef TIMING_MEASUREMENTS_ON
    phase4_comm_t += MPI_Wtime() - time2;
    #endif

    #ifdef DEBUG_PHASE4_DATAMOVEMENT
    printf("--------------------------------------------\n");
    if (myrank==1){
      printf("Subvector Received by rank-%d from rank-%d in STAGE-%d\n",myrank,dstrank,stage+log_2(rankelems));
      for (i=0;i<rankelems;i++)
        printf("%f+%fj\n",subvector_real[i],subvector_imag[i]);
      printf("--------------------------------------------\n");
      printf("Local Input Subvector at rank-%d in STAGE-%d\n",myrank,stage+log_2(rankelems));
      for (i=0;i<rankelems;i++)
        printf("%f+%fj\n",subvector[i],subvector[i]);
      printf("--------------------------------------------\n");
    }
    #endif  
    

    /* Now you have your data, do your butterfly part */
    
    #ifdef TIMING_MEASUREMENTS_ON
    time1 = MPI_Wtime();
    #endif
    
    tempoff =myrank%(ranksperblock/2);
    temp = myrank%ranksperblock;
    a = a+ tempoff*rankelems*e;
    for (i=0;i<rankelems;i++){
       /* a = (double)factor_offset*e; */
       c = cos(a);
       s = sin(a);
       a += e;
       /* which side are you on boys? */
       #ifdef DEBUG_L1_PHASE4_TWIDDLES
       printf("Rank-%d is doing butterflies with rank-%d in STAGE-%d\n",myrank,dstrank,stage+log_2(rankelems));
       #endif
       if (temp<ranksperblock/2){
         temp1 = c*subvector_real[i] - s*subvector_imag[i];
         temp2 = s*subvector_real[i] + c*subvector_imag[i];
         subvector[i].real = subvector[i].real+temp1;
         subvector[i].imag = subvector[i].imag+temp2;
         #ifdef DEBUG_PHASE4_BFLYDIRECTION
         printf("Rank-%d in STAGE-%d IS TAKING UP DIRECTION\n",myrank,stage+log_2(rankelems));
         #endif
         #ifdef DEBUG_L1_PHASE4_TWIDDLES         
         printf("Butterfly %d\t%d\nTwiddle factor = %f\n",(myrank*rankelems)+i,(dstrank*rankelems)+i,a/e);
         #endif
       }else{
         temp1 = c*subvector[i].real - s*subvector[i].imag;
         temp2 = s*subvector[i].real + c*subvector[i].imag;
         subvector[i].real = subvector_real[i]-temp1;
         subvector[i].imag = subvector_imag[i]-temp2;     
         /* subvector[i].real = subvector_real[i] - c*subvector[i].real + s*subvector[i].imag; */
         /* subvector[i].imag = subvector_imag[i] - s*subvector[i].real - c*subvector[i].imag; */
         #ifdef DEBUG_PHASE4_BFLYDIRECTION
         printf("Rank-%d in STAGE-%d IS TAKING DOWN DIRECTION\n",myrank,stage+log_2(rankelems));
         #endif
         #ifdef DEBUG_L1_PHASE4_TWIDDLES         
         printf("Butterfly %d\t%d\nTwiddle factor = %f\n",(myrank*rankelems)+i,(dstrank*rankelems)+i,a/e);
         #endif
       }
    }
    #ifdef TIMING_MEASUREMENTS_ON
    phase4_compute_t += MPI_Wtime()-time1;
    #endif
    
    #ifdef DEBUG_PHASE4_OUTPUT1
    if (myrank==1){
      printf("SubVector at Rank-%d after Stage-%d:\n\n",myrank,stage+log_2(rankelems));
      for (i=0;i<rankelems;i++)
        printf("%f+%fj\n",subvector[i].real,subvector[i].imag);
      printf("--------------------------------------------\n\n");
    }
    #endif
  }

/* ////////////// PHASE-5: Gathering Output to the Master ////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */

  if (myrank==0){
    /* allocate real and imaginary vectors at the master to hold the FFT output */
    FFT_x_real = (double*)malloc(N * sizeof(double));
    if (FFT_x_real==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for FFT_x_real ...exiting\n");
      measurements->status=-1;
      return measurements;
    }

    FFT_x_imag = (double*)malloc(N * sizeof(double));
    if (FFT_x_imag==NULL){
      printf("Fatal Error in file mpi_FFT: Unable to allocate memory for FFT_x_imag ...exiting\n");
      measurements->status=-1;
      return measurements;
    }
  }

  /* reformat the complex subvectors to be gathered as MPI_DOUBLED */
  for (i=0;i<rankelems;i++){
    subvector_real[i]=subvector[i].real;
    subvector_imag[i]=subvector[i].imag;
  }

  #ifdef TIMING_MEASUREMENTS_ON
  time2 = MPI_Wtime();
  #endif

  MPI_Gather(subvector_real,rankelems,MPI_DOUBLE,FFT_x_real,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Gather(subvector_imag,rankelems,MPI_DOUBLE,FFT_x_imag,rankelems,MPI_DOUBLE,0,MPI_COMM_WORLD);

  #ifdef TIMING_MEASUREMENTS_ON
  phase5_comm_t += MPI_Wtime() - time2;
  #endif

  free(subvector_real);
  free(subvector_imag);
  
  #ifdef TIMING_MEASUREMENTS_ON
  total_t = MPI_Wtime()-time3;
  time2=MPI_Wtime();
  #endif

/* ///////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */

  #ifdef TIMING_MEASUREMENTS_ON
  /* Reduce Time statistics and compute averages */
  MPI_Reduce(&total_t,&measurements->avg_tot_time,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&total_t,&measurements->min_tot_time,1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&total_t,&measurements->max_tot_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_compute_t,&measurements->avgcompute_time[0],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_compute_t,&measurements->avgcompute_time[1],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_compute_t,&measurements->avgcompute_time[2],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_compute_t,&measurements->avgcompute_time[3],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_compute_t,&measurements->avgcompute_time[4],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_compute_t,&measurements->mincompute_time[0],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_compute_t,&measurements->mincompute_time[1],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_compute_t,&measurements->mincompute_time[2],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_compute_t,&measurements->mincompute_time[3],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_compute_t,&measurements->mincompute_time[4],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_compute_t,&measurements->maxcompute_time[0],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_compute_t,&measurements->maxcompute_time[1],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_compute_t,&measurements->maxcompute_time[2],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_compute_t,&measurements->maxcompute_time[3],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_compute_t,&measurements->maxcompute_time[4],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_comm_t,&measurements->avgcomm_time[0],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_comm_t,&measurements->avgcomm_time[1],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_comm_t,&measurements->avgcomm_time[2],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_comm_t,&measurements->avgcomm_time[3],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_comm_t,&measurements->avgcomm_time[4],1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_comm_t,&measurements->mincomm_time[0],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_comm_t,&measurements->mincomm_time[1],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_comm_t,&measurements->mincomm_time[2],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_comm_t,&measurements->mincomm_time[3],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_comm_t,&measurements->mincomm_time[4],1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);

  MPI_Reduce(&phase1_comm_t,&measurements->maxcomm_time[0],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase2_comm_t,&measurements->maxcomm_time[1],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase3_comm_t,&measurements->maxcomm_time[2],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase4_comm_t,&measurements->maxcomm_time[3],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  MPI_Reduce(&phase5_comm_t,&measurements->maxcomm_time[4],1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);

  if (myrank==0){
    measurements->avg_tot_time /= numranks;
    for (i=0;i<5;i++){
      measurements->avgcompute_time[i] /= numranks;
      measurements->avgcomm_time[i] /= numranks;
    }
  }
  #endif
  
  #ifdef TIMING_MEASUREMENTS_ON
  #ifdef PRINT_TIMING_MEASUREMENTS_SUMMARY
  if (myrank==0){
    printf("Avg Total FFT Time over %d Ranks = %f\n",numranks,measurements->avg_tot_time);
    printf("Min Total FFT Time over %d Ranks = %f\n",numranks,measurements->min_tot_time);
    printf("Max Total FFT Time over %d Ranks = %f\n",numranks,measurements->max_tot_time);
    for (i=0;i<5;i++){
      printf("----------------------------------------------------------------------------------------------\n");
      printf("Avg Compute Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->avgcompute_time[i]);
      printf("Max Compute Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->maxcompute_time[i]);
      printf("Min Compute Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->mincompute_time[i]);
      printf("**********************************************************************************************\n");
      printf("Avg Communication Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->avgcomm_time[i]);
      printf("Max Communication Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->maxcomm_time[i]);
      printf("Min Communication Time in Phase-%d over %d Ranks = %f\n",i+1,numranks,measurements->mincomm_time[i]);
      printf("----------------------------------------------------------------------------------------------\n");
    }
  }
  #endif
  #endif

  if (myrank==0){
    #ifdef OUTPUT_FREQ_VECTOR
    printf("\n***************\n***************\n\n\nPARALLEL FFT Transform Output:\n\n");
    for (i=0;i<N;i++){
      printf("%f+%fj\n",FFT_x_real[i],FFT_x_imag[i]);
    }
    #endif

    /* validate the result */
    if (validation==1){
      d_complex* FFT_complex=(d_complex*)malloc(N * sizeof(d_complex));
      for (i=0;i<N;i++){
        FFT_complex[i].real = FFT_x_real[i];
        FFT_complex[i].imag = FFT_x_imag[i];
      }

      validation_result = validateFFT(N,FFT_complex,x_original);
      if (validation_result == TRUE)
        printf("FFT is correct\n\n");
      else
        printf("FFT implementation IS NOT ACCURATE");
      free(FFT_complex);
    }
    free(FFT_x_real);
    free(FFT_x_imag);
    free(x);free(x_original);
  }
  
  #ifdef TIMING_MEASUREMENTS_ON
  validate_t = MPI_Wtime()-time2;
  #ifdef PRINT_TIMING_MEASUREMENTS_RANKS
  printf("\n-------------------------------------------------------------------------------\n");
  printf("Total time spent in PFFT=%f sec\n", total_t);
  if (validation==1)
    printf("Total time spent in DFT=%f sec\n", validate_t);
  printf("Rank-%d Computation Timings:\n",myrank);
  printf("Phase-1:%f\n",phase1_compute_t);
  printf("Phase-2:%f\n",phase2_compute_t);
  printf("Phase-3:%f\n",phase3_compute_t);
  printf("Phase-4:%f\n",phase4_compute_t);
  printf("Phase-5:%f\n",phase5_compute_t);
  printf("Rank-%d Communication Timings:\n",myrank);
  printf("Phase-1:%f\n",phase1_comm_t);
  printf("Phase-2:%f\n",phase2_comm_t);
  printf("Phase-3:%f\n",phase3_comm_t);
  printf("Phase-4:%f\n",phase4_comm_t);
  printf("Phase-5:%f\n",phase5_comm_t);
  printf("\-------------------------------------------------------------------------------\n");
  #endif
  #endif

  return measurements;
}

/**
 * Computes the O(N^2) expensive DFT of the input vector
 * and then compares the output with the provided input vector
 * fft_in, which is supposed to comprise the frequency-domain
 * components of the time-domain input vector as computed
 * by the FFT implementation under test.
 * If at least one component (real or imaginary part) of the
 * difference is found to be non-zero, the function returns FALSE,
 * otherwise it returns TRUE.
 */
static int validateFFT(int n,d_complex* fft_in,d_complex* x){
    int i;

  /* perform DFT */
  d_complex* z = (d_complex*)malloc(n * sizeof(d_complex));
  if (z==NULL){
    printf("Fatal Error in function validateFFT: Unable to allocate memory for DFT output vector...exiting\n");
    return FALSE;
  }
  i=0;

  serialDFT(n,x,z);

  #ifdef DEBUG_L1
  printf("DFT Transform Input:\n");
  for (i=0;i<n;i++){
    printf("%f+%fj\n",x[i].real,x[i].imag);
  }
  printf("\n\nDFT Transform Output:\n");
  for (i=0;i<n;i++){
    printf("%f+%fj\n",z[i].real,z[i].imag);
  }
  #endif

  /* calculate diff between DFT and FFTint ret = serialDFT(n,x,z); */
  for (i=0;i<n;i++)
    if (((fft_in[i].real - z[i].real) > EPSILON ) || ((fft_in[i].imag - z[i].imag) > EPSILON ))
      return FALSE;
  
  return TRUE;
}

void usage(char *pname){
  fprintf(stderr,"\n-----------------------------------------------------------------------------------------------------------------------------------\n");
  fprintf(stderr,"Usage: %s -s len [-v 1|0]\n",pname);
  fprintf(stderr,"	-s len	        Length of the (random) vector whose FFT will be calculation (problem size)\n");
  fprintf(stderr,"		        Length of the vector must be a power of 2.\n");
  fprintf(stderr,"	-v 0|1	        If 1, the (sequential) DFT of the input will be computed and used to validate the correctness of parallel FFT.\n");
  fprintf(stderr,"			It is recommended to omit validation for large problem sizes.\n");
  fprintf(stderr,"\nNote: The number of MPI ranks must be a power of 2 and at most half the problem size!\n");
  fprintf(stderr,"------------------------------------------------------------------------------------------------------------------------------------\n\n");
}
