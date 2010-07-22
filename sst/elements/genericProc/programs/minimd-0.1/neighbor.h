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

#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include "atom.h"

class Neighbor {
 public:
  int every;                       // re-neighbor every this often
  int nbinx,nbiny,nbinz;           // # of global bins
  double cutneigh;                 // neighbor cutoff
  double cutneighsq;               // neighbor cutoff squared
  int ncalls;                      // # of times build has been called
  int max_totalneigh;              // largest # of neighbors ever stored

  int *numneigh;                   // # of neighbors for each atom
  int **firstneigh;                // ptr to 1st neighbor of each atom

  Neighbor();
  ~Neighbor();
  int setup(Atom &);               // setup bins based on box and cutoff
  void build(Atom &);              // create neighbor list
  
 private:
  double xprd,yprd,zprd;           // box size
  int npage;                       // current page in page list
  int maxpage;                     // # of pages currently allocated
  int **pages;                     // neighbor list pages

  int nmax;                        // max size of atom arrays in neighbor
  int *binhead;                    // ptr to 1st atom in each bin
  int *bins;                       // ptr to next atom in each bin

  int nstencil;                    // # of bins in stencil
  int *stencil;                    // stencil list of bin offsets

  int mbins;                       // binning parameters
  int mbinx,mbiny,mbinz;
  int mbinxlo,mbinylo,mbinzlo;
  double binsizex,binsizey,binsizez;
  double bininvx,bininvy,bininvz;

  void binatoms(Atom &);           // bin all atoms
  double bindist(int, int, int);   // distance between binx
  int coord2bin(double, double, double);   // mapping atom coord to a bin
};

#endif
