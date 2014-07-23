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

/*
 *-------1---------2---------3---------4---------5---------6---------7--
 *3456789-123456789-123456789-123456789-123456789-123456789-123456789-12
 */

/**
 * @mainpage
 *
 * @section The reverse Cuthill-McKee algorithm
 *
 *<ul>
 *<li><b>input:</b> an undirected graph @a G</li>
 *<li>unmask all nodes</li>
 *<li>find a not masked node @a p</li>
 *<li>for the connected component @a C rooted by @a p
 *  <ul>
 *    <li>Find pseudo-peripheral node @a root in @a C using @c fnrooti().
 *        See [3] and [4] for more details, the implementation in [4]
 *        is also presented in [2].
 *    </li>
 *    <li>
 *    Find the Cuthill-McKee ordering for the component rooted
 *    by @a root. See [1] for more detils, [2] presents the
 *    SPARSPAK implementation .
 *    </li>
 *    <li>
 *    Reverse the ordering (see [5]).
 *    </li>
 *  </ul>
 *</li>
 *</ul>
 *
 * @section ref References
 *
 *
 * [1] E.&nbsp;Cuthill and J.&nbsp;McKee.
 * <b><a href="http://portal.acm.org/citation.cfm?id=805928"
 * style="color:black;">Reducing the bandwidth of sparse symmetric
 * matrices</a></b>.
 * In <em>Proceedings of the 1969 24th national conference</em>, pages
 * 157-172, New York, NY, USA, 1969. ACM Press.
 *
 * [2] Alan George and Joseph W.&nbsp;H. Liu.
 * <b>Computer solution of large sparse positive definite systems</b>.
 * <em>Prentice-Hall series in computational mathematics</em>. Prentice-Hall,
 * Englewood Cliffs, NJ, USA, 1981.
 *
 * [3] N.&nbsp;E. Gibbs, W.&nbsp;G. Poole, and P.&nbsp;K. Stockemeyer.
 * <b>An algorithm for reducing the bandwidth and profile of a sparse
 * matrix</b>.
 * <em>SIAM Journal of Numerical Analysis</em>, 13(2):236-250, April 1976.
 *
 * [4] Alan George and Joseph W.&nbsp;H. Liu.
 * <b><a href="http://doi.acm.org/10.1145/355841.355845"
 * style="color:black;">An
 * implementation of a pseudoperipheral node finder</a></b>.
 * <em>ACM Trans. Math. Softw.</em>, 5(3):284-295, 1979.
 *
 * [5] Wai-Hung Liu and Andrew&nbsp;H. Sherman.
 * <b>Comparative analysis of the Cuthill-McKee and the reverse
 * Cuthill-McKee ordering algorithms for sparse matrices</b>.
 * <em>SIAM Journal on Numerical Analysis</em>, 13(2):198-213, April 1976.
 *
 */

/**
 * @file rcm.c
 *
 * Version 0.9.1. September 24, 2005.
 *
 * @section compile How to compile rcm.c
 *
 * Basically rcm.c should compile without any problem on any modern
 * ANSI C compiler. Some preprocessor magic ensures that this source
 * file is self sufficient and can be compiled without any additional
 * source files except standard header files.
 *
 * Several preprocessor defines can be used to influence the compilation:
 *
 * <dl>
 * <dt>@c #RCM_PREFIX</dt>
 * <dd>
 *   All external symbols get prefixed with @c RCM_PREFIX, it if is
 *   defined.
 *   Notice that if you use @c RCM_PREFIX, you must also define
 *   it to the same definition before you
 *   include  @c rcm.h, otherwise the function declarations
 *   will not match.
 * </dt>
 * <dt>@c DEBUG [default: no debug code]</dt>
 *   <dd>if the @c DEBUG symbol is defined,
 *   additional checks are included in the functions.
 *   These checks are only useful to test errors in
 *   the rcm code. If compiled with @c DEBUG, the header files
 *     @c <assert.h> and
 *     @c <stdio.h>
 *   have to be available.
 * </dd>
 * <dt>@c NDEBUG</dt>
 * <dd>Normally some assertion code is compiled into the functions.
 *     This can be surpressed by defining @c NDEBUG.
 * </dd>
 * <dt>@c NO_LIMITS_H [default: load @c <limits.h>]</dt>
 *   <dd>Your compiler does not have a
 *   working @c <limits.h>
 *   header file, and thus we are not going to include it.
 *   The @c <limits.h> header file is used to check if
 *   <code>INT_MAX==LONG_MAX</code>.
 *   If we have <code>INT_MAX==LONG_MAX</code>,
 *   the @c long version of each entry
 *   function will simply call its @c int counterpart.
 *   @n
 *   If @c NO_LIMITS_H is defined this check can not be performed
 *   and we are always compiling a special @c long version, even if
 *   @c int and @c long are identical.
 *   </dd>
 * <dt>@c NO_HEADERS [default: load @c <stdlib.h> and @c <string.h>]</dt>
 * <dd>
 *   If set, no header file will be included.
 *   The default case is to load
 *   @c <stdlib.h> and
 *   @c <string.h>
 *   in any case
 *   and other header files as needed, e.g. @c <assert.h> for debugging
 *   or standard assertion code.
 *   @n
 *   Now, if @c NO_HEADERS is defined, functionality depending on
 *   header files is not available. This includes any assertions,
 *   debugging code and reduced code size if @c int and @c long are
 *   equal.
 *  </dd>
 * </dl>
 *
 *
 */


