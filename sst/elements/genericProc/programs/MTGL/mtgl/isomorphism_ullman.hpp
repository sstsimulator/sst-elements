/*  _________________________________________________________________________
 *
 *  MTGL: The MultiThreaded Graph Library
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top MTGL directory.
 *  _________________________________________________________________________
 */

/****************************************************************************/
/*! \file isomorphism_ullman.hpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 10/6/2009
*/
/****************************************************************************/

#ifndef MTGL_ISOMORPHISM_ULLMAN
#define MTGL_ISOMORPHISM_ULLMAN

#include <cstdio>
#include <iomanip>
#include <limits>
#include <sys/time.h>

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>

#ifdef __MTA__
  #include <sys/mta_task.h>
  #include <machine/runtime.h>
#endif

#define DEBUG_Trace_ullman 0

namespace mtgl
{

class default_isomorph_visitor
{
public:

  virtual bool
  operator()(int n, size_t ni1[], size_t ni2[]) { return false; }
};

#if 0
template<typename dataType, typename dimType>
class mt_array2d {
public:
  // Default Constructor
  mt_array2d()
    {
    shareCount = NULL;
    nRows = 0;
    nCols = 0;
    data  = NULL;
    }
  // Constructor
  mt_array2d(dimType num_rows, dimType num_cols)
    {
    allocate(num_rows, num_cols);
    }
  // Copy Constructor
  mt_array2d( mt_array2d<dataType,dimType> &other )
    {
    allocate(other.nRows, other.nCols);
    for(dimType irow=0; irow<nRows; irow++)
      {
      #pragma mta assert parallel
      for(dimType icol=0; icol<nCols; icol++)
        data[irow][icol] = other.data[irow][icol];
      }
    }
  // Destructor
  ~mt_array2d()
    {
    if(shareCount==NULL) return;
    else
      {
      (*shareCount)--;
      if(*shareCount==0)
        {
        #pragma mta assert parallel
        for(int irow=0; irow<nRows; irow++)
          if( data[irow] )
            delete [] data[irow];
        delete [] data;
        delete shareCount;
        shareCount = NULL;
        }
      }
    }
  // allocate
  void allocate(dimType num_rows, dimType num_cols)
    {
    shareCount = new int;
    *shareCount = 1;
    nRows = num_rows;
    nCols = num_cols;
    data = new dataType *[nRows];
    #pragma mta assert parallel
    for(dimType irow=0; irow<nRows; irow++)
      data[irow] = new dataType[nCols];
    }
  // shallow_copy
  void shallow_copy( mt_array2d<dataType,dimType>&other )
    {
    shareCount = other.shareCount;
    nRows = other.nRows;
    nCols = other.nCols;
    data  = other.data;
    (*shareCount)++;
    }
  // operator[]
  dataType * operator [](dimType i)
    {
    return data[i];
    }
  // reset
  void reset(dataType value)
    {
    #pragma mta assert parallel
    for(dimType irow=0; irow<nRows; irow++)
      #pragma mta assert parallel
      for(dimType icol=0; icol<nCols; icol++)
        data[irow][icol] = value;
    }
  dataType get(dimType row, dimType col)
    {
    assert(row >= 0 && row < nRows);
    assert(col >= 0 && col < nCols);
    return( data[row][col] );
    }
  void set(dimType row, dimType col, dataType value)
    {
    assert(row >= 0 && row < nRows);
    assert(col >= 0 && col < nCols);
    data[row][col] = value;
    }
  dataType** ptr(void) { return data; }
  void print_self(int indent_num=0)
    {
    std::string indt;
    for(int i=0; i<indent_num; i++)
      {
      indt += std::string("\t");
      }
    for(dimType irow=0; irow<nRows; irow++)
      {
      std::cout << indt << "[ ";
      for(dimType icol=0; icol<nCols; icol++)
        {
        assert( data[irow] );
        std::cout << data[irow][icol] << " ";
        }
      std::cout << "]" << std::endl;
      }
    }
  // TODO: Add some locking capabilities?  (locking on row, col, or entry)
  // TODO: Validate parallelism...
private:
  dataType ** data;
  dimType     nRows;
  dimType     nCols;
  int      *  shareCount;
};
#endif

#if 1
// allocates a single buffer and indexes data into it as a 2d array
template<typename dataType, typename dimType>
class mt_array2d
{
public:
  // Default Constructor
  mt_array2d()
    {
    shareCount = NULL;
    nRows = 0;
    nCols = 0;
    data  = NULL;
    }
  // Constructor
  mt_array2d(dimType num_rows, dimType num_cols)
    {
    allocate(num_rows, num_cols);
    }
  // Copy Constructor
  mt_array2d( mt_array2d<dataType,dimType> &other )
    {
    allocate(other.nRows, other.nCols);
    for(dimType irow=0; irow<nRows; irow++)
      {
      #pragma mta assert parallel
      for(dimType icol=0; icol<nCols; icol++)
        data[irow][icol] = other.data[irow][icol];
      }
    }
  // Destructor
  ~mt_array2d()
    {
    if(shareCount==NULL) return;
    else
      {
      (*shareCount)--;
      if(*shareCount==0)
        {
        free( internal_buffer );
        internal_buffer = NULL;
        delete [] data;
        data = NULL;
        delete shareCount;
        shareCount = NULL;
        }
      }
    }

