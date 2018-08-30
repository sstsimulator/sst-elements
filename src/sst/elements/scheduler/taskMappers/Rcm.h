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

#ifndef SST_SCHEDULER_RCM_H_
#define SST_SCHEDULER_RCM_H_

namespace SST {
    namespace Scheduler {
        class RCM{
            public:
                RCM(){};
                /**
                 * @brief Find the reverse Cuthill-McKee ordering for the
                 * graph provided in @a xadj and @a adj.
                 *
                 * Find the reverse Cuthill-McKee ordering for the
                 * graph provided in @a xadj and @a adj. For each component
                 * of the graph @c fnrooti() finds a pseudo-peripheral node @a p.
                 * Then @c rcmi() generates the reverse Cuthill-McKee ordering for
                 * this blocked, using @a p as the root.
                 *
                 * @param[in] n is the number of nodes/vertices in the graph.
                 * 
                 * @param[in] xadj
                 *               Pointers (indices) into  @a adj.  @a xadj[@a p]
                 *               points  to  the
                 *               start  of  the  adjacency  list for @a p.
                 *               @a xadj[<i>p</i>+1]-1 points to the last element in
                 *               the adjacency list for @a p.
                 *               If the graph has <i>n</i> nodes, then
                 *               @a xadj has to have <i>n</i>+1 entries.
                 *
                 * @param[in] adj
                 *               The adjacency lists. The lists are stored  in  strictly
                 *               increasing order without gaps.
                 *               An entry in @a xadj is the index of the first
                 *               entry in the associated adjacency list stored in @a adj.
                 *               Hence the
                 *               elements adjacent to node @a p are stored in @a adj in
                 *               the block
                 *               pointed to by @a xadj[@a p] (low-end, included)
                 *               and @a xadj[<i>p</i>+1]
                 *               (high-end, excluded).
                 *
                 * @param[in,out] mask
                 *               The  nodes  numbered by RCM will have their
                 *               @a mask values
                 *               set to @c -1.  The  @a mask  array  is  used  to
                 *               mark  nodes
                 *               already  consumed  by RCM. On output all nodes are
                 *               consumed, i.e., all entries have been set to @c -1.
                 *           @n
                 *               If the flag @c #RCM_USE_MASK is set, a slightly
                 *               different algorithm is used. In this case a
                 *               @a mask[@a p] value strictly greater than zero is used
                 *               to indicate that the node @a p is not to be considered
                 *               part  of  the  graph.
                 *           @n
                 *               Notice that the algorithm is slightly slower, if
                 *               the @c #RCM_USE_MASK flag is set.
                 *
                 * @param[out] perm
                 *               is  used to store the reverse Cuthill-McKee ordering of
                 *               the nodes in the graph.
                 *
                 * @param[out] deg
                 *               Storage used for the degrees of the nodes in the graph.
                 *               On output @a deg[@a p] will contain the degree of
                 *               node @a p.
                 */
                void genrcm(const int n, const int *xadj, const int *adj,
                            int *perm, signed char *mask, int *deg);
                /**
                 * @brief Find a pseudo-peripheral node for the subgraph specified
                 * by @a root.
                 * 
                 * Find a pseudo-peripheral node for the subgraph specified
                 * by @a root.
                 *
                 * @param[in] xadj
                 *               Pointers to the adjacency lists in @a adj.
                 *               See the @a xadj parameter of @c genrcmi()
                 *               for more details.
                 *
                 * @param[in] adj
                 *               The adjacency lists.
                 *               See the @a adj parameter of @c genrcmi()
                 *               for more details.
                 *
                 * @param[in] deg
                 *               The degrees of the nodes in the graph.
                 *
                 * @param[in,out] root
                 *               On input the starting node for the search and
                 *               representant
                 *               of the connected component in which we look for a
                 *               pseudo-peripheral node.   On  onput,  it  is  the  node
                 *               found.
                 * 
                 * @param[in] mask
                 *               Used  to  keep track of found nodes in the traversal of
                 *               the graph component.  Nodes marked by having a non-zero
                 *               mask  value  on input are considered to be removed from
                 *               the graph.  Although mask is modified during  the
                 *               running  of  fnroot,  on output the values in mask will
                 *               be  the same as on input.
                 * @param[out] ccsize
                 *               The size of the component rooted by root.
                 * @parma ls
                 *               Temporary storage used internally.
                 *
                 * @see
                 * N.&nbsp;E. Gibbs, W.&nbsp;G. Poole, and P.&nbsp;K. Stockemeyer.
                 * <b>An algorithm for reducing the bandwidth and
                 * profile of a sparse matrix</b>.
                 * <em>SIAM Journal of Numerical Analysis</em>, 13(2):236-250, April 1976.
                 *
                 * @see
                 * Alan George and Joseph W.&nbsp;H. Liu.
                 * <b><a href="http://doi.acm.org/10.1145/355841.355845"
                 * style="color:black;">An
                 * implementation of a pseudoperipheral node finder</a></b>.
                 * <em>ACM Trans. Math. Softw.</em>, 5(3):284-295, 1979.
                 */