#ifndef RCM__DRIVER
#define RCM__DRIVER 1


#define SELF "rcm.c"


#define RCM_FORTRAN_INDICES 1
#define RCM_C_INDICES 2
#define RCM_NO_SORT 4
#define RCM_INSERTION_SORT 8
#define RCM_NO_REVERSE 16
#define RCM_USE_MASK 32


/**
 * @def restrict
 * @brief Support for `restrict'
 */
#ifdef __GNUC__
#  define restrict __restrict__
#elif defined(__cplusplus)
#  define restrict
#elif (!defined(__STDC_VERSION__)) || (__STDC_VERSION__ < 199901L)
#  define restrict
#endif

/* Support for `inline' */
#ifndef __cplusplus
# ifdef __GNUC__
#  define inline __inline__
# else
#  define inline
# endif
#endif



#if defined(DEBUG) || !defined(NDEBUG)
# ifdef NO_HEADERS
#  define assert(expr) (void)0
# else
#  include <assert.h>
# endif
#endif

#define ASSERT assert

#ifdef DEBUG
#  define CHECK assert
#else
#  define CHECK(expr) (void)0
#endif



#ifndef NO_HEADERS
# include <stdlib.h>
# include <string.h>
#endif


#undef LONG_EQUALS_INT
#if !( defined(NO_LIMITS_H) || defined(NO_HEADERS) )
# include <limits.h>
# if INT_MAX == LONG_MAX
#  define LONG_EQUALS_INT 1
# endif
#endif



#ifndef CAT_2
# define DIRECT_CAT_3(a,b,c) a ## b ## c
# define CAT_3(a,b,c) DIRECT_CAT_3(a,b,c)
# define DIRECT_CAT_2(a,b) a ## b
# define CAT_2(a,b) DIRECT_CAT_2(a,b)
#endif

#ifndef RCM_PREFIX
# define RCM_FUNC2(name,suffix) CAT_2(name,suffix)
#else
# define RCM_FUNC2(name,suffix) CAT_3(RCM_PREFIX,name,suffix)
#endif
#define RCM_FUNC(name) RCM_FUNC2(name,SUFFIX)


#undef SUFFIX


/*
 * int versions
 */

#undef INT
#define INT int


#define SUFFIX i
#include SELF
#undef SUFFIX


/*
 * long version
 */

#if LONG_EQUALS_INT

void RCM_FUNC2(genrcm,l)(const long n, const int flags,
                         const long *xadj, const long *adj,
                         long *perm, signed char *mask, long *deg)
{
    CHECK(sizeof(int) == sizeof(long));
    RCM_FUNC2(genrcm,i)((int) n, flags, (int*) xadj, (int*) adj,
                        (int*) perm, mask, (int*) deg);
}

void RCM_FUNC2(fnroot,l)(long *root, const int flags,
                         const long *xadj, const long *adj,
                         const long *deg, 
                         long *ccsize, signed char *mask, long *ls)
{
    CHECK(sizeof(int) == sizeof(long));
    RCM_FUNC2(fnroot,i)((int*) root, flags, (int*) xadj, (int*) adj,
                        (int*) deg, (int*) ccsize, mask, (int*) ls);
}