  // allocate
  void allocate(dimType num_rows, dimType num_cols)
    {
    shareCount = new int;
    *shareCount = 1;
    nRows = num_rows;
    nCols = num_cols;
    internal_buffer = (dataType*)malloc(sizeof(dataType)*nRows*nCols);
    data = new dataType *[nRows];
    #pragma mta assert parallel
    for(dimType irow=0; irow<nRows; irow++)
      {
      data[irow]   = &internal_buffer[irow*nCols];
      }
    }

  // shallow_copy
  void shallow_copy( mt_array2d<dataType,dimType>&other )
    {
    shareCount = other.shareCount;
    nRows = other.nRows;
    nCols = other.nCols;
    data  = other.data;
    (*shareCount)++;
    }

  // operator[]
  dataType * operator [](dimType i)
    {
    return data[i];
    }

  // reset
  void reset(dataType value)
    {
    default_value = value;
    #pragma mta assert parallel
    for(dimType irow=0; irow<nRows; irow++)
      #pragma mta assert parallel
      for(dimType icol=0; icol<nCols; icol++)
        data[irow][icol] = value;
    }

  dataType get(dimType row, dimType col)
    {
    assert(row >= 0 && row < nRows);
    assert(col >= 0 && col < nCols);
    return( data[row][col] );
    }

  void set(dimType row, dimType col, dataType value)
    {
    assert(row >= 0 && row < nRows);
    assert(col >= 0 && col < nCols);
    data[row][col] = value;
    }

  dataType** ptr(void) { return data; }

  void print_self(int indent_num=0)
    {
    std::string indt;
    for(int i=0; i<indent_num; i++)
      {
      indt += std::string("\t");
      }
    for(dimType irow=0; irow<nRows; irow++)
      {
      std::cout << indt << "[ ";
      for(dimType icol=0; icol<nCols; icol++)
        {
        assert( data[irow] );
        std::cout << data[irow][icol] << " ";
        }
      std::cout << "]" << std::endl;
      }
    }

private:
  dataType ** data;
  dataType  * internal_buffer;
  dataType    default_value;
  dimType     nRows;
  dimType     nCols;
  int      *  shareCount;
};
#endif


template<typename Graph, typename match_visitor = default_isomorph_visitor>
class ullman_state
{
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;
  typedef typename graph_traits<Graph>::vertex_iterator viter_t;

  ullman_state(Graph& Gbeta, Graph& Galpha, match_visitor& vis);
  ullman_state(ullman_state& other);
  ~ullman_state();

  bool is_goal() { return core_len == num_verts_1; }
  bool is_dead();
  bool next_pair(size_t * N1, size_t * N2, size_t pre_N1, size_t pre_N2);
  bool is_feasible_pair(size_t N1, size_t N2);
  void add_pair(size_t N1, size_t N2);
  void get_core_set(size_t C1[], size_t C2[]);

