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
#include "integrate.h"

Integrate::Integrate() {}
Integrate::~Integrate() {}

void Integrate::setup()
{
  dtforce = 48.0*dt;
}

void Integrate::run(Atom &atom, Force &force, Neighbor &neighbor,
		    Comm &comm, Thermo &thermo, Timer &timer)
{
  int i,n,nlocal;
  double **x,**v,**f,**vold;

  for (n = 0; n < ntimes; n++) {

    x = atom.x;
    v = atom.v;
    nlocal = atom.nlocal;

    for (i = 0; i < nlocal; i++) {
      x[i][0] += dt*v[i][0];
      x[i][1] += dt*v[i][1];
      x[i][2] += dt*v[i][2];
    }

    timer.stamp();

    if ((n+1) % neighbor.every) {
      comm.communicate(atom);
      timer.stamp(TIME_COMM);
    } else {
      comm.exchange(atom);
      comm.borders(atom);
      timer.stamp(TIME_COMM);
      neighbor.build(atom);
      timer.stamp(TIME_NEIGH);
    }      

    force.compute(atom,neighbor,comm.me);
    timer.stamp(TIME_FORCE);

    comm.reverse_communicate(atom);
    timer.stamp(TIME_COMM);

    vold = atom.vold;
    v = atom.v;
    f = atom.f;
    nlocal = atom.nlocal;

    for (i = 0; i < nlocal; i++) {
      vold[i][0] = v[i][0];
      vold[i][1] = v[i][1];
      vold[i][2] = v[i][2];
      v[i][0] += dtforce*f[i][0];
      v[i][1] += dtforce*f[i][1];
      v[i][2] += dtforce*f[i][2];
    }

    if (thermo.nstat) thermo.compute(n+1,atom,neighbor,force);
  }
}
