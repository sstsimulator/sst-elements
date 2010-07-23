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
#include "stdlib.h"

#include "neighbor.h"

#define PAGESIZE 10000
#define ONEATOM 1000
#define PAGEDELTA 1
#define FACTOR 0.999
#define SMALL 1.0e-6

Neighbor::Neighbor()
{
  ncalls = 0;
  max_totalneigh = 0;
  numneigh = NULL;
  firstneigh = NULL;
  nmax = 0;
  binhead = NULL;
  bins = NULL;
  stencil = NULL;

  maxpage = 1;
  pages = (int **) malloc(maxpage*sizeof(int *));
  pages[0] = (int *) malloc(PAGESIZE*sizeof(int));
}

Neighbor::~Neighbor()
{
  if (numneigh) free(numneigh);
  if (firstneigh) free(firstneigh);
  for (int i = 0; i < maxpage; i++) free(pages[i]);
  if (pages) free(pages);
  if (binhead) free(binhead);
  if (bins) free(bins);
}

/* binned neighbor list construction with full Newton's 3rd law
   every pair stored exactly once by some processor
   each owned atom i checks its own bin and other bins in Newton stencil */

void Neighbor::build(Atom &atom)
{
  int i,j,k,m,n,ibin,nlocal,nall,npnt;
  double xtmp,ytmp,ztmp,delx,dely,delz,rsq;
  int *neighptr;
  double **x;

  ncalls++;
  nlocal = atom.nlocal;
  nall = atom.nlocal + atom.nghost;

  /* extend atom arrays if necessary */

  if (nall > nmax) {
    nmax = nall;
    if (numneigh) free(numneigh);
    if (firstneigh) free(firstneigh);
    if (bins) free(bins);
    numneigh = (int *) malloc(nmax*sizeof(int));
    firstneigh = (int **) malloc(nmax*sizeof(int *));
    bins = (int *) malloc(nmax*sizeof(int));
  }

  /* bin local & ghost atoms */

  printf(" binning...\n");
  binatoms(atom);

  /* loop over each atom, storing neighbors */

  x = atom.x;

  npnt = 0;
  npage = 0;

  printf(" looping %d...\n", nlocal);
  for (i = 0; i < nlocal; i++) {

    /* if necessary, goto next page and add pages */

    if (PAGESIZE - npnt < ONEATOM) {
      npnt = 0;
      npage++;
      if (npage == maxpage) {
	maxpage += PAGEDELTA;
	pages = (int **) realloc(pages,maxpage*sizeof(int *));
	for (m = npage; m < maxpage; m++)
	  pages[m] = (int *) malloc(PAGESIZE*sizeof(int));
      }
    }

    neighptr = &pages[npage][npnt];
    n = 0;

    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];

    /* loop over rest of atoms in i's bin, ghosts are at end of linked list
       if j is owned atom, store it, since j is beyond i in linked list
       if j is ghost, only store if j coords are "above and to the right" of i
    */

    j = bins[i];
    while (j >= 0) {
      if (j >= nlocal) {
	if ((x[j][2] < ztmp) || (x[j][2] == ztmp && x[j][1] < ytmp) ||
	    (x[j][2] == ztmp && x[j][1]  == ytmp && x[j][0] < xtmp)) {
	  j = bins[j];
	  continue;
	}
      }
      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      if (rsq <= cutneighsq) neighptr[n++] = j;
      j = bins[j];
    }

    /* loop over all atoms in other bins in stencil, store every pair */

    ibin = coord2bin(xtmp,ytmp,ztmp);
    for (k = 0; k < nstencil; k++) {
      j = binhead[ibin+stencil[k]];
      while (j >= 0) {
	delx = xtmp - x[j][0];
	dely = ytmp - x[j][1];
	delz = ztmp - x[j][2];
	rsq = delx*delx + dely*dely + delz*delz;
	if (rsq <= cutneighsq) neighptr[n++] = j;
	j = bins[j];
      }
    }

    firstneigh[i] = neighptr;
    numneigh[i] = n;
    npnt += n;
  }
}
      