  size_t get_core_len() { return core_len; }

  void refine(void);

  ullman_state * Clone();

  void print_self(int indent_num=0);

private:

  size_t null_node;

  Graph & G1;  // small graph (pattern)
  Graph & G2;  // large graph (target )

  size_t  num_verts_1;  // size of small graph  - n2
  size_t  num_verts_2;  // size of large graph  - n1

  size_t * coreset_1;
  size_t * coreset_2;
  size_t   core_len;

//  int ** M;
  mt_array2d<int,size_t> Matrix;

  match_visitor & iso_visitor; // Visitor class

  // helpers
  void print_graph(Graph& g);
  void print_array(size_t* A, size_t size, size_t nonValue);
  void print_array(int** A, int dim1, int dim2, int nonValue);
  bool has_edge(Graph & graph, size_t node1, size_t node2, vertex_id_map<Graph> & vid_map);

}; // !class ullman_state



/* Constructor */
template<typename Graph, typename match_visitor>
ullman_state<Graph,match_visitor>::
ullman_state(Graph & Gbeta, Graph & Galpha, match_visitor & vis)
  : G2(Gbeta), G1(Galpha), iso_visitor(vis)
{
#if DEBUG_Trace_ullman
  cout << "entr\tullman_state(Gbeta, Galpha, vis)" << endl;
#endif

  num_verts_1 = num_vertices(G1);
  num_verts_2 = num_vertices(G2);
  null_node   = (num_verts_1 > num_verts_2 ? num_verts_1 : num_verts_2) + 1;
  core_len    = 0;

  coreset_1 = new size_t[ num_verts_1 ];
  coreset_2 = new size_t[ num_verts_2 ];

  // allocate matrix
  Matrix.allocate(num_verts_1, num_verts_2);
#if 0
  M = new int *[ num_verts_1 ];
  for(size_t i=0; i<num_verts_1; i++)
    {
    M[i] = new int[num_verts_2];  // n2=num_verts_1
    assert(M[i]);
    }
#endif

  #pragma mta assert parallel
  for(size_t i=0; i<num_verts_1; i++)
    coreset_1[i] = null_node;

  #pragma mta assert parallel
  for(size_t i=0; i<num_verts_2; i++)
    coreset_2[i] = null_node;

  viter_t verts1 = vertices(G1);
  viter_t verts2 = vertices(G2);

  #pragma mta assert parallel
  for(size_t i=0; i<num_verts_1; i++)
    {
    vertex_t vertex_1  = verts1[i];
    size_t   v1_indeg  = in_degree(vertex_1, G1);
    size_t   v1_outdeg = out_degree(vertex_1, G1);

    #pragma mta assert parallel
    for(size_t j=0; j<num_verts_2; j++)
      {
      vertex_t vertex_2  = verts2[j];
      size_t   v2_indeg  = in_degree(vertex_2, G2);
      size_t   v2_outdeg = out_degree(vertex_2, G2);

      bool termIN  = v1_indeg <= v2_indeg;
      bool termOUT = v1_outdeg <= v2_outdeg;
      bool compat_bi_aj = true;  // COMPATIBILITY (ATTRIBUTE CHECK)  TODO!!!

      Matrix.set(i,j, (termIN && termOUT && compat_bi_aj) ? 1 : 0);
      }
    }

  assert(coreset_1);
  assert(coreset_2);
  // assert( M );
  //print_array(M, num_verts_2, num_verts_1, 0);
}  // end constructor


/* Copy Constructor */
template<typename Graph, typename match_visitor>
ullman_state<Graph,match_visitor>::
ullman_state(ullman_state& other)
  : G1(other.G1), G2(other.G2), iso_visitor(other.iso_visitor)
{
  null_node   = other.null_node;
  num_verts_1 = other.num_verts_1;
  num_verts_2 = other.num_verts_2;
  core_len    = other.core_len;

  coreset_1 = new size_t[ num_verts_1 ];
  coreset_2 = new size_t[ num_verts_2 ];

  Matrix.allocate(num_verts_1, num_verts_2);
  Matrix.reset(0);   // Optional

  assert(coreset_1);
  assert(coreset_2);

  #pragma mta assert parallel
  for(int i=0; i < num_verts_1; i++)
    {
    coreset_1[i] = other.coreset_1[i];
    }
  #pragma mta assert parallel
  for(int i=0; i < num_verts_2; i++)
    {
    coreset_2[i] = other.coreset_2[i];
    }
  #pragma mta assert parallel
  for(int i=core_len; i<num_verts_1; i++)   // Note: a triangular loop!
    {
    #pragma mta assert parallel
    for(int j=0; j<num_verts_2; j++)
      {
      Matrix.set(i,j, other.Matrix[i][j]);
      }
    }
} // end copy constructor


// destructor.... die die die!!!
template<typename Graph, typename match_visitor>
ullman_state<Graph,match_visitor>::
~ullman_state()
{
  // cleanup
  delete[] coreset_1;
  delete[] coreset_2;
}


/* next_pair */
template<typename Graph, typename match_visitor>
bool
ullman_state<Graph, match_visitor>::
next_pair(size_t * N1, size_t * N2, size_t pre_N1, size_t pre_N2)
{
#if DEBUG_Trace_ullman
  cout << "entr\tnext_pair()" << endl;
#endif
  if(pre_N1 == null_node)
    {
    pre_N1 = core_len;
    pre_N2 = 0;
    }
  else if(pre_N2==null_node)
    {
    pre_N2 = 0;
    }
  else
    {
    mt_incr(pre_N2,1);
    }
  if(pre_N2 >= num_verts_2)
    {
    mt_incr(pre_N1,1);
    pre_N2 = 0;
    }
  if(pre_N1 != core_len)
    {
    #if DEBUG_Trace_ullman
    cout << "exit\tnext_pair() = F" << endl;
    #endif
    return false;
    }
#if 1
  while( pre_N2 < num_verts_2 && Matrix[pre_N1][pre_N2]==0)
    {
    mt_incr(pre_N2,1);
    }
#else
  size_t min = num_verts_2;
  for(size_t i=pre_N2; i<num_verts_2; i++)
    {
    if( Matrix[pre_N1][i] != 0)
      {
      min = (i<min) ? i : min;
      }
    }
#endif

  if(pre_N2 < num_verts_2)
    {
    *N1 = pre_N1;
    *N2 = pre_N2;
    #if DEBUG_Trace_ullman
    cout << "exit\tnext_pair() = T" << endl;
    #endif
    return true;
    }
  #if DEBUG_Trace_ullman
  cout << "exit\tnext_pair() = F" << endl;
  #endif
  return false;
}


/* is_dead */
template<typename Graph, typename match_visitor>
bool
ullman_state<Graph, match_visitor>::
is_dead()
{
  #if DEBUG_Trace_ullman
  cout << "entr\tis_dead()" << endl;
  #endif
  if( num_verts_1 > num_verts_2 )
    {
    #if DEBUG_Trace_ullman
    cout << "exit\tis_dead() = T" << endl;
    #endif
    return true;
    }
  for(size_t i=core_len; i < num_verts_1; i++)
    {
    for(size_t j=0; j<num_verts_2; j++)
      {
      if(Matrix[i][j] != 0)
        {
        goto next_row;
        }
      }
    #if DEBUG_Trace_ullman
    cout << "exit\tis_dead() = T" << endl;
    #endif
    return true;
    next_row: ;
    }
  #if DEBUG_Trace_ullman
  cout << "exit\tis_dead() = F" << endl;
  #endif
  return false;
}


/* is_feasible_pair */
template<typename Graph, typename match_visitor>
bool
ullman_state<Graph, match_visitor>::
is_feasible_pair(size_t N1, size_t N2)
{
  #if DEBUG_Trace_ullman
  cout << "entr\tis_feasible_pair()" << endl;
  #endif
  assert( N1 < num_verts_1 );
  assert( N2 < num_verts_2 );
  return( Matrix[N1][N2] != 0 );
}


/* add_pair */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph,match_visitor>::
add_pair(size_t N1, size_t N2)
{
  #if DEBUG_Trace_ullman
  cout << "entr\tadd_pair()" << endl;
  #endif
  assert(N1 < num_verts_1);
  assert(N2 < num_verts_2);
  assert(core_len < num_verts_1);
  assert(core_len < num_verts_2);

  coreset_1[N1] = N2;
  coreset_2[N2] = N1;

  mt_incr(core_len,1);

  for(size_t k=core_len; k<num_verts_1; k++)
    {
    Matrix.set(k,N2,0);
    }

  refine();

  #if DEBUG_Trace_ullman
  cout << "exit\tis_feasible_pair()" << endl;
  #endif
}



/* refine */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph,match_visitor>::
get_core_set(size_t C1[], size_t C2[])
{
  #if DEBUG_Trace_ullman
  cout << "entr\tget_core_set()" << endl;
  #endif
  int j=0;
  for(int i=0; i<num_verts_1; i++)   // may be parallel
    {
    if(coreset_1[i] != null_node)
      {
      int tmp_j = mt_incr(j,1);
      C1[tmp_j] = i;
      C2[tmp_j] = coreset_1[i];
      }
    }
  #if DEBUG_Trace_ullman
  cout << "exit\tget_core_set()" << endl;
  #endif
}


/* refine */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph,match_visitor>::
refine(void)
{
  #if DEBUG_Trace_ullman
  cout << "entr\trefine()" << endl;
  #endif
  vertex_id_map<Graph> vid_map_1 = get(_vertex_id_map, G1);
  vertex_id_map<Graph> vid_map_2 = get(_vertex_id_map, G2);

  #if DEBUG_Trace_ullman
  this->print_self(2);
  #endif

  #pragma mta assert parallel
  for(size_t i=core_len; i < num_verts_1; i++)  // Parallel?
    {
    #pragma mta assert parallel
    for(size_t j=0; j < num_verts_2; j++)       // Parallel?
      {
      if( Matrix[i][j] )
        {
        //cout << "\t   M[" << i << "][" << j << "]" << endl;
        bool edge_ik, edge_ki, edge_jl, edge_lj;
        for(size_t k=core_len-1; k<core_len; k++)
          {
          size_t l = coreset_1[k];
          assert(l != null_node);
          edge_ik = has_edge( G1, i,k, vid_map_1 );  // (G1 = alpha) (pattern)
          edge_ki = has_edge( G1, k,i, vid_map_1 );
          edge_jl = has_edge( G2, j,l, vid_map_2 );  // (G2 = beta)  (target)
          edge_lj = has_edge( G2, l,j, vid_map_2 );

          #if DEBUG_Trace_ullman
          //cout << "\t\t G_a(i-" << i << ", k-" << k << ") ? " << edge_ik << endl;
          cout << "\t\t G_a(k-" << k << ", i-" << i << ") ? " << edge_ki << endl;
          //cout << "\t\t G_b(j-" << j << ", l-" << l << ") ? " << edge_jl << endl;
          cout << "\t\t G_b(l-" << l << ", j-" << j << ") ? " << edge_lj << endl;
          #endif

          bool compat_Bik_Ajl = true;   // TODO: Gb(i,k)<=>Ga(j,l) compatibility test
          bool compat_Aik_Bjl = true;   // TODO: Gb(i,k)<=>Ga(j,l) compatibility test
          bool compat_Bki_Alj = true;   // TODO: Gb(k,i)<=>Ga(l,j) compatibility test

#if 0
          if(!edge_ki && edge_lj)
            break;
          // testing --- > implement monomorphism
          else if(edge_ki != edge_lj )
            {
            Matrix.set(i,j,0);
            cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (1)" << endl;
            break;
            }
          else if(edge_ki && !compat_Aik_Bjl)
            {
            Matrix.set(i,j,0);
            cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (2)" << endl;
            break;
            }

//          if(!edge_ik && edge_jl)
//            break;
          // testing --- > implement monomorphism
//          else
          if(edge_ik != edge_jl )
            {
            Matrix.set(i,j,0);
            cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (1)" << endl;
            break;
            }
          else if(edge_ik && !compat_Aik_Bjl)
            {
            Matrix.set(i,j,0);
            cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (2)" << endl;
            break;
            }



          cout << endl;
#endif

#if 0
          // testing --- > implement monomorphism  (close)
          if(!edge_ki && edge_lj) break;  // disregard edges that aren't in Gp but are in G
          if(edge_ki != edge_lj)
            {
            Matrix.set(i,j,0);
            //cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (1)" << endl;
            break;
            }
          else if(edge_ki && !compat_Aik_Bjl)
            {
            Matrix.set(i,j,0);
            //cout << "\t\t\tSet[" << i << "][" << j << "] = 0  (2)" << endl;
            break;
            }
#endif
#if 1
          // orig ullman/vf code
          if(edge_ik != edge_jl || edge_ki != edge_lj)
            {
            //Matrix[i][j] = 0;
            Matrix.set(i,j, 0);
            break;
            }
          else if(edge_ik && !compat_Bik_Ajl)
            {
            // Matrix[i][j]=0;
            Matrix.set(i,j, 0);
            break;
            }
          else if(edge_ki && !compat_Bki_Alj)
            {
            Matrix.set(i,j, 0);
            //Matrix[i][j] = 0;
            break;
            }
#endif
          }
        }
      }
    }
  #if DEBUG_Trace_ullman
  cout << "exit\trefine()" << endl;
  #endif
}


/* Clone */
template<typename Graph, typename match_visitor>
ullman_state<Graph,match_visitor> *
ullman_state<Graph,match_visitor>::Clone()
{
  return new ullman_state(*this);
}


/* Helper method to print out a 2D array */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph, match_visitor>::
print_array(size_t* A, size_t size, size_t nonValue)
{
  cout << "\t[ ";
  for(int i=0; i<size; i++)
    {
    if(A[i]!=nonValue)
      cout << A[i] << " ";
    else
      cout << "- ";
    }
  cout << "]" << endl;
}


/* Helper method to print out a 2D array */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph, match_visitor>::
print_array(int** A, int dim1, int dim2, int nonValue)
{
  assert(A);
  for(int i=0; i<dim1; i++)
    {
    cout << "\t[ ";
    for(int j=0; j<dim2; j++)
      {
      //assert(A[i]);
      if(A[i][j]!=nonValue)
        cout << A[i][j] << " ";
      else
        cout << "- ";
      }
    cout << "]" << endl;
    fflush(stdout);
    }
}


/* Helper method to print out a 2D array */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph, match_visitor>::
print_self(int indent_num)
{
  std::string indnt;
  for(int i=0; i<indent_num; i++)
    {
    indnt += "\t";
    }
  cout << indnt << "----------------------------------------" << endl;
  cout << indnt << "ullman_state:" << endl;
  cout << indnt << "\tG1 num_verts = " << num_verts_1 << endl;
  cout << indnt << "\tG2 num_verts = " << num_verts_2 << endl;
  cout << indnt << "\tcoreset_1    = "; print_array(coreset_1, num_verts_1, null_node);
  cout << indnt << "\tcoreset_2    = "; print_array(coreset_2, num_verts_2, null_node);
  cout << indnt << "\tcore_len     = " << core_len    << endl;
  cout << indnt << "\tnull_node    = " << null_node   << endl;
  //print_array(M, num_verts_1, num_verts_2, 0);
  Matrix.print_self(indent_num);
  cout << indnt << "----------------------------------------" << endl;
  fflush(stdout);
}


/* Helper method to print out a graph.  */
template<typename Graph, typename match_visitor>
void
ullman_state<Graph, match_visitor>::print_graph(Graph& g)
  {
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  size_t num_verts = num_vertices(g);
  size_t num_edges = num_edges(g);

  viter_t verts = vertices(g);

  for (size_t vidx = 0; vidx < num_verts; vidx++)
    {
    vertex_t vi = verts[vidx];
    int deg = out_degree(vi, g);
    eiter_t inc_edges = out_edges(vi, g);
    cout << vidx << "\tout [" << setw(2) << deg << "] { ";

    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = inc_edges[adji];
      size_t other = get(vid_map, target(e, g));
      cout << other << " ";
      }
    cout << "}" << endl;

    deg = in_degree(vi, g);
    in_eiter_t in_inc_edges = in_edges(vi, g);
    cout << "\tin  [" << setw(2) << deg << "] { ";

    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = in_inc_edges[adji];
      size_t other = get(vid_map, source(e, g));
      cout << other << " ";
      }
    cout << "}" << endl;
    }
  }


