/*
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

/**
 * @file rcm.h
 *
 * Version 0.9.1. September 24, 2005.
 *
 * @section rcm-use How to use the reverse Cuthill-McKee package
 *
 * The main routine to call is @c genrcmi() (or @c genrcml() if you
 * use @c long integers to store your graph information).
 * The way @c genrcmi()/@c genrcml() works can be adjusted by setting
 * the @a flags parameter. In most cases the `default' behavior
 * will be just fine and the caller just sets @a flags to @c 0.
 * Specific flags can be set by using the constants defined in
 * @c rcm.h. Multiple flags are combined by binary or
 * (the `<tt>|</tt>' operator in C).
 * @n
 * The undirected graph @c genrcmi() should work on is passed
 * in the form of the three parameters @a n, @a xadj and @a adj.
 * The graph is expected to be stored in a compressed format
 * similar to the compressed sparse row/column formats for matrices.
 * @n
 * The main outputs is the new ordering of the nodes, to which the
 * graph should be permuted. Notice that the output is only this
 * new ordering. The graph, as passed in @a xadj and @a adj, is
 * not modified by @c genrcmi().
 * The new ordering is stored in the caller-provided vector @a perm,
 * i.e., @a perm[@a p] is the node to become node @a p after the
 * permutation is actually applied to the graph.
 * @n
 * Additionally two working arrays (@a mask and @a deg) are needed.
 *
 * On default, @c genrcmi()/@c genrcml() expects indices to be zero
 * based (as in C) and not one based (as in Fortran). This applies
 * to @a xadj, @a adj and @a mask on input (to @a mask on input only if
 * the  @c #RCM_USE_MASK flag is set) and to @a perm, @a mask and
 * @a deg on ouput. If your data is one based (like in Fortran),
 * set the @c #RCM_FORTRAN_INDICES flag.
 *
 * Further details can be found at the documentation for @c genrcmi().
 *
 *
 * @subsection call-sub Callable subroutines
 *
 * Additionally to @c genrcmi()/@c genrcml(), two subroutines
 * are declared non-@c static, i.e., can be used from outside of
 * @c rcm.c. For more details we refer to the linked detail
 * documentation of these functions.
 * @c fnrooti() finds a pseudo-peripheral node (starting at any
 * given node in the component) and @c rcmi() finds the
 * RCM ordering, based on a given root for the rooted level set
 * structure.
 */

#ifndef RCM__H
#define RCM__H

/**
 * @def RCM_FORTRAN_INDICES
 * @brief Use Fortran style array indices.
 * The default is to use C style (zero based) indices.
 *
 * Fortran and C/C++ use different array indexing.
 * In Fortran indices are based on one, in C on zero.
 * This means the first element in an array has index 1 in Fortran
 * and index 0 in C.
 *
 * If the @c RCM_FORTRAN_INDICES flag is selected, an index 1 is
 * interpreted to point to the first element of an array.
 *
 * @remark
 * Internally all indices are zero based, as it is natural for
 * a C program. @c RCM_FORTRAN_INDICES covers only input/output
 * data. The use of pointer arithmetic (following the way
 * <a href="http://www.netlib.org/f2c/">f2c</a>
 * converts arrays from Fortran to C) allows the use of Fortran
 * style indices without any slow down.
 */
#define RCM_FORTRAN_INDICES 1

/**
 * @def RCM_C_INDICES
 * @brief Use C style array indices. This is the default setting.
 *
 * Using C style array indices is the default and there is no
 * need to specify it. Of the two indexing related flags
 * (@c RCM_FORTRAN_INDICES and @c RCM_C_INDICES),
 * @c #RCM_FORTRAN_INDICES takes the precedence, i.e. if both
 * flags are set, Fortran style indices are used and no warning or
 * error message is produced.
 *
 * The main purpose of @c RCM_C_INDICES is to help writing
 * a Fortran interface. In a Fortran interface, using Fortran style
 * indices would be the default and @c RCM_C_INDICES could be used
 * to select C indices instead.
 *
 * @see RCM_FORTRAN_INDICES
 */
#define RCM_C_INDICES 2

/**
 * @def RCM_NO_SORT
 * @brief Do not use any sorting in the Cuthill-McKee algorithm.
 * The default is to sort the nodes newly added to a level set from
 * the same adjacency list by non-decreasing degree.
 *
 * In the Cuthill-McKee algorithm we construct a level set by scanning
 * through the previous one. Adjacent nodes not yet used in this or any
 * prior level set are added to the new one. The nodes added from
 * the same level set are ordered by their degree.
 * This sorting is suppressed if the @c RCM_NO_SORT flag is set.
 *
 */
#define RCM_NO_SORT 4

