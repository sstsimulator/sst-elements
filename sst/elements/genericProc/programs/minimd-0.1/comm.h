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

#ifndef COMM_H
#define COMM_H

#include "atom.h"

class Comm {
 public:
  Comm();
  ~Comm();
  int setup(double, Atom &);
  void communicate(Atom &);
  void reverse_communicate(Atom &);
  void exchange(Atom &);
  void borders(Atom &);
  void growsend(int);
  void growrecv(int);
  void growlist(int, int);

 public:
  int me;                           // my proc ID
  int nswap;                        // # of swaps to perform
  int *pbc_any;                     // whether any PBC on this swap
  int *pbc_flagx;                   // PBC correction in x for this swap
  int *pbc_flagy;                   // same in y
  int *pbc_flagz;                   // same in z
  int *sendnum,*recvnum;            // # of atoms to send/recv in each swap
  int *comm_send_size;              // # of values to send in each comm
  int *comm_recv_size;              // # of values to recv in each comm
  int *reverse_send_size;           // # of values to send in each reverse
  int *reverse_recv_size;           // # of values to recv in each reverse
  int *sendproc,*recvproc;          // proc to send/recv with at each swap

  int *firstrecv;                   // where to put 1st recv atom in each swap
  int **sendlist;                   // list of atoms to send in each swap
  int *maxsendlist;

  double *buf_send;                 // send buffer for all comm
  double *buf_recv;                 // recv buffer for all comm
  int maxsend;
  int maxrecv;

  int procneigh[3][2];           // my 6 proc neighbors
  int procgrid[3];                  // # of procs in each dim
  int need[3];                      // how many procs away needed in each dim
  double *slablo,*slabhi;           // bounds of slabs to send to other procs
};

#endif