/* has_edge */
template<typename Graph,typename match_visitor>
bool
ullman_state<Graph,match_visitor>::
has_edge(Graph & graph, size_t node1, size_t node2, vertex_id_map<Graph> & vid_map)
{
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  int ret = 0;

  if (node1 >= 0 && node2 >= 0 && node1 < num_vertices(graph) && node2 < num_vertices(graph))
    {
    const vertex_t u = get_vertex(node1, graph);
    const int deg = out_degree(u, graph);
    eiter_t inc_edges = out_edges(u, graph);

    int found = 0;

    if (deg > 20)
      {
      #pragma mta assert parallel
      for (int i = 0; i < deg; i++)
        {
        if (0 == found)
          {
          edge_t e = inc_edges[i];
          const size_t vidx = get(vid_map, target(e, graph));
          if (vidx == node2)
            {
            mt_incr(found, 1); // set flag so rest of xmt loops basically do nothing.
            mt_incr(ret, 1); // (atomic increment, set to true)
            }
          }
        }
      }
    else
      {
      for (int i = 0; (i < deg) && (0 == ret); i++)
        {
        edge_t e = inc_edges[i];
        const size_t vidx = get(vid_map, target(e, graph));
        if (vidx == node2)
          {
          mt_incr(ret, 1); // (atomic increment, set to true)
          }
        }
      }
    }
  return (0 != ret);
}



