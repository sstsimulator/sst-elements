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
#include "stdio.h"
#include "math.h"
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
#include "integrate.h"
#include "force.h"
#include "neighbor.h"
#include "comm.h"
#include "thermo.h"
#include "timer.h"

void stats(int, double *, double *, double *, double *, int, int *);

void output(In &in, Atom &atom, Force &force, Neighbor &neighbor, Comm &comm,
	    Thermo &thermo, Integrate &integrate, Timer &timer)
{
  int i,n;
  int me,nprocs;
  int histo[10];
  double tmp,ave,max,min,total;
  FILE *fp;

  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  /* enforce PBC, then check for lost atoms */

  atom.pbc();

  int natoms;
  MPI_Allreduce(&atom.nlocal,&natoms,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);

  int nlost = 0;
  for (i = 0; i < atom.nlocal; i++) {
    if (atom.x[i][0] < 0.0 || atom.x[i][0] >= atom.box.xprd || 
	atom.x[i][1] < 0.0 || atom.x[i][1] >= atom.box.yprd || 
	atom.x[i][2] < 0.0 || atom.x[i][2] >= atom.box.zprd) nlost++;
  }
  int nlostall;
  MPI_Allreduce(&nlost,&nlostall,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);

  if (natoms != atom.natoms || nlostall > 0) {
    if (me == 0) printf("Atom counts = %d %d %d\n",
			nlostall,natoms,atom.natoms);
    if (me == 0) printf("ERROR: Incorrect number of atoms\n");
    return;
  }

  /* long-range energy and pressure corrections */

  double engcorr = 8.0*3.1415926*in.rho *
    (1.0/(9.0*pow(force.cutforce,9.0)) - 1.0/(3.0*pow(force.cutforce,3.0)));
  double prscorr = 8.0*3.1415926*in.rho*in.rho *
    (4.0/(9.0*pow(force.cutforce,9.0)) - 2.0/(3.0*pow(force.cutforce,3.0)));

  /* thermo output */

  double conserve;
  if (me == 0) {
    fprintf(stdout,"Timestep, T*, U*, P*, Conservation:\n");
    fp = fopen("lj.out","w");
    fprintf(fp,"Timestep, T*, U*, P*, Conservation:\n");
    for (i = 0; i < thermo.mstat; i++) {
      conserve = (1.5*thermo.tmparr[i]+thermo.engarr[i]) / 
	(1.5*thermo.tmparr[0]+thermo.engarr[0]);
      fprintf(stdout,"%d %15.10g %15.10g %15.10g %15.10g\n",
	      thermo.steparr[i],thermo.tmparr[i],
	      thermo.engarr[i]+engcorr,thermo.prsarr[i]+prscorr,conserve);
      fprintf(fp,"%d %15.10g %15.10g %15.10g %15.10g\n",
	      thermo.steparr[i],thermo.tmparr[i],
	      thermo.engarr[i]+engcorr,thermo.prsarr[i]+prscorr,conserve);
    }
  }
      
  /* performance output */

  if (me == 0) {
    fprintf(stdout,"\n");
    fprintf(fp,"\n");
  }

  double time_total = timer.array[TIME_TOTAL];
  MPI_Allreduce(&time_total,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  time_total = tmp/nprocs;
  double mflops = 4.0/3.0 * 3.1415926 * 
    pow(force.cutforce,3.0) * in.rho * 0.5 *
    23 * natoms * integrate.ntimes / time_total / 1000000.0;

  if (me == 0) {
    fprintf(stdout,"Total time: %g on %d procs for %d atoms\n",
	    time_total,nprocs,natoms);
    fprintf(stdout,"Aggregate Mflops: %g\n",mflops);
    fprintf(stdout,"Mflops/proc: %g\n",mflops/nprocs);
    fprintf(fp,"Total time: %g on %d procs for %d atoms\n",
	    time_total,nprocs,natoms);
    fprintf(fp,"Aggregate Mflops: %g\n",mflops);
    fprintf(fp,"Mflops/proc: %g\n",mflops/nprocs);
  }

  if (time_total == 0.0) time_total = 1.0;

  if (me == 0) {
    fprintf(stdout,"\n");
    fprintf(fp,"\n");
  }
  
  double time_force = timer.array[TIME_FORCE];
  MPI_Allreduce(&time_force,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  tmp /= nprocs;
  if (me == 0) {
    fprintf(stdout,"Force time/%%: %g %g\n",tmp,tmp/time_total*100.0);
    fprintf(fp,"Force time/%%: %g %g\n",tmp,tmp/time_total*100.0);
  }

  double time_neigh = timer.array[TIME_NEIGH];
  MPI_Allreduce(&time_neigh,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  tmp /= nprocs;
  if (me == 0) {
    fprintf(stdout,"Neigh time/%%: %g %g\n",tmp,tmp/time_total*100.0);
    fprintf(fp,"Neigh time/%%: %g %g\n",tmp,tmp/time_total*100.0);
  }

  double time_comm = timer.array[TIME_COMM];
  MPI_Allreduce(&time_comm,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  tmp /= nprocs;
  if (me == 0) {
    fprintf(stdout,"Comm  time/%%: %g %g\n",tmp,tmp/time_total*100.0);
    fprintf(fp,"Comm  time/%%: %g %g\n",tmp,tmp/time_total*100.0);
  }

      
  double time_other = time_total - (time_force + time_neigh + time_comm);
  MPI_Allreduce(&time_other,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  tmp /= nprocs;
  if (me == 0) {
    fprintf(stdout,"Other time/%%: %g %g\n",tmp,tmp/time_total*100.0);
    fprintf(fp,"Other time/%%: %g %g\n",tmp,tmp/time_total*100.0);
  }

  if (me == 0) {
    fprintf(stdout,"\n");
    fprintf(fp,"\n");
  }

  stats(1,&time_force,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Force time: %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Force time: %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  stats(1,&time_neigh,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Neigh time: %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Neigh time: %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  stats(1,&time_comm,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Comm  time: %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Comm  time: %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  stats(1,&time_other,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Other time: %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Other time: %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  if (me == 0) {
    fprintf(stdout,"\n");
    fprintf(fp,"\n");
  }

  tmp = atom.nlocal;
  stats(1,&tmp,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Nlocal:     %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Nlocal:     %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  tmp = atom.nghost;
  stats(1,&tmp,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Nghost:     %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Nghost:     %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  n = 0;
  for (i = 0; i < comm.nswap; i++) n += comm.sendnum[i];
  tmp = n;
  stats(1,&tmp,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Nswaps:     %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Nswaps:     %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  n = 0;
  for (i = 0; i < atom.nlocal; i++) n += neighbor.numneigh[i];
  tmp = n;
  stats(1,&tmp,&ave,&max,&min,10,histo);
  if (me == 0) {
    fprintf(stdout,"Neighs:     %g ave %g max %g min\n",ave,max,min);
    fprintf(stdout,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(stdout," %d",histo[i]);
    fprintf(stdout,"\n");
    fprintf(fp,"Neighs:     %g ave %g max %g min\n",ave,max,min);
    fprintf(fp,"Histogram:");
    for (i = 0; i < 10; i++) fprintf(fp," %d",histo[i]);
    fprintf(fp,"\n");
  }

  MPI_Allreduce(&tmp,&total,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  if (me == 0) {
    fprintf(stdout,"Total # of neighbors = %g\n",total);
    fprintf(fp,"Total # of neighbors = %g\n",total);
  }

  if (me == 0) {
    fprintf(stdout,"\n");
    fprintf(fp,"\n");
  }
      
  if (me == 0) {

    /*        
    fprintf(fp,"Single processor:\n");
    fprintf(fp,"Max # of local atoms = %d\n",atom.max_nlocal);
    fprintf(fp,"Max # of ghost atoms = %d\n",atom.max_ghost);
    fprintf(fp,"Max # of neighbors = %d\n",neighbor.max_neigh);
    fprintf(fp,"Max # of exchanges = %d\n",comm.max_exch);
    fprintf(fp,"Max # of borders = %d\n",comm.max_bord);
    fprintf(fp,"Max # of bins = %d\n",comm.max_bin);
    fprintf(fp,"Bins in stencil = %d\n",comm.nstencil);
    fprintf(fp,"# of Swaps = %d, Needs = %d %d %d\n",comm.nswap,
	    comm.need[0],comm.need[1],comm.need[2]);
    fprintf(fp,"Cutneigh = %g, Cut/Box = %g %g %g\n",neigh.cutneigh,
            neigh.cutneigh*comm.procgrid[0]/atom.box.xprd,
            neigh.cutneigh*comm.procgrid[1]/atom.box.yprd,
            neigh.cutneigh*comm.procgrid[2]/atom.box.zprd);
    */
  }

  if (me == 0) fclose(fp);
}

void stats(int n, double *data, double *pave, double *pmax, double *pmin,
	   int nhisto, int *histo)
{
  int i,m;
  int *histotmp;

  double min = 1.0e20;
  double max = -1.0e20;
  double ave = 0.0;
  for (i = 0; i < n; i++) {
    ave += data[i];
    if (data[i] < min) min = data[i];
    if (data[i] > max) max = data[i];
  }

  int ntotal;
  MPI_Allreduce(&n,&ntotal,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
  double tmp;
  MPI_Allreduce(&ave,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  ave = tmp/ntotal;
  MPI_Allreduce(&min,&tmp,1,MPI_DOUBLE,MPI_MIN,MPI_COMM_WORLD);
  min = tmp;
  MPI_Allreduce(&max,&tmp,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
  max = tmp;

  for (i = 0; i < nhisto; i++) histo[i] = 0;

  double del = max - min;
  for (i = 0; i < n; i++) {
    if (del == 0.0) m = 0;
    else m = static_cast<int>((data[i]-min)/del * nhisto);
    if (m > nhisto-1) m = nhisto-1;
    histo[m]++;
  }

  histotmp = (int *) malloc(nhisto*sizeof(int));
  MPI_Allreduce(histo,histotmp,nhisto,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
  for (i = 0; i < nhisto; i++) histo[i] = histotmp[i];
  free(histotmp);

  *pave = ave;
  *pmax = max;
  *pmin = min;
}
