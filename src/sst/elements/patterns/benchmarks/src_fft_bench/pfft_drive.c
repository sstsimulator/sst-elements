/*
** Copyright (c) 2011, IBM Corporation
** All rights reserved.
**
** Code for driving multiple runs of the Parallel FFT benchark
** using various problem sizes.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>
#include <ctype.h>
#include "dft.h"
#include "timestats_structures.h"
#include "pfft_r2_dit.h"
#include "benchmarks/stat_p.h"


/* Constants */
#define FALSE			(0)
#define TRUE			(1)
#define DEFAULT_NUM_OPS		(200)
#define DEFAULT_NUM_SETS	(9)
#define DEFAULT_PRECISION	(0.01)

void usage_drive(char *pname);



int main(int argc, char* argv[]){

  int actual_char,num_sets=50,myrank,numranks;
  double req_precision= DEFAULT_PRECISION;
  int N =-1;
  int validation = 0;
  int printstats = 0;
  int Nmin =-1;
  int Nsamples = 4;
  int Nstep = 1;

  int j,sample;
  double metric=0,tot=0,tot_squared=0,precision;
  int ii=0;
  int aborttrials;
  double avg_compute_time[5]={0,0,0,0,0};
  double min_compute_time[5]={1000,1000,1000,1000,1000};
  double max_compute_time[5]={-1000,-1000,-1000,-1000,-1000};
  double avg_comm_time[5]={0,0,0,0,0};
  double min_comm_time[5]={1000,1000,1000,1000,1000};
  double max_comm_time[5]={-1000,-1000,-1000,-1000,-1000};
  
  FILE *fp;
  char fname[64];

  /* ///////////////////////////////////////////////////////// */
  /* // Initialize MPI Env */
  int rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS) {
    printf ("Fatal Error :unable to initialize the MPI environemnt. Terminating....\n");
    MPI_Finalize();
    return(-1);
  }
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  MPI_Comm_size(MPI_COMM_WORLD,&numranks);
  
  opterr = 0;

  /* ///////////////////////////////////////////////////////// */
  /* // Process Input Args */
 
  while ((actual_char = getopt (argc, argv, "hf:s:vrp:a:b:c:")) != -1)
    switch (actual_char){
      case 'h':
        if (myrank==0)
          usage_drive(argv[0]);
        MPI_Finalize();
        return(1);
        break;
      case 'f':
        N = strtol(optarg, (char **)NULL, 0);
        break;
      case 'a':
        Nmin = strtol(optarg, (char **)NULL, 0);
        break;
      case 'b':
        Nsamples = strtol(optarg, (char **)NULL, 0);
        break;
      case 'c':
        Nstep = strtol(optarg, (char **)NULL, 0);
        break;
      case 'v':
        validation = 1;
        break;
      case 'r':
        printstats = 1;
        break;
      case 's':
	num_sets= strtol(optarg, (char **)NULL, 0);
	if (num_sets < 1)   {
	    if (myrank == 0)   {
		fprintf(stderr, "Number of sets (-s) must be > 0!\n");
	    }
            MPI_Finalize();
            return -1;
	}
     	break;
      case 'p':
	req_precision= strtod(optarg, (char **)NULL);
	if (myrank == 0)   {
	    printf("# req_precision is %f\n", req_precision);
	}
	break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else
          if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
            fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        MPI_Finalize();   
        return -1;
       default:
         usage_drive(argv[0]);
         fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
         MPI_Finalize();
         return -1;
    }

  /* //////////////////////////////////////////////////////////////////// */
  /* // Input checks */

  if (N==-1 && Nmin==-1){
    if (myrank==0){
      fprintf(stderr,"Problem size parameter has not been specified. Check usage to specify FFT size...terminating\n");
      usage_drive(argv[0]);
    }
    MPI_Finalize();
    return -1;
  }

  if (Nmin!=-1){
    /* Check that problem size is a power of 2 */
    if (log_2(Nmin)==-1){
      if (myrank==0){
        printf("Fatal Error in file mpi_fft.c: Size of Input sequence must be a power of 2...exiting\n");
        usage_drive(argv[0]);
      }
    MPI_Finalize();
    return(-1);
    }
    if (myrank==0){
      printf("Running a series of %d FFT runs sizes ranging from Nstart = %d with uniform multiplicative step 2^%d\n",Nsamples,Nmin,Nstep);
      if (validation==0)
        printf("FFT Validation is OFF\n");
      else
        printf("FFT Validation is ON (Warning: Lengthy runs)\n");
    }
  }

  if (N!=-1){ 
    /* Check that problem size is a power of 2 */
    if (log_2(N)==-1){
      if (myrank==0){
        printf("Fatal Error in file mpi_fft.c: Size of Input sequence must be a power of 2...exiting\n");
        usage_drive(argv[0]);
      }
    MPI_Finalize();
    return(-1);
    }
    Nstep=1;
    Nsamples=1;
    Nmin = N;
    if (myrank==0){
      printf("Running single FFT size run with input vector size N = %d\n",N);
      if (validation==0)
        printf("FFT Validation is OFF\n");
      else
        printf("FFT Validation is ON (Warning: Lengthy runs)\n");
      }
  }

  if (Nmin!=-1)
    N = Nmin;

  /* Check that #ranks is a power of 2 */
  if (log_2(numranks)==-1){
    if (myrank==0){
      printf ("\nInput Error: Number of ranks must a power of two. Terminating....\n\n\n");
      usage(argv[0]);
    }
    MPI_Finalize();
    return(-1);
  }

  /* Check that #ranks is at most half the input size */
  if (numranks > N/2){
    if (myrank==0)
      printf ("\nInput Error: Number of ranks must be at most half the problem size. Please decrease the number of ranks. Terminating....\n\n\n");
    MPI_Finalize();
    return(-1);
  }


  /* //////////////////////////////////////////////////// */
  /* // Fire FFT(s) */
  if (myrank==0){
    sprintf(fname,"FFT_results_%d_cores_precision_%f",numranks,req_precision);
    if ((fp = fopen(fname, "w")) != NULL){
       fprintf(fp, "#Shamrock, %d Cores, Precision=%f\n#\n",numranks,req_precision);
       fprintf(fp, "#Size\tAvgT\tP1CoMP\tP2CoMM\tP3CoMP\tP4CoMP\tP4CoMM\tP5CoMM\n");
       fclose(fp);
    }
  }
  
  for (sample=0;sample<Nsamples;sample++){
    int fftsize = Nmin*((2<<(Nstep*sample)>>1));
    if (myrank==0)
      printf("Running trial(s) for FFT size N = %d........ \n\n",fftsize);
    if (myrank==0){
      if (printstats)
        printf("Trial\tAvg Total\tMin Total\tMax Total\tPh1 CoMP Avg\tPh2 CoMM Avg\tPh3 CoMP Avg\tPh4 CoMP Avg\tPh4 CoMM Avg\tPh5 CoMM Avg\tPrecision\n");
      }  
    /* for each problem size run multiple trials until you reach statistical convergence */
    aborttrials=0;
    ii=0;
    metric = tot = tot_squared = precision = 0;
    for (j=0;j<5;j++){
        avg_compute_time[j]=0;
        avg_comm_time[j]=0;
        min_compute_time[j]=1000;
        max_compute_time[j]=-1000;
        min_comm_time[j]=1000;
        max_comm_time[j]=-1000;
    }
    while (ii<num_sets){
      time_stat* stats = pfft_r2_dit(fftsize,validation,numranks,myrank,rc);
      metric = stats->avg_tot_time;
      tot= tot + metric;
      tot_squared= tot_squared + metric*metric;
      precision= stat_p(ii + 1, tot, tot_squared);
      for (j=0;j<5;j++){
        avg_compute_time[j]+=stats->avgcompute_time[j];
        avg_comm_time[j]+=stats->avgcomm_time[j];
        if (min_compute_time[j]>stats->mincompute_time[j])
          min_compute_time[j]=stats->mincompute_time[j];
        if (min_comm_time[j]>stats->mincomm_time[j])
          min_comm_time[j]=stats->mincomm_time[j];
        if (max_compute_time[j]<stats->maxcompute_time[j])
          max_compute_time[j]=stats->maxcompute_time[j];
        if (max_comm_time[j]<stats->maxcomm_time[j])
          max_comm_time[j]=stats->maxcomm_time[j];
      }
      if (stats->status != -1){
        if (myrank==0){
          if (printstats){
            printf("%d\t",ii);
            /* printf("%f\t%f\t%f\t",stats->avg_tot_time,stats->min_tot_time,stats->max_tot_time,stats); */
            printf("%f\t%f\t%f\t", stats->avg_tot_time, stats->min_tot_time, stats->max_tot_time);
            printf("%f\t",stats->avgcompute_time[0]);
            printf("%f\t",stats->avgcomm_time[1]);
            printf("%f\t",stats->avgcompute_time[2]);
            printf("%f\t",stats->avgcompute_time[3]);
            printf("%f\t",stats->avgcomm_time[3]);
            printf("%f\t",stats->avgcomm_time[4]);
            printf("%f",precision);
            printf("\n");
          }
          else{
            /* printf("%d\t",ii); */
            /* printf("%f",precision); */
            /* printf("\n"); */
          }
        }
      }
      else{
        fprintf(stderr,"pfft returned an abnormal status....exiting");
        exit(-1);
      }
      if (myrank==0)
        if ( ii > 10 && precision <= req_precision){
          aborttrials=1;
          if ((fp = fopen(fname, "a+")) != NULL){
            ++ii;
            fprintf(fp, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",fftsize,tot/ii,avg_compute_time[0]/ii,avg_comm_time[1]/ii,avg_compute_time[2]/ii,avg_compute_time[3]/ii,avg_comm_time[3]/ii,avg_comm_time[4]/ii);
            fclose(fp);
          }
        }
        MPI_Bcast (&aborttrials,1,MPI_INT,0,MPI_COMM_WORLD);
      if (aborttrials==1)
        ii=num_sets;
      ii++;
    }
  }
  MPI_Finalize();
  return 0;
}