/* bin owned and ghost atoms */

void Neighbor::binatoms(Atom &atom)
{
  int i,ibin,nlocal,nall;
  double **x;

  nlocal = atom.nlocal;
  nall = atom.nlocal + atom.nghost;
  x = atom.x;

  xprd = atom.box.xprd;
  yprd = atom.box.yprd;
  zprd = atom.box.zprd;

  for (i = 0; i < mbins; i++) binhead[i] = -1;

  /* bin ghost atoms 1st, so will be at end of linked list */

  for (i = nlocal; i < nall; i++) {
    ibin = coord2bin(x[i][0],x[i][1],x[i][2]);
    bins[i] = binhead[ibin];
    binhead[ibin] = i;
  }

  /* bin owned atoms */

  for (i = 0; i < nlocal; i++) {
    ibin = coord2bin(x[i][0],x[i][1],x[i][2]);
    bins[i] = binhead[ibin];
    binhead[ibin] = i;
  }
}

/* convert xyz atom coords into local bin #
   take special care to insure ghost atoms with
   coord >= prd or coord < 0.0 are put in correct bins */

int Neighbor::coord2bin(double x, double y, double z)
{
  int ix,iy,iz;

  if (x >= xprd)
    ix = (int) ((x-xprd)*bininvx) + nbinx - mbinxlo;
  else if (x >= 0.0)
    ix = (int) (x*bininvx) - mbinxlo;
  else
    ix = (int) (x*bininvx) - mbinxlo - 1;
  
  if (y >= yprd)
    iy = (int) ((y-yprd)*bininvy) + nbiny - mbinylo;
  else if (y >= 0.0)
    iy = (int) (y*bininvy) - mbinylo;
  else
    iy = (int) (y*bininvy) - mbinylo - 1;
  
  if (z >= zprd)
    iz = (int) ((z-zprd)*bininvz) + nbinz - mbinzlo;
  else if (z >= 0.0)
    iz = (int) (z*bininvz) - mbinzlo;
  else
    iz = (int) (z*bininvz) - mbinzlo - 1;

  return (iz*mbiny*mbinx + iy*mbinx + ix + 1);
}


/*
setup neighbor binning parameters
bin numbering is global: 0 = 0.0 to binsize
                         1 = binsize to 2*binsize
                         nbin-1 = prd-binsize to binsize
                         nbin = prd to prd+binsize
                         -1 = -binsize to 0.0
coord = lowest and highest values of ghost atom coords I will have
        add in "small" for round-off safety
mbinlo = lowest global bin any of my ghost atoms could fall into
mbinhi = highest global bin any of my ghost atoms could fall into
mbin = number of bins I need in a dimension
stencil() = bin offsets in 1-d sense for stencil of surrounding bins
*/