/**
 * @def RCM_INSERTION_SORT
 * @brief Use linear insertion sort instead of heapsort.
 *
 * Changing the sorting routine does not only change the
 * execution time, but can also change the permutation found.
 * This is due to the fact that heapsort is not stable,
 * i.e. nodes with the same degree may be exchanged by
 * heapsort. On the other hand insertion sort kepp such
 * nodes in their original order.
 * Heapsort was choosen as the default sorting algorithm
 * because it is much faster in the worst case and there
 * are also real word examples showing a significant
 * improvement in execution time over insertion sort.
 * Finding a (symmetric) RCM ordering of the
 * <a href="http://www.stanford.edu/~sdkamvar/research.html#Data"
 * >Stanford Web Matrix</a> is more than 15 times
 * faster if heapsort is used instead of insertion sort.
 *
 * The SPARSPAK implementation of RCM uses linear insertion sort
 * and hence the @c RCM_INSERTION_FLAG can be used to get the exact
 * same permutation SPARSPAK would find. This is mostly useful
 * for the purpose of testing and comparison.
 *
 */
#define RCM_INSERTION_SORT 8

/**
 * @def RCM_NO_REVERSE
 * @brief Compute just a Cuthill-McKee ordering and skip the reversal.
 */
#define RCM_NO_REVERSE 16

/**
 * @def RCM_USE_MASK
 * @brief Use the values found in the @a mask arrays to filter
 * vertices of the graph. Work only on nodes with a non-zero
 * mask value.
 */
#define RCM_USE_MASK 32


/**
 * @def RCM_PREFIX
 * @brief Common prefix to be used for all functions declared in @c rcm.h,
 * defaults to an empty definition (i.e., no prefix).
 *
 * Common prefix to be used for all functions declared in @c rcm.h.
 * @c RCM_PREFIX is only defined if it is not defined before.
 * A previous definition is left untouched.
 * The default definition of @c RCM_PREFIX is empty, i.e., no
 * prefix is attached to function names.
 * This coincides with the behavior of @c rcm.c.
 * The same definition for @c RCM_PREFIX must be used for
 * @c rcm.h and @c rcm.c.
 */
#ifndef RCM_PREFIX
# define RCM_PREFIX /* empty */
#endif

/** @cond no-doxygen */

#ifndef CAT_2
# define DIRECT_CAT_2(a,b) a ## b
# define CAT_2(a,b) DIRECT_CAT_2(a,b)
#endif

#define RCM_FUNC(name) CAT_2(RCM_PREFIX,name)

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

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
     * @param[in] flags  are  used  to  select  variations of the
     *               way RCM works.
     *               Use @c 0 if no flag should be set. Otherwise use the
     *               bitwise
     *               OR of one or more of the supported flags:
     *               @c #RCM_FORTRAN_INDICES,
     *               @c #RCM_C_INDICES,
     *               @c #RCM_NO_SORT,
     *               @c #RCM_INSERTION_SORT,
     *               @c #RCM_NO_REVERSE and
     *               @c #RCM_USE_MASK.
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
    extern
    void RCM_FUNC(genrcmi)(const int n , const int flags,
                           const int *xadj, const int *adj,
                           int *perm,
                           signed char *mask, int *deg);

    /**
     * @brief Version of @c genrcmi() for @c long data.
     */
    extern
    void RCM_FUNC(genrcml)(const long n, const int flags,
                           const long *xadj, const long *adj,
                           long *perm,
                           signed char *mask, long *deg);

    /**
     * @brief Find a pseudo-peripheral node for the subgraph specified
     * by @a root.
     * 
     * Find a pseudo-peripheral node for the subgraph specified
     * by @a root.
     * 
     * @param[in] flags
     *               are used to select variations of  the  way  RCM  works.
     *               Use  @c 0 if no flag should be set. Otherwise use the
     *               bitwise OR of one or more @c RCM_* flags. @c fnrooti()
     *               supports the @c #RCM_FORTRAN_INDICES flag.
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

    extern
    void RCM_FUNC(fnrooti)(int *root, const int flags,
                           const int *xadj, const int *adj,
                           const int *deg,
                           int *ccsize,
                           signed char *mask, int *ls);

    /**
     * @brief Version of @c fnrooti() for @c long data.
     */
    extern
    void RCM_FUNC(fnrootl)(long *root, const int flags,
                           const long *xadj, const long *adj,
                           const long *deg,
                           long *ccsize,
                           signed char *mask, long *ls);

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
     * @param[in] flags
     *               changes the way the function works.
     *               Use the value @c 0 to not set any flag.
     *               Multiple flags can be combined by using binary OR.
     *               The following flags are supported:
     *               @c #RCM_FORTRAN_INDICES,
     *               @c #RCM_NO_REVERSE,
     *               @c #RCM_NO_SORT,
     *               @c #RCM_INSERTION_SORT
     *               and
     *               @c #RCM_USE_MASK.
     *               Other @c RCM_* flags are silently ignored.
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

    extern
    void RCM_FUNC(rcmi)(const int root, const int flags,
                        const int *xadj, const int *adj,
                        const int *deg,
                        signed char *mask,
                        int *perm, int *ccsize);

    /**
     * @brief Version of @c rcmi() for @c long data.
     */
    extern
    void RCM_FUNC(rcml)(const long root, const int flags,
                        const long *xadj, const long *adj,
                        const long *deg,
                        signed char *mask,
                        long *perm, long *ccsize);

#ifdef __cplusplus
}
#endif
#endif /* RCM__H */