void RCM_FUNC2(rcm,l)(const long root, const int flags,
                      const long *xadj, const long *adj,
                      const long *deg,
                      signed char *mask,
                      long *perm, long *ccsize)
{
    CHECK(sizeof(int) == sizeof(long));
    RCM_FUNC2(rcm,i)((int) root, flags, (int*) xadj, (int*) adj,
                     (int*) deg, mask, (int*) perm, (int*) ccsize);
}


#else /* LONG_EQUALS_INT */

#undef INT
#define INT long


#define SUFFIX l
#include SELF
#undef SUFFIX

#endif /* else part of LONG_EQUALS_INT */


#else /* RCM__DRIVER */

/** @cond no-doxygen */

/* function names and forward declarations */

#define GENRCM  RCM_FUNC(genrcm)
#define DEGREE  RCM_FUNC(degree)
#define FNROOT  RCM_FUNC(fnroot)
#define ROOTLS  RCM_FUNC(rootls)
#define RCM     RCM_FUNC(rcm)
#define HEAPSORT        RCM_FUNC(heapsort)
#define INSERTION_SORT  RCM_FUNC(insertion_sort)
#define SIFT    RCM_FUNC(sift)


void FNROOT(INT *root, const int flags,
            const INT *xadj, const INT *adj,
            const INT *deg, 
            INT *ccsize, signed char *mask, INT *ls);

void RCM(const INT root, const int flags,
         const INT *xadj, const INT *adj,
         const INT *deg,
         signed char *mask,
         INT *perm, INT *ccsize);
/*** @endcond */




/**
 * @brief Calculates the degree for all nodes in the graph.
 *
 * Calculates the degree for all nodes in the graph.
 *
 * @param[in] n number of nodes
 * @param[in] flags binary or'ed flags.
 *     <dl>
 *       <dt>@c RCM_FORTRAN_INDICES</dt><dd>for 1-based indices</dd>
 *       <dt>@c RCM_USE_MASK</dt><dd>take care of outmasked nodes</dd>
 *     </dl>
 *
 * @param[in] xadj pointers to the adjacency lists
 * @param[in] adj adjacency lists
 * @param[in] mask used with @c RCM_USE_MASK to mask out nodes from the graph.
 * @param[out] deg on output <code>deg[p]</code> contains the
 *     degree of node @c p.
 *     if <code>RCM_USE_MASK</code> is not set, all degrees are calculated,
 *     regardless of entries in the mask array.
 */
inline static
void DEGREE(const INT n, const int flags,
            const INT *restrict xadj, const INT *restrict adj,
            signed char *restrict mask, INT *restrict deg)
{
    INT i, k;
    INT nbr;
    INT ndeg;

    int base = (flags & RCM_FORTRAN_INDICES) ? 1 : 0;
    int fast_deg = !(flags & RCM_USE_MASK);

    if (fast_deg) {
        for (i = 0; i < n; ++i) {
            deg[i] = xadj[i+1] - xadj[i];
        }
    } else {
        if (flags & RCM_FORTRAN_INDICES) {
            --xadj;
            --adj;
            --mask;
            --deg;
        }
        for (i = base; i < n+base; ++i) {
            if (!mask[i]) {
                CHECK( !fast_deg );
                ndeg = 0;
                for (k = xadj[i]; k < xadj[i+1]; ++k) {
                    nbr = adj[k];
                    if (!mask[nbr] && nbr != i) {
                        ++ndeg;
                    }
                }
                ASSERT( ndeg == xadj[i+1]-xadj[i] );
                deg[i] = ndeg;
            }
        }
    }
}


/**
 * @brief Generic @c genrcmX() code (see @c genrcmi()).
 *
 * Generic @c genrcmX() code (see @c genrcmi()).
 * @n
 * <i>The following documentation is copied from @c genrcmi():</i>
 *
 * @copydoc genrcmi()
 */
