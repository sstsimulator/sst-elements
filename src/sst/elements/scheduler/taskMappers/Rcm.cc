// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * THIS FILE HAS BEEN ALTERED FOR THE SST SOFTWARE PACKAGE
 *
 *
 * Copyright (c) 2005 David Fritzsche
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment in the
 *    product documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include "Rcm.h"

using namespace SST::Scheduler;

void RCM::genrcm(const int n, const int *xadj, const int *adj, int *perm,
                 signed char *mask, int *deg)
{
    int i, root, ccsize;
    int num = 0;

    for (i = 0; i < n; ++i) {
        mask[i] = 0;
    }

    degree(n, xadj, adj, mask, deg);

    for (i = 0; i < n; ++i) {
        if (mask[i]) {
            continue;
        }
        root = i;

        /*
         * Find a pseudo-peripheral node and calculate RCM.
         * Note that we reuse perm to also store the level
         * structure (perm+num is parameter ls of FNROOT()).
         */
        fnroot(&root, xadj, adj, deg, &ccsize, mask, perm+num);
        rcm(root, xadj, adj, deg, mask, perm+num, &ccsize);

        num += ccsize;
        if (num >= n) {
            break;
        }
    }
}

void RCM::degree(const int n, const int* xadj, const int* adj,
                 signed char* mask, int* deg)
{
    for (int i = 0; i < n; ++i) {
        deg[i] = xadj[i+1] - xadj[i];
    }
}

void RCM::fnroot(int* root, const int* xadj, const int* adj,
                 const int* deg, int* ccsize, signed char* mask, int* ls)
{
    int j, new_nlvl, last_lvl, node, ndeg, mindeg;
    int nlvl = 1;

    nlvl = 1;

    while (1) {
        rootls(*root, xadj, adj,
               mask,
               ccsize, &new_nlvl, &last_lvl,
               ls);
        /*
         * Return if the number of levels stays the same
         * or if we have reached the maximum level count.
         * The number of levels can not decrease.
         */
        if (new_nlvl == nlvl || new_nlvl == *ccsize) {
            break;
        }
        nlvl = new_nlvl;
        /*
         * Pick a node with minimum degree from the last level.
         * last_lvl points to the start of the last level.
         */
        mindeg = *ccsize;
        for (j = last_lvl; j < *ccsize; ++j) {
            node = ls[j];
            ndeg = deg[node];
            if (ndeg < mindeg) {
                *root = node;
                mindeg = ndeg;
            }
        }
    }
}

void RCM::rootls(const int root, const int* xadj, const int* adj,
                 signed char* mask, int * ccsize, int * nlvl,
                 int * last_lvl, int * ls)
{
    int i, j;
    int nbr, node;

    int lvl_begin, lvl_end; /* C style indices */
    
    mask[root] = 1; /* mask root */

    /* init first level */
    *nlvl = 0;
    ls[0] = root;
    lvl_begin = 0;
    lvl_end = 1;
    *ccsize = 1;

    /*
     * lvl_begin is the pointer to the beginning of the current
     * level, and lvl_end-1 points to the end of this level.
     */
    do {
        /*
         * Generate the next level by finding all the non-masked
         * neighbors of all the nodes in the current level.
         */
        for (i = lvl_begin; i < lvl_end; ++i) {
            node = ls[i];
            for (j = xadj[node]; j < xadj[node + 1]; ++j) {
                nbr = adj[j];
                if (!mask[nbr]) {
                    mask[nbr] = 1;
                    ls[*ccsize] = nbr;
                    ++(*ccsize);
                }
            }
        }

        /*
         * Reset lvl_begin and lvl_end to iterate over the now
         * generated next level.
         */
        *last_lvl = lvl_begin;
        lvl_begin = lvl_end;
        lvl_end = *ccsize;
        ++(*nlvl);
    }
    while (lvl_end - lvl_begin > 0);

    /*
     * Reset mask to zero for the nodes in the level structure,
     * i.e. the nodes are again unmasked.
     */
    for (i = 0; i < *ccsize; ++i) {
        mask[ ls[i] ] = 0;
    }
}

void RCM::rcm(const int root, const int* xadj, const int* adj, const int* deg,
              signed char* mask, int* perm, int* ccsize)
{
    int i, j;
    int nbr, node;

    int lvl_begin, lvl_end;
    int fnbr, lnbr;

    /*
     * Fill the first level set with the root node,
     * and initialize lvl_begin, lvl_end and lnbr
     * accordingly.
     * Adjust the arrays if Fortran style indices are used
     * for xadj, adj and perm.
     */

    perm[0] = root;
    mask[ root ] = -1;

    lvl_begin = 0;
    lvl_end = 1;
    lnbr = lvl_end;

    /*
     * Iterate over all level sets. lvl_begin and lvl_end point to the
     * beginning and behind the end of the current level respectively.
     */
    do {
        /* iterate over all nodes in the current level */
        for (i = lvl_begin; i < lvl_end; ++i) {
            node = perm[i];
            /*
             * Find the non-masked neighbors of node.
             * fnbr and lnbr keep track of where these nodes are
             * copied to in perm.
             */
            fnbr = lnbr;
            for (j = xadj[node]; j < xadj[node+1]; ++j) {
                nbr = adj[j];
                if ( !mask[nbr] ) {
                    mask[nbr] = -1;
                    perm[lnbr] = nbr;
                    ++lnbr;
                }
            }
            if (lnbr - fnbr > 1) {
                heapsort(lnbr-fnbr, perm+fnbr, deg);
            }
        }
        lvl_begin = lvl_end;
        lvl_end = lnbr;
    } while (lvl_end - lvl_begin > 0);

    /* we now have a Cuthill-McKee ordering in perm */

    *ccsize = lnbr;

    /* reverse perm if reversal is not explicitly unwanted */
    j = lnbr - 1;
    for (i = 0; i < lnbr / 2; ++i, --j) {
        node = perm[j];
        perm[j] = perm[i];
        perm[i] = node;
    }
}

void RCM::heapsort(const int sz, int * perm, const int * deg)
{
    int n = sz;
    int i, t;

    for (i = n/2; i > 0;) {
        --i;
        sift(i, n, perm[i], perm, deg);
    }
    for (; n > 1;) {
        --n;
        t = perm[n];
        perm[n] = perm[0];
        sift(i, n, t, perm, deg);
    }
}

void RCM::sift(const int i, const int n, const int t, int * perm, const int * deg)
{
    int j, w;
    j = i;
    w = i*2 + 1;

    while (w < n) {
        if ( (w + 1 < n) && ( deg[perm[w]] < deg[perm[w+1]] ) ) {
            ++w;
        }
        if ( deg[t] < deg[perm[w]] ) {
            /* we have to exchange/rotate and to go further down */
            perm[j] = perm[w];
            j = w;
            w = j*2 + 1;
        } else {
            /* v has heap property */
            break;
        }
    }
    perm[j] = t;
}
