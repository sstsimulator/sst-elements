/* ----------------------------------------------------------------------
   miniMD is a simple, parallel molecular dynamics (MD) code.   miniMD is
   an MD microapplication in the Mantevo project at Sandia National 
   Laboratories ( http://software.sandia.gov/mantevo/ ). The primary 
   authors of miniMD are Steve Plimpton and Paul Crozier 
   (pscrozi@sandia.gov).

   Copyright (2008) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This library is free software; you 
   can redistribute it and/or modify it under the terms of the GNU Lesser 
   General Public License as published by the Free Software Foundation; 
   either version 3 of the License, or (at your option) any later 
   version.
  
   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
   Lesser General Public License for more details.
    
   You should have received a copy of the GNU Lesser General Public 
   License along with this software; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.  See also: http://www.gnu.org/licenses/lgpl.txt .

   For questions, contact Paul S. Crozier (pscrozi@sandia.gov). 

   Please read the accompanying README and LICENSE files.
---------------------------------------------------------------------- */

#include "stdlib.h"
#ifndef USING_MPI
# define MPI_Comm_rank(a,b) (*(b)) = 0
# define MPI_Comm_size(a,b) (*(b)) = 1
# define MPI_Barrier(a)
# define MPI_Allreduce(a,b,c,d,e,f) do { \
    if (c==1) { (*(b)) = (*(a)); } else { \
	for(size_t i=0;i<c;++i) { (b)[i] = (a)[i]; } } } while (0)
#include <sys/time.h>
#else
#include "mpi.h"
#endif
#include "timer.h"

Timer::Timer()
{
  array = (double *) malloc(TIME_N*sizeof(double));
  for (int i = 0; i < TIME_N; i++) array[i] = 0.0;
}

Timer::~Timer()
{
  if (array) free(array);
}

void Timer::stamp()
{
#ifdef USING_MPI
  previous_time = MPI_Wtime();
#else
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  previous_time = stamp.tv_sec + stamp.tv_usec*1e-6;
#endif
}

void Timer::stamp(int which)
{
#ifdef USING_MPI
  double current_time = MPI_Wtime();
#else
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  double current_time = stamp.tv_sec + stamp.tv_usec*1e-6;
#endif
  array[which] += current_time - previous_time;
  previous_time = current_time;
}

void Timer::barrier_start(int which)
{
#ifdef USING_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  array[which] = MPI_Wtime();
#else
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  array[which] = stamp.tv_sec + stamp.tv_usec*1e-6;
#endif
}

void Timer::barrier_stop(int which)
{
#ifdef USING_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  double current_time = MPI_Wtime();
#else
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  double current_time = stamp.tv_sec + stamp.tv_usec*1e-6;
#endif
  array[which] = current_time - array[which];
}