template<typename Graph, typename match_visitor = default_isomorph_visitor>
class isomorphism_ullman
{
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  isomorphism_ullman(Graph & Gbeta, Graph & Galpha, match_visitor & vis);
  ~isomorphism_ullman();

  int run(bool match_one=true);

  bool run_match_one();

  // Match one
  bool match(int * pn, size_t C1[], size_t C2[],
             ullman_state<Graph,match_visitor> & S);
  // Match all
  bool match(size_t C1[], size_t C2[], match_visitor vis,
             ullman_state<Graph,match_visitor> & S,
             int * pcount);

  int run_match_all();

private:

  Graph & G1;  // small graph (pattern)  (n1)
  Graph & G2;  // large graph (target )  (n2)

  match_visitor & visitor;

  size_t   num_verts_1;
  size_t   num_verts_2;
  size_t   core_alloc_size;
  size_t   null_node;
};


/* constructor */
template<typename Graph, typename match_visitor>
isomorphism_ullman<Graph,match_visitor>::
isomorphism_ullman(Graph & Gbeta, Graph & Galpha, match_visitor & vis)
  : G2(Gbeta), G1(Galpha), visitor(vis)
{
  core_alloc_size = 0;
  num_verts_1 = num_vertices(G1);
  num_verts_2 = num_vertices(G2);
  null_node   = (num_verts_1 > num_verts_2 ? num_verts_1 : num_verts_2) + 1;
}