                void fnroot(int *root, const int *xadj, const int *adj,
                            const int *deg, int *ccsize, signed char *mask, int *ls);
               /**
                 * @brief Find a reverse Cuthill-McKee ordering of the
                 * connected component specified by @a root.
                 *
                 * Find a reverse Cuthill-McKee ordering of the
                 * connected component specified by @a root.
                 * The numbering process is to be started at the node @a root.
                 * 
                 * @param[in] root
                 *               is  the  root  node  of the connected component and the
                 *               starting node of the RCM ordering.
                 * 
                 * @param[in] xadj
                 *               Pointers to the adjacency lists in @a adj.
                 *               See the @a xadj parameter of @c genrcmi()
                 *               for more details.
                 *
                 * @param[in] adj
                 *               The adjacency lists.
                 *               See the @a adj parameter of @c genrcmi()
                 *               for more details.
                 *
                 * @param[in] deg
                 *               An array of the node degrees.   The  array  has  to  be
                 *               filled  by the caller, @a deg[@a p] is used for the
                 *               degree
                 *               of node @a p. No further checking is done.
                 * 
                 * @param[in,out] mask
                 *               The nodes numbered by RCM will have their
                 *               @a mask values
                 *               set to @c -1.  The mask array is used to mark nodes
                 *               in the component already consumed by RCM.
                 *               Therefore  on  function  entry @a mask[@a p]  must
                 *               be @c 0 for all nodes @a p in the
                 *               same component as @a root. It is the responsibility
                 *               of the caller to ensure this.
                 *           @n
                 *               If the flag @c #RCM_USE_MASK is set, a slightly
                 *               different algorithm is used. In this case a
                 *               @a mask[@a p] value strictly greater than zero is used
                 *               to indicate that the node @a p is not to be considered
                 *               part  of  the  graph.
                 *           @n
                 *               Notice  that the degree of a node changes when a
                 *               part of the  graph  is  masked  out.   As  the  degrees
                 *               (stored  in @a deg) are supplied by the caller,
                 *               the caller
                 *               is responsible for setting  the  entries  of  @a deg
                 *               correctly.
                 * 
                 * @param[out] perm
                 *               is  used to store the reverse Cuthill-McKee ordering of
                 *               the nodes in the component rooted by  @a root. Only the
                 *               first @a ccsize entries of @a perm are overwritten.
                 * 
                 * @param[out] ccsize
                 *               The size of the component found.
                 */

                void rcm(const int root, const int *xadj, const int *adj, const int *deg,
                         signed char *mask, int *perm, int *ccsize);
            private:
                void degree(const int n, const int* xadj, const int* adj, 
                            signed char* mask, int* deg);

                void rootls(const int root, const int* xadj, const int* adj,
                            signed char* mask, int* ccsize, int* nlvl,
                            int* last_lvl, int* ls);

                void heapsort(const int sz, int* perm, const int* deg);

                void sift(const int i, const int n, const int t, int* perm, const int* deg);
        };
    }
}

#endif /* SST_SCHEDULER_RCM_H_ */