void usage_drive(char *pname){
  fprintf(stderr,"\n-----------------------------------------------------------------------------------------------------------------------------------\n");
  fprintf(stderr,"Usage: %s -f len [-v 1|0] [-h] [-p precision] [-s maxtrials] [-r 0|1] \n",pname);
  fprintf(stderr,"      -f len          Length of the (random) vector whose FFT will be calculated (problem size)\n");
  fprintf(stderr,"                      Length of the vector must be a power of 2.\n");
  fprintf(stderr,"      -v 0|1          If 1, the (sequential) DFT of the input will be computed and used to validate the correctness of parallel FFT.\n");
  fprintf(stderr,"                      It is recommended to omit validation for large problem sizes.Default is OFF\n");
  fprintf(stderr,"      -h              Print this help message.\n");
  fprintf(stderr,"      -p precision    Desired precision to be achieved within maxtrials independent FFT runs (desired range: 0.1 < p < 0).\n");
  fprintf(stderr,"      -s maxtrials    Execution of FFT runs will terminate after this cumulative number of trials even if statistical convergence\n");
  fprintf(stderr,"                      has not been achieved.Default is 50.\n");
  fprintf(stderr,"      -r 0|1          Set to 1 if live printing of time stats is desired. Default is OFF.\n\n\n");
  fprintf(stderr,"Multiple Sizes Usage: %s -a startlen [-b samples] [-c step] [-v 1|0] [-h] [-p precision] [-s maxtrials] [-r 0|1] \n",pname);
  fprintf(stderr,"      -a startlen     Initial Length of the (random) vector whose FFT will be calculated (problem size)\n");
  fprintf(stderr,"                      Length of the vector must be a power of 2.\n");
  fprintf(stderr,"      -b samples      Number of distinct FFT problem sizes to be run in the series of various size runs. Default is 10.\n");
  fprintf(stderr,"      -c step         Problem sizes in the series are formed starting with \"startlen\" and advancing by multiplying former size with 2^step.\n");
  fprintf(stderr,"                      Default step is 1.\n");
  fprintf(stderr,"      -v 0|1          If 1, the (sequential) DFT of the input will be computed and used to validate the correctness of parallel FFT.\n");
  fprintf(stderr,"                      It is recommended to omit validation for large problem sizes.Default is OFF\n");
  fprintf(stderr,"      -h              Print this help message.\n");
  fprintf(stderr,"      -p precision    Desired precision to be achieved within maxtrials independent FFT runs (desired range: 0.1 < p < 0).\n");
  fprintf(stderr,"      -s maxtrials    Execution of FFT runs will terminate after this cumulative number of trials even if statistical convergence\n");
  fprintf(stderr,"                      has not been achieved.Default is 50.\n");
  fprintf(stderr,"      -r 0|1          Set to 1 if live printing of time stats is desired. Default is OFF.\n");
  fprintf(stderr,"\nNote: The number of MPI ranks must be a power of 2 and at most half the problem size!\n");
  fprintf(stderr,"------------------------------------------------------------------------------------------------------------------------------------\n\n");
}