/* destructionator */
template<typename Graph, typename match_visitor>
isomorphism_ullman<Graph,match_visitor>::
~isomorphism_ullman()
{
  core_alloc_size = 0;
}


/* has_edge */
template<typename Graph,typename match_visitor>
int
isomorphism_ullman<Graph,match_visitor>::
run(bool match_one)
{
#if DEBUG_Trace_ullman
  cout << "entr\trun(" << match_one << ")" << endl;
#endif

  size_t num_verts_1 = num_vertices(G1);
  size_t num_verts_2 = num_vertices(G2);

  core_alloc_size = num_verts_1 < num_verts_2 ? num_verts_1 : num_verts_2;

  if(match_one)
    return run_match_one();
  else
    return run_match_all();

  return 0;
}



/* run_match_one */
template<typename Graph,typename match_visitor>
bool
isomorphism_ullman<Graph,match_visitor>::
run_match_one()
{
#if DEBUG_Trace_ullman
  cout << "entr\trun_match_one()" << endl;
#endif

  ullman_state<Graph,match_visitor> state(G2, G1, visitor);

  size_t coreset_1[core_alloc_size];
  size_t coreset_2[core_alloc_size];

  bool status = false;
  int  result_size=0;
  status = match(&result_size, coreset_1, coreset_2, state);

  return status;
}



