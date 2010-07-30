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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "stdio.h"
#include "math.h"
#include "force.h"
#ifdef USING_QTHREADS
# include <qthread.h>
# include <qthread/qloop.h>
#endif

#ifdef PIM_EXTENSIONS
#include <ppcPimCalls.h>
#endif

Force::Force() {}
Force::~Force() {}

void Force::setup()
{
  cutforcesq = cutforce*cutforce;
}

#ifdef USING_QTHREADS
struct Fcompute_il_args {
  const Neighbor &neighbor;
  const double *const*const x;
  double **f;
  const double cutforcesq;
  Fcompute_il_args(const Neighbor &n_, const double*const*const x_, double **f_, const double c_) :
      neighbor(n_), x(x_), f(f_), cutforcesq(c_)
    {}
};

static void Fcompute_innerloop(qthread_t *me, const size_t startat, const size_t stopat, void *arg)
{
    Fcompute_il_args a(*(Fcompute_il_args*)arg);
    for (size_t i=startat; i<stopat; ++i) {
	const int *const neighs = a.neighbor.firstneigh[i];
	const int numneigh = a.neighbor.numneigh[i];
	const double xtmp = a.x[i][0];
	const double ytmp = a.x[i][1];
	const double ztmp = a.x[i][2];
	for (int k = 0; k < numneigh; k++) {
#ifdef PIM_EXTENSIONS
	  const int j = neighs[k];
	  int result = 0;
	  do {
	    result = PIM_ForceCalc(j, i, &a.f[0][0], &a.x[0][0], &a.cutforcesq,
				   numneigh);
	  } while (result == 0);
	  while (PIM_OutstandingMR() > 7) {;}
#else
	  const int j = neighs[k];
	  /*printf("cutfrocesq %f\n", a.cutforcesq);
	  printf("a.x[0][0] %p %p\n", a.x, &a.x[0][0]);
	  printf("a.x[i] %p %d: %p %p %p\n", a.x, i, &a.x[i][0],&a.x[i][1],&a.x[i][2]);
	  printf("a.x[j] %p %d: %p %p %p\n", a.x, j, &a.x[j][0],&a.x[j][1],&a.x[j][2]);
	  printf("a.f[j] %p %d: %p %p %p\n", a.f, j, &a.f[j][0],&a.f[j][1],&a.f[j][2]);*/
	  const double delx = xtmp - a.x[j][0];
	  const double dely = ytmp - a.x[j][1];
	  const double delz = ztmp - a.x[j][2];
	  const double rsq = delx*delx + dely*dely + delz*delz;
	  if (rsq < a.cutforcesq) {
	    const double sr2 = 1.0/rsq;
	    const double sr6 = sr2*sr2*sr2;
	    const double force = sr6*(sr6-0.5)*sr2;
#ifdef SST
	    qthread_readFE(NULL, NULL, (const aligned_t*)&(a.f[i][0]));
	    a.f[i][0] += delx*force;
	    a.f[i][1] += dely*force;
	    a.f[i][2] += delz*force;
	    qthread_fill((const aligned_t*)&(a.f[i][0]));
	    qthread_readFE(NULL, NULL, (const aligned_t*)&(a.f[j][0]));
	    a.f[j][0] -= delx*force;
	    a.f[j][1] -= dely*force;
	    a.f[j][2] -= delz*force;
	    //printf("af_j_2 %d,%d,%d %f\n", i,j,k,a.f[j][2]);
	    qthread_fill((const aligned_t*)&(a.f[j][0]));
#else
	    // must be atomic because i may be someone else's j
	    qthread_dincr(&(a.f[i][0]), delx*force);
	    qthread_dincr(&(a.f[i][1]), dely*force);
	    qthread_dincr(&(a.f[i][2]), delz*force);
	    qthread_dincr(&(a.f[j][0]), -1*(delx*force));
	    qthread_dincr(&(a.f[j][1]), -1*(dely*force));
	    qthread_dincr(&(a.f[j][2]), -1*(delz*force));
#endif
	  }
#endif // pim extensions
	}
    }
}
#endif

void Force::compute(Atom &atom, Neighbor &neighbor, int me)
{
  int i,j,k,nlocal,nall,numneigh;
  double xtmp,ytmp,ztmp,delx,dely,delz,rsq;
  double sr2,sr6,force;
  int *neighs;
  double **x,**f;

  nlocal = atom.nlocal;
  nall = atom.nlocal + atom.nghost;
  x = atom.x;
  f = atom.f;

  // clear force on own and ghost atoms

  for (i = 0; i < nall; i++) {
    f[i][0] = 0.0;
    f[i][1] = 0.0;
    f[i][2] = 0.0;
  }

  // loop over all neighbors of my atoms
  // store force on both atoms i and j
#ifdef USING_QTHREADS
  Fcompute_il_args args(neighbor, x, f, cutforcesq);
  qt_loop_balance(1, nlocal, Fcompute_innerloop, &args);
#else
  for (i = 0; i < nlocal; i++) {
    neighs = neighbor.firstneigh[i];
    numneigh = neighbor.numneigh[i];
    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];
    for (k = 0; k < numneigh; k++) {
      j = neighs[k];
      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      if (rsq < cutforcesq) {
	sr2 = 1.0/rsq;
	sr6 = sr2*sr2*sr2;
	force = sr6*(sr6-0.5)*sr2;
	f[i][0] += delx*force;
	f[i][1] += dely*force;
	f[i][2] += delz*force;
	f[j][0] -= delx*force;
	f[j][1] -= dely*force;
	f[j][2] -= delz*force;
      }
    }
  }
#endif
}