void GENRCM(const INT n, const int flags,
            const INT *xadj, const INT *adj,
            INT *perm, signed char *mask, INT *deg)
{
    INT i, root, ccsize;
    INT num = 0;
    int xflags = flags;
    int base = (flags & RCM_FORTRAN_INDICES) ? 1 : 0;

    if (!(xflags & RCM_USE_MASK)) {
        for (i = 0; i < n; ++i) {
            mask[i] = 0;
        }
        xflags &= (~RCM_USE_MASK);
    }
    DEGREE(n, xflags, xadj, adj, mask, deg);

    for (i = 0; i < n; ++i) {
        if (mask[i]) {
            continue;
        }
	root = i + base;

        /*
         * Find a pseudo-peripheral node and calculate RCM.
         * Note that we reuse perm to also store the level
         * structure (perm+num is parameter ls of FNROOT()).
         */
	FNROOT(&root, xflags, xadj, adj, deg,
               &ccsize,
               mask, perm+num);
	RCM(root, xflags,
            xadj, adj, deg, mask, perm+num, &ccsize);

        num += ccsize;
        CHECK( num <= n );
	if (num >= n) {
            break;
        }

    }

}



/**
 * @brief Generates rooted level structure.
 *
 * Generates rooted level structure.
 * Only those nodes for
 * which @a mask is nonzero will be considered.
 *
 * @param[in] root
 *            the node at which the level structure is to
 *            be rooted
 * @param[in] flags
 *            tailors behavior. Use @c 0 to not set any flag.
 *            @c ROOTLS() supports the @c #RCM_FORTRAN_INDICES flag.
 * @param[in] xadj
 *            pointers to the adjacency lists in @a adj
 * @param[in] adj
 *            adjacency lists
 * @param[in,out] mask
 *            nodes @a p with @a mask[@a p] not zero are
 *            considered masked out.
 *            @a mask is modified, but restored to the original
 *            values on output.
 * @param[out] ccsize
 *            size of the component rooted by @a root
 * @param[out] nlvl
 *            number of levels found
 * @param[out] last_lvl
 *            pointer to the beginning of the last level in @a ls
 * @param[out] ls
 *            nodes in the level sets, ordered by level sets.
 *            The last level is stored in @a ls[@a k], where
 *            @a last_lvl <= @a k < @a ccsize.
 *
 */