/* match one */
template<typename Graph, typename match_visitor>
bool
isomorphism_ullman<Graph,match_visitor>::
match(int * pn, size_t C1[], size_t C2[], ullman_state<Graph,match_visitor> & S)
{
#if DEBUG_Trace_ullman
  cout << "entr\tmatch(pn, C1, C2, S)" << endl;
  S.print_self(1);
#endif

  if(S.is_goal())
    {
    *pn = S.get_core_len();
    S.get_core_set(C1, C2);
    visitor(*pn, C1, C2);
    return true;
    }

  if(S.is_dead())
    {
    return false;
    }

  size_t N1 = null_node;
  size_t N2 = null_node;

  bool found = false;

  while( !found && S.next_pair(&N1, &N2, N1, N2) )
    {
    #if DEBUG_Trace_ullman
    cout << "\tpair: " << N1 << ", " << N2 << endl;
    #endif
    if( S.is_feasible_pair(N1,N2) )
      {
      ullman_state<Graph,match_visitor> * S1 = S.Clone();
      S1->add_pair(N1,N2);
      found = match(pn, C1, C2, *S1);
      // S1->back_track();
      delete S1;
      }
    }
  return found;
}



/* run_match_all */
template<typename Graph,typename match_visitor>
int
isomorphism_ullman<Graph,match_visitor>::
run_match_all()
{
#if DEBUG_Trace_ullman
  cout << "entr\trun_match_all()" << endl;
#endif

  ullman_state<Graph,match_visitor> state(G2, G1, visitor);

  size_t coreset_1[core_alloc_size];
  size_t coreset_2[core_alloc_size];

  bool status = false;
  int  pcount = 0;

  match(coreset_1, coreset_2, visitor, state, &pcount);

  cout << "pcount = " << pcount << endl;

  return 0;
}


