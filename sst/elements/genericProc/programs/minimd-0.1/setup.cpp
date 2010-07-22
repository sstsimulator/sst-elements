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
#include "atom.h"
#include "thermo.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


double random(int *);

/* create simulation box */

void create_box(Atom &atom, int nx, int ny, int nz, double rho)
{
  double lattice = pow((4.0/rho),(1.0/3.0));
  atom.box.xprd = nx*lattice;
  atom.box.yprd = ny*lattice;
  atom.box.zprd = nz*lattice;
}

/* initialize atoms on fcc lattice in parallel fashion */

int create_atoms(Atom &atom, int nx, int ny, int nz, double rho)
{
  /* total # of atoms */

  atom.natoms = 4*nx*ny*nz;
  atom.nlocal = 0;

  /* determine loop bounds of lattice subsection that overlaps my sub-box
     insure loop bounds do not exceed nx,ny,nz */

  double alat = pow((4.0/rho),(1.0/3.0));
  int ilo = static_cast<int>(atom.box.xlo/(0.5*alat) - 1);
  int ihi = static_cast<int>(atom.box.xhi/(0.5*alat) + 1);
  int jlo = static_cast<int>(atom.box.ylo/(0.5*alat) - 1);
  int jhi = static_cast<int>(atom.box.yhi/(0.5*alat) + 1);
  int klo = static_cast<int>(atom.box.zlo/(0.5*alat) - 1);
  int khi = static_cast<int>(atom.box.zhi/(0.5*alat) + 1);

  ilo = MAX(ilo,0);
  ihi = MIN(ihi,2*nx-1);
  jlo = MAX(jlo,0);
  jhi = MIN(jhi,2*ny-1);
  klo = MAX(klo,0);
  khi = MIN(khi,2*nz-1);

  /* each proc generates positions and velocities of atoms on fcc sublattice 
       that overlaps its box
     only store atoms that fall in my box
     use atom # (generated from lattice coords) as unique seed to generate a
       unique velocity
     exercise RNG between calls to avoid correlations in adjacent atoms */
      
  double xtmp,ytmp,ztmp,vx,vy,vz;
  int i,j,k,m,n;

  int iflag = 0;
  for (k = klo; k <= khi; k++) {
    for (j = jlo; j <= jhi; j++) {
      for (i = ilo; i <= ihi; i++) {
	if (iflag) continue;
	if ((i+j+k) % 2 == 0) {
	  xtmp = 0.5*alat*i;
	  ytmp = 0.5*alat*j;
	  ztmp = 0.5*alat*k;
	  if (xtmp >= atom.box.xlo && xtmp < atom.box.xhi &&
	      ytmp >= atom.box.ylo && ytmp < atom.box.yhi &&
	      ztmp >= atom.box.zlo && ztmp < atom.box.zhi) {
            n = k*(2*ny)*(2*nx) + j*(2*nx) + i + 1;
            for (m = 0; m < 5; m++) random(&n);
            vx = random(&n);
            for (m = 0; m < 5; m++) random(&n);
            vy = random(&n);
            for (m = 0; m < 5; m++) random(&n);
            vz = random(&n);
            atom.addatom(xtmp,ytmp,ztmp,vx,vy,vz);
	  }
	}
      }
    }
  }

  /* check for overflows on any proc */

  int me;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);

  int iflagall;
  MPI_Allreduce(&iflag,&iflagall,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
  if (iflagall) {
    if (me == 0) printf("No memory for atoms\n");
    return 1;
  }

  /* check that correct # of atoms were created */

  int natoms;
  MPI_Allreduce(&atom.nlocal,&natoms,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
  if (natoms != atom.natoms) {
    if (me == 0) printf("Created incorrect # of atoms\n");
    return 1;
  }

  return 0;
}

/* adjust initial velocities to give desired temperature */

void create_velocity(double t_request, Atom &atom, Thermo &thermo)
{
  int i;

  /* zero center-of-mass motion */

  double vxtot = 0.0;
  double vytot = 0.0;
  double vztot = 0.0;
  for (i = 0; i < atom.nlocal; i++) {
    vxtot += atom.v[i][0];
    vytot += atom.v[i][1];
    vztot += atom.v[i][2];
  }

  double tmp;
  MPI_Allreduce(&vxtot,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  vxtot = tmp/atom.natoms;
  MPI_Allreduce(&vytot,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  vytot = tmp/atom.natoms;
  MPI_Allreduce(&vztot,&tmp,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  vztot = tmp/atom.natoms;

  for (i = 0; i < atom.nlocal; i++) {
    atom.v[i][0] -= vxtot;
    atom.v[i][1] -= vytot;
    atom.v[i][2] -= vztot;
    atom.vold[i][0] = atom.v[i][0];
    atom.vold[i][1] = atom.v[i][1];
    atom.vold[i][2] = atom.v[i][2];
  }

  /* rescale velocities, including old ones */

  double t = thermo.temperature(atom);
  double factor = sqrt(t_request/t);
  for (i = 0; i < atom.nlocal; i++) {
    atom.v[i][0] *= factor;
    atom.v[i][1] *= factor;
    atom.v[i][2] *= factor;
    atom.vold[i][0] = atom.v[i][0];
    atom.vold[i][1] = atom.v[i][1];
    atom.vold[i][2] = atom.v[i][2];
  }
}

/* Park/Miller RNG w/out MASKING, so as to be like f90s version */

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876

double random(int *idum)
{
  int k;
  double ans;

  k=(*idum)/IQ;
  *idum=IA*(*idum-k*IQ)-IR*k;
  if (*idum < 0) *idum += IM;
  ans=AM*(*idum);
  return ans;
}

#undef IA
#undef IM
#undef AM
#undef IQ
#undef IR
#undef MASK
