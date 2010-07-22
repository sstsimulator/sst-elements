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

#include "stdio.h"
#ifndef USING_MPI
# define MPI_Comm_rank(a,b) (*(b)) = 0
# define MPI_Comm_size(a,b) (*(b)) = 1
# define MPI_Allreduce(a,b,c,d,e,f) do { \
    if (c==1) { (*(b)) = (*(a)); } else { \
	for(size_t i=0;i<c;++i) { (b)[i] = (a)[i]; } } } while (0)
#else
#include "mpi.h"
#endif

#include "ljs.h"
#include "atom.h"
#include "force.h"
#include "neighbor.h"
#include "integrate.h"
#include "thermo.h"

#define MAXLINE 256

int input(In &in, Atom &atom, Force &force, Neighbor &neighbor, 
	  Integrate &integrate, Thermo &thermo)
{
  FILE *fp;
  int flag;
  char line[MAXLINE];

  int me;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);

  if (me == 0) {
    fp = fopen("lj.in","r");
    if (fp == NULL) { flag = 0; perror("fopen"); }
    else flag = 1;
  }
#ifdef USING_MPI
  MPI_Bcast(&flag,1,MPI_INT,0,MPI_COMM_WORLD);
#endif
  if (flag == 0) {
    if (me == 0) printf("ERROR: Cannot open lj.in\n");
    return 1;
  }

  if (me == 0) {
    fgets(line,MAXLINE,fp);
    fgets(line,MAXLINE,fp);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%d %d %d",&in.nx,&in.ny,&in.nz);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%d",&integrate.ntimes);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%d %d %d",&neighbor.nbinx,&neighbor.nbiny,&neighbor.nbinz);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%lg",&integrate.dt);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%lg",&in.t_request);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%lg",&in.rho);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%d",&neighbor.every);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%lg %lg",&force.cutforce,&neighbor.cutneigh);
    fgets(line,MAXLINE,fp);
    sscanf(line,"%d",&thermo.nstat);
    fclose(fp);
  }

#ifdef USING_MPI
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Bcast(&integrate.dt,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Bcast(&in.t_request,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Bcast(&in.rho,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Bcast(&neighbor.every,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&force.cutforce,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Bcast(&neighbor.cutneigh,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  MPI_Bcast(&thermo.nstat,1,MPI_INT,0,MPI_COMM_WORLD);

  MPI_Bcast(&in.nx,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&in.ny,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&in.nz,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&integrate.ntimes,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&neighbor.nbinx,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&neighbor.nbiny,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&neighbor.nbinz,1,MPI_INT,0,MPI_COMM_WORLD);
#endif

  return 0;
}