/* match all */
template<typename Graph, typename match_visitor>
bool
isomorphism_ullman<Graph,match_visitor>::
match(size_t C1[], size_t C2[], match_visitor vis,
      ullman_state<Graph,match_visitor> & S,
      int * pcount)
{
  #if DEBUG_Trace_ullman
  cout << "entr\tmatch(C1,C2,vis,S);" << endl;
  #endif

  if(S.is_goal())
    {
    ++(*pcount);
    int t_corelen = S.get_core_len();
    S.get_core_set(C1, C2);
    #if DEBUG_Trace_ullman
    cout << "exit\tmatch(C1,C2,vis,S);  (matched!)" << endl;
    #endif
    return vis(core_alloc_size, C1, C2);
    }

  if(S.is_dead())
    {
    #if DEBUG_Trace_ullman
    cout << "exit\tmatch(C1,C2,vis,S);  (is dead)" << endl;
    #endif
    return false;
    }

  size_t N1 = null_node;
  size_t N2 = null_node;

  bool found = false;

  while(S.next_pair(&N1, &N2, N1, N2) )
    {
    if( S.is_feasible_pair(N1,N2) )
      {
      ullman_state<Graph,match_visitor> * S1 = S.Clone();
      S1->add_pair(N1,N2);

      if( match(C1,C2,vis,*S1,pcount) )
        {
        // S1->back_track();
        delete S1;
#if DEBUG_Trace_ullman
cout << "exit\tmatch(C1,C2,vis,S);  (matched()=true)" << endl;
#endif
        return true;
        }
      else
        {
        // S1->back_track();
        delete S1;
        }
      }
    }
#if DEBUG_Trace_ullman
cout << "exit\tmatch(C1,C2,vis,S);  (end of routine)" << endl;
#endif
  return false;
}

} // !NAMESPACE MTGL

#endif  // #ifdef MTGL_ISOMORPHISM_ULLMAN