int Neighbor::setup(Atom &atom)
{
  int i,j,k,nmax;
  double coord;
  int mbinxhi,mbinyhi,mbinzhi;
  int nextx,nexty,nextz;
 
  cutneighsq = cutneigh*cutneigh;
 
  xprd = atom.box.xprd;
  yprd = atom.box.yprd;
  zprd = atom.box.zprd;

  /*
c bins must evenly divide into box size, 
c   becoming larger than cutneigh if necessary
c binsize = 1/2 of cutoff is near optimal

  if (flag == 0) {
    nbinx = 2.0 * xprd / cutneigh;
    nbiny = 2.0 * yprd / cutneigh;
    nbinz = 2.0 * zprd / cutneigh;
    if (nbinx == 0) nbinx = 1;
    if (nbiny == 0) nbiny = 1;
    if (nbinz == 0) nbinz = 1;
  }
  */

  binsizex = xprd/nbinx;
  binsizey = yprd/nbiny;
  binsizez = zprd/nbinz;
  bininvx = 1.0 / binsizex;
  bininvy = 1.0 / binsizey;
  bininvz = 1.0 / binsizez;

  coord = atom.box.xlo - cutneigh - SMALL*xprd;
  mbinxlo = static_cast<int>(coord*bininvx);
  if (coord < 0.0) mbinxlo = mbinxlo - 1;
  coord = atom.box.xhi + cutneigh + SMALL*xprd;
  mbinxhi = static_cast<int>(coord*bininvx);

  coord = atom.box.ylo - cutneigh - SMALL*yprd;
  mbinylo = static_cast<int>(coord*bininvy);
  if (coord < 0.0) mbinylo = mbinylo - 1;
  coord = atom.box.yhi + cutneigh + SMALL*yprd;
  mbinyhi = static_cast<int>(coord*bininvy);

  coord = atom.box.zlo - cutneigh - SMALL*zprd;
  mbinzlo = static_cast<int>(coord*bininvz);
  if (coord < 0.0) mbinzlo = mbinzlo - 1;
  coord = atom.box.zhi + cutneigh + SMALL*zprd;
  mbinzhi = static_cast<int>(coord*bininvz);

/* extend bins by 1 in each direction to insure stencil coverage */

  mbinxlo = mbinxlo - 1;
  mbinxhi = mbinxhi + 1;
  mbinx = mbinxhi - mbinxlo + 1;

  mbinylo = mbinylo - 1;
  mbinyhi = mbinyhi + 1;
  mbiny = mbinyhi - mbinylo + 1;

  mbinzlo = mbinzlo - 1;
  mbinzhi = mbinzhi + 1;
  mbinz = mbinzhi - mbinzlo + 1;

  /*
compute bin stencil of all bins whose closest corner to central bin
  is within neighbor cutoff
for partial Newton (newton = 0),
  stencil is all surrounding bins including self
for full Newton (newton = 1),
  stencil is bins to the "upper right" of central bin, does NOT include self
next(xyz) = how far the stencil could possibly extend
factor < 1.0 for special case of LJ benchmark so code will create
  correct-size stencil when there are 3 bins for every 5 lattice spacings
  */

  nextx = static_cast<int>(cutneigh*bininvx);
  if (nextx*binsizex < FACTOR*cutneigh) nextx++;
  nexty = static_cast<int>(cutneigh*bininvy);
  if (nexty*binsizey < FACTOR*cutneigh) nexty++;
  nextz = static_cast<int>(cutneigh*bininvz);
  if (nextz*binsizez < FACTOR*cutneigh) nextz++;

  nmax = (nextz+1) * (2*nexty+1) * (2*nextx+1);
  if (stencil) free(stencil);
  stencil = (int *) malloc(nmax*sizeof(int));

  nstencil = 0;
  for (k = 0; k <= nextz; k++) {
    for (j = -nexty; j <= nexty; j++) {
      for (i = -nextx; i <= nextx; i++) {
	if (k > 0 || j > 0 || (j == 0 && i > 0)) {
	  if (bindist(i,j,k) < cutneighsq) {
	    stencil[nstencil] = k*mbiny*mbinx + j*mbinx + i;
	    nstencil++;
	  }
	}
      }
    }
  }

  if (binhead) free(binhead);
  mbins = mbinx*mbiny*mbinz;
  binhead = (int *) malloc(mbins*sizeof(int));
  return 0;
}
      
/* compute closest distance between central bin (0,0,0) and bin (i,j,k) */

double Neighbor::bindist(int i, int j, int k)
{
double delx,dely,delz;

if (i > 0)
  delx = (i-1)*binsizex;
else if (i == 0)
  delx = 0.0;
else
  delx = (i+1)*binsizex;

if (j > 0)
  dely = (j-1)*binsizey;
else if (j == 0)
  dely = 0.0;
else
  dely = (j+1)*binsizey;

if (k > 0)
  delz = (k-1)*binsizez;
else if (k == 0)
  delz = 0.0;
else
  delz = (k+1)*binsizez;
 
 return (delx*delx + dely*dely + delz*delz);
}