inline static
void ROOTLS(const INT root, const int flags,
            const INT *restrict xadj, const INT *restrict adj,
            signed char *restrict mask,
            INT *restrict ccsize, INT *restrict nlvl,
            INT *restrict last_lvl, INT *restrict ls)
{
    INT i, j;
    INT nbr, node;

    INT lvl_begin, lvl_end; /* C style indices */

    if (flags & RCM_FORTRAN_INDICES) {
        --mask;
        --xadj;
        --adj;
    }
    
    ASSERT( !mask[root] );
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

/**
 * @brief Generic @c fnrootX() code (see @c fnrooti()).
 *
 * Generic @c fnrootX() code (see @c fnrooti()).
 * @n
 * <i>The following documentation is copied from @c fnrooti():</i>
 *
 * @copydoc fnrooti()
 */
void FNROOT(INT *restrict root, const int flags,
            const INT *restrict xadj, const INT *restrict adj,
            const INT *restrict deg,
            INT *restrict ccsize,
            signed char *restrict mask, INT *restrict ls)
{
    INT j, new_nlvl, last_lvl, node, ndeg, mindeg;
    INT nlvl = 1;

    if (flags & RCM_FORTRAN_INDICES) {
        --deg;
    }

    nlvl = 1;

    while (1) {
        ROOTLS(*root, flags, xadj, adj,
               mask,
               ccsize, &new_nlvl, &last_lvl,
               ls);
        /*
         * Return if the number of levels stays the same
         * or if we have reached the maximum level count.
         * The number of levels can not decrease.
         */
        CHECK( new_nlvl > 0 );
        CHECK( new_nlvl >= nlvl );
        CHECK( new_nlvl <= *ccsize );
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


/**
 * @brief Sifts down a heap element, helper for @c HEAPSORT().
 *
 * Helper function for @c HEAPSORT().
 * Sifts down the value @a t in the heap between @a i and <i>n</i>-1.
 *
 */
static
void SIFT(const INT i, const INT n, const INT t, INT *restrict perm,
          const INT *restrict deg)
{
    INT j, w;
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


/**
 * @brief Heapsort for RCM; sorts after the degree of nodes.
 *
 * Heapsort function to sorts an array.
 * This version is tailored for the RCM algorithm and sort
 * the entries in @a perm after their degree (stored in @a deg).
 * The @c #RCM_INSERTION_SORT flag controls if @c HEAPSORT()
 * or @c INSERTION_SORT() is used for sorting.
 *
 * @param[in] sz        size of the array
 * @param[in,out] perm  data to be sorted
 * @param[in] deg       degrees of the nodes
 *
 */
inline static
void HEAPSORT(const INT sz, INT *restrict perm,
              const INT *restrict deg)
{
    INT n = sz;
    INT i, t;

    for (i = n/2; i > 0;) {
        --i;
        SIFT(i, n, perm[i], perm, deg);
    }
    for (; n > 1;) {
        --n;
        t = perm[n];
        perm[n] = perm[0];
        SIFT(i, n, t, perm, deg);
    }
}

/**
 * @brief Linear insertion sort for RCM; sorts after the degree of nodes.
 *
 * Linear insertion sort function for RCM.
 * It sorts the entries in @a perm after their degree (stored in @a deg).
 * The @c #RCM_INSERTION_SORT flag controls if @c HEAPSORT()
 * or @c INSERTION_SORT() is used for sorting.
 *
 * @param[in] sz        size of the array
 * @param[in,out] perm  data to be sorted
 * @param[in] deg       degrees of the nodes
 *
 */
inline static
void INSERTION_SORT(const INT sz, INT *restrict perm,
                const INT *restrict deg)
{
    INT k, l;
    INT nbr;

    for (k = sz-1; k > 0; --k) {
        nbr = perm[k-1];
        for (l = k; l < sz && deg[perm[l]] < deg[nbr]; ++l) {
            perm[l-1] = perm[l];
        }
        perm[l-1] = nbr;
    }
}


 
/**
 * @brief Generic @c rcmX() code (see @c rcmi()).
 *
 * Generic @c rcmX() code (see @c rcmi()).
 * @n
 * <i>The following documentation is copied from @c rcmi():</i>
 *
 * @copydoc rcmi()
 */
void RCM(const INT root, const int flags,
         const INT * restrict xadj, const INT * restrict adj,
         const INT * restrict deg,
         signed char * restrict mask,
         INT * restrict perm, INT * restrict ccsize)
{
    INT i, j;
    INT nbr, node;

    INT lvl_begin, lvl_end;
    INT fnbr, lnbr;

    int base = (flags & RCM_FORTRAN_INDICES) ? 1 : 0;
    int sort = !(flags & RCM_NO_SORT);

    /*
     * Fill the first level set with the root node,
     * and initialize lvl_begin, lvl_end and lnbr
     * accordingly.
     * Adjust the arrays if Fortran style indices are used
     * for xadj, adj and perm.
     */

    if (flags & RCM_FORTRAN_INDICES) {
        --xadj;
        --adj;
        --mask;
        --perm;
        --deg;
    }

    perm[base] = root;
    ASSERT( !mask[root] );
    mask[ root ] = -1;

    lvl_begin = 0 + base;
    lvl_end = 1 + base;
    lnbr = lvl_end;

    /*
     * Iterate over all level sets. lvl_begin and lvl_end point to the
     * beginning and behind the end of the current level respectively.
     */
    do {
        /* iterate over all nodes in the current level */
        for (i = lvl_begin; i < lvl_end; ++i) {
            node = perm[i];
            CHECK( mask[node] == -1 );
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
            if (lnbr - fnbr > 1 && sort) {
                if ((flags & RCM_INSERTION_SORT)) {
                    INSERTION_SORT(lnbr-fnbr, perm+fnbr, deg);
                } else {
                    HEAPSORT(lnbr-fnbr, perm+fnbr, deg);
                }
            }
        }
        lvl_begin = lvl_end;
        lvl_end = lnbr;
    } while (lvl_end - lvl_begin > 0);

    ASSERT(lnbr > 0); /* check that no overflow occured */

    /* we now have a Cuthill-McKee ordering in perm */

    if (flags & RCM_FORTRAN_INDICES) {
        ++perm;
        --lnbr;
    }
    *ccsize = lnbr;

    /* reverse perm if reversal is not explicitly unwanted */
    if (!(flags & RCM_NO_REVERSE)) {
        j = lnbr - 1;
        for (i = 0; i < lnbr / 2; ++i, --j) {
            node = perm[j];
            perm[j] = perm[i];
            perm[i] = node;
        }
    }
}


#endif /* RCM__DRIVER */
