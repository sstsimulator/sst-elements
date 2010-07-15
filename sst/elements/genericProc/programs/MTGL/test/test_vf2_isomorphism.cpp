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
/*! \file test_vf2_isomorphism.cpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 8/25/2009
*/
/****************************************************************************/

#include <unistd.h>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <string>
#include <strstream>
#include <sstream>
#include <vector>

#include <mtgl/bfs.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/generate_subgraph.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/random_walk.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/subgraph_adapter.hpp>
#include <mtgl/util.hpp>
#include <mtgl/vf2_isomorphism.hpp>

#define PRINT_MATCHES     0
#define GENERATED_TESTS   0
#define GENERATE_AND_SAVE 0   // generate a random graph, subgraph, and save
#define LOAD_EDGE_INDUCED_GRAPH_FROM_FILE 1

// =============================================================================
using namespace std;
using namespace mtgl;

// =============================================================================
template<typename T>
void my_print_array(T * Array, int size, const char* TAG=NULL)
  {
  if(TAG)
    {
    cout << TAG;
    }
  cout << "[ ";
  for(int i=0; i<size; i++)
    {
    cout << setw(2) << Array[i] << " ";
    }
  cout << "]" << endl;
  }

// =============================================================================
template<typename T>
bool match_arrays(T* A1, T* A2, int size)
  {
  assert(A1);
  assert(A2);
  bool matched = true;
  #pragma mta assert parallel
  for(int i=0; i<size; i++)
    {
    if( A1[i] != A2[i] )
      matched = false;
    }
  return matched;
  }

// =============================================================================
string generate_string(bool include_pid=false, time_t* time_param=NULL)
  { 
  #if !defined(__MTA__) && !defined(__XMT__)
  srand((unsigned)time(time_param));
  #else
  // need something for XMT
  #endif

  stringstream ss;
  string buf;

  int lowest=97;
  int highest=122;
  int range=(highest-lowest)+1;
  int buflen, bufoff;
  char buffer[64];

  if(include_pid)
    {
    pid_t pid = getpid();
    sprintf(buffer, "%x", pid);
    buf += buffer;
    buflen = buf.size();
    bufoff = buflen>=3 ? 3 : buflen;
    }

  for(int index=0; index<3; index++)
    {
    char value = lowest+rand()%(range) ;
    ss << value;
    }

  if(include_pid)
    {
    ss << buf.substr(buflen-bufoff, bufoff);
    }

  return ss.str();
  }

// =============================================================================
class vf2_match_visitor_t : public default_vf2_visitor
  {
  public:

  vf2_match_visitor_t()
    {
    lock = 0;  
    results = NULL;
    this->reset();
    };

  ~vf2_match_visitor_t() { };

  void reset()
    {
    num_matches = 0;
    total_verts_matched = 0;
    this->remove_result_vector();
    }

  void set_result_vector( vector<size_t*> * result_vector )
    {
    this->results = result_vector;
    }

  void remove_result_vector()
    {
    if(results)
      {
      results->clear();
      results = NULL;
      }
    }
  
  bool operator()(int n, size_t ni1[], size_t ni2[])
    {
    mt_incr(num_matches,1);
    mt_incr(total_verts_matched, n);

    #if PRINT_MATCHES
    cout << endl << "<--- MATCHED --->" << endl;
    for(int i=0; i<n; i++)
      {
      cout << ni1[i] << " == " << ni2[i] << endl;
      }
    #endif

    if(results)
      {
      size_t * result = (size_t*)malloc(sizeof(size_t)*n);
      for(int i=0; i<n; i++)
        {
        result[i] = ni2[i];
        }
      mt_readfe(lock);  
      results->push_back( result );
      mt_write(lock,1);
      }

    return false;
    }

  vector<size_t*> * results;
  int num_matches;
  int total_verts_matched;
  int lock;
  };

// =============================================================================
bool xmt_load_textfile(char* filename, char ** buffer)
  {
  #ifdef __MTA__
  assert(filename);

  char    * _buffer   = NULL;
  int       size     = 0;
  int64_t   stat_err = 0;

  luc_error_t       err;
  luc_endpoint_id_t swEP = 0;
  snap_stat_buf     statBuf;

  err = snap_init();
  if(SNAP_ERR_OK != err)
    {
    fprintf(stderr, "Error: snap_init() failed.\n");
    return false;
    }

  char* swEP_string = getenv("SWORKER_EP");
  
  if(swEP_string != NULL)
    {
    swEP = strtoul(swEP_string, NULL, 16);
    }

  if( !filename || snap_stat(filename, swEP, &statBuf, &stat_err) != LUC_ERR_OK)
    {
    fprintf(stderr, "Error: snap_stat() failed.\n");  
    return false;
    }

  size = statBuf.st_size / sizeof(char);

  _buffer = (char*)malloc(sizeof(char)*size);
  if(!_buffer)
    {
    fprintf(stderr, "Error: failed to allocate temp buffer\n");  
    return false;  
    }
  
  if(snap_restore(filename, _buffer, statBuf.st_size, NULL) != SNAP_ERR_OK)
    {
    fprintf(stderr, "Error: snap_restore() failed.\n");
    return false;
    }

  *buffer = _buffer;
  return true;
  #else
  fprintf(stderr, "Error: MTA/XMT required.\n");
  return false;
  #endif
  }

// =============================================================================
/*  Load a text file that contains 1 or 2 columns of integers that are
 *  delimited by a space.  The number of rows is returned as well as the
 * max value found.
 */
template<typename T, typename U>
bool xmt_load_columns_from_textfile(char* filename, int num_cols,
                                    T & max_value,  T & num_rows,
                                    U** column1,    U** column2)
  {
  #ifdef __MTA__
  if(num_cols < 1 || num_cols > 2)
    {
    fprintf(stderr, "num_cols not in range (1,2)\n");
    return false;
    }
  
  if(*column1 != NULL) 
    {
    fprintf(stderr,"column1 pointer is not empty!\n"); 
    return false; 
    }
  if(num_cols == 2 && *column2 != NULL)
    {
    fprintf(stderr,"column2 pointer is not empty!\n");
    return false;
    }

  char * buffer   = NULL;
  if( !xmt_load_textfile(filename, &buffer) )
    {
    return false;  
    }
  assert(buffer);

  max_value = 0;
  num_rows  = 0;

  {
    // PASS 1: Determine how many edges we have and max VID.  
    string line;
    istringstream iss;
    iss.str( buffer );
    while( !std::getline( iss, line ).eof() )
      {
      int u,v;
      istringstream lineStream(line);
      if(num_cols==1)  
        {
        lineStream >> u;
        max_value = u > max_value ? u : max_value;
        }
      else if(num_cols==2)
        {
        lineStream >> u >> v;
        max_value = u > max_value ? u : max_value;
        max_value = v > max_value ? v : max_value;
        }
      num_rows++;
      }
  }
  
  if(max_value==0 || num_rows==0)
    {
    return false;
    }

  // Allocate Storage
  T irow=0;
  U* _column1 = NULL;
  U* _column2 = NULL;
  _column1 = (U*)malloc(sizeof(U)*num_rows);
  assert(_column1);

  if(num_cols==2)
    {
    _column2 = (U*)malloc(sizeof(U)*num_rows);
    assert(_column2);
    }

    {
    // PASS 2: Fill edge arrays.
    string line;
    istringstream iss;
    iss.str( buffer );
    while( !std::getline( iss, line ).eof() )
      {
      istringstream lineStream(line);
      if(num_cols==1)
        {
        lineStream >> _column1[irow];
        }
      else if(num_cols==2)
        {
        lineStream >> _column1[irow] >> _column2[irow];
        }
      irow++;
      }
    if(num_cols == 1)
      {
      *column1 = _column1;
      }
    if(num_cols == 2)
      {
      *column1 = _column1;
      *column2 = _column2;
      }
    }

  // Cleanup
  if(buffer) 
    { 
    free(buffer); 
    buffer=NULL; 
    }
  
  return true;
  
  #else
  
  fprintf(stderr, "Error: XMT/MTA required.\n");
  return false;
  
  #endif
  }

// =============================================================================
template<typename T, typename U>
bool load_edges_from_file(const char* filename, T& num_verts, T& num_edges, U** sources, U** targets)
  {
  if(*sources != NULL) { cerr << "sources pointer is not empty!" << endl; return false; }
  if(*targets != NULL) { cerr << "targets pointer is not empty!" << endl; return false; }
  
  {
  ifstream IFP( filename );
  if(IFP.is_open() )
    {
    string line;
    cout << "\t|";
    while( getline(IFP, line) )
      {
      cout << "-";
      int u, v;
      istringstream iss(line);
      iss >> u >> v;
      num_edges++;
      num_verts = u > num_verts ? u : num_verts;
      num_verts = v > num_verts ? v : num_verts;
      }
    //num_verts++;
    cout << "|" << endl;
    }
  IFP.close();
  }

  if(num_verts==0 || num_edges==0)
    return false;

  // allocate storage
  T iedge=0;
  U* _sources = (U*)malloc(sizeof(U)*num_edges);
  U* _targets = (U*)malloc(sizeof(U)*num_edges);
  assert(_sources);
  assert(_targets);

  {
  // 2nd pass
  ifstream IFP( filename );
  if(IFP.is_open() )
    {
    string line;
    cout << "\t|";
    while( getline(IFP, line) )
      {
      cout << "-";
      istringstream iss(line);
      iss >> _sources[iedge] >> _targets[iedge];
      iedge++;
      }
    cout << "|" << endl;
    }
  IFP.close();
  }

  *sources = _sources;
  *targets = _targets;

  return true;
  }

// =============================================================================
template<typename Graph>
bool load_graph_from_file(char* filename, Graph &ga)
  {
  assert(filename);
  cout << "Load graph from '" << filename << "'" << endl; fflush(stdout);
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  size_t num_verts = 0;
  size_t num_edges = 0;
  vertex_t * srcs = NULL;
  vertex_t * dsts = NULL;
  
  #ifdef __MTA__
  if( !xmt_load_columns_from_textfile(filename, 2, num_verts, num_edges, &srcs, &dsts) )
    {
    cerr << "Problem encountered loading file " << filename << "." << endl;
    return false;
    }
  
  #else
   
  if( !load_edges_from_file(filename, num_verts, num_edges, &srcs, &dsts) )
    {
    cerr << "Problem encountered loading file " << filename << "." << endl;
    return false;
    }
  
  #endif

  num_verts++;  // (num_verts is really returned as max_value, but arrays are 0-indexed)
  
  init(num_verts,num_edges,srcs,dsts,ga);
  cout << "\tNumber of vertices = " << num_verts << endl;
  cout << "\tNumber of edges    = " << num_edges << endl;
  return true;
  }

// =============================================================================
template<typename T>
bool load_map_from_file(char* filename, T** map, T* size)
  {
  if(!filename)
    {
    cerr << "ERROR: No filename provided to load_map_from_file()." << endl;
    return false;
    }

  cout << "Load map from '" << filename << "'" << endl; fflush(stdout);

  #ifdef __MTA__

  T   max   = 0;
  T * dummy = NULL;
  T * _map  = NULL;
  
  if( !xmt_load_columns_from_textfile(filename, 1, max, *size, &_map, &dummy) )
    {
    cerr << "ERROR: Map file failed to load." << endl;
    return false;
    }
  *map = _map;
  
  #else
  
  vector<T> tmp(0);

  ifstream IFP( filename );
  if(IFP.is_open() )
    {
    string line;
    cout << "\t|";
    while( getline(IFP, line) )
      {
      cout << "-";
      int v;
      istringstream iss(line);
      iss >> v;
      tmp.push_back(v);
      }
    cout << "|" << endl;
    }
  else
    {
    cerr << "ERROR: Could not open file '" << filename << "'" << endl;
    return false;
    }
  IFP.close();

  *map = (T*)malloc(sizeof(T)*tmp.size());
  for(int i=0; i<tmp.size(); i++)
    {
    (*map)[i] = tmp[i];
    }

  *size = tmp.size();
  
  #endif

  return true;
  }

// =============================================================================
template<typename Graph>
void output_graph_edgelist(Graph& g, ostream& out)
  {
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  size_t num_verts = num_vertices(g);
  size_t num_edges = num_edges(g);
  vertex_iterator_t verts = vertices(g);

  for(size_t vidx=0; vidx<num_verts; vidx++)
    {
    vertex_t vi = verts[vidx];
    int deg = out_degree(vi, g);
    eiter_t edgs = out_edges(vi, g);
    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = edgs[adji];
      size_t other = get(vid_map, target(e, g));
      out << vidx << " " << other << endl;
      }
    }
  }

// =============================================================================
template<typename Graph>
void my_print_graph(Graph& g)
  {
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;
  typedef typename graph_traits<Graph>::incident_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  size_t num_verts = num_vertices(g);
  size_t num_edges = num_edges(g);
  vertex_iterator_t verts = vertices(g);

  for (size_t vidx = 0; vidx < num_verts; vidx++)
    {
    vertex_t vi = verts[vidx];
    int deg = out_degree(vi, g);
    cout << vidx << "\tout [" << setw(2) << deg << "] { ";
    eiter_t edgs = out_edges(vi, g);
    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = edgs[adji];
      size_t other = get(vid_map, target(e, g));
      cout << other << " ";
      }
    cout << "}" << endl;

    deg = in_degree(vi, g);
    cout << "\tin  [" << setw(2) << deg << "] { ";

    in_eiter_t in_edgs = in_edges(vi, g);
    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = in_edgs[adji];
      size_t other = get(vid_map, source(e, g));
      cout << other << " ";
      }
    cout << "}" << endl;
    }
  }

// =============================================================================
template<typename Graph>
void output_vflib_graph(Graph& g, ostream& out)
  {
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;
  typedef typename graph_traits<Graph>::incident_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  size_t num_verts = num_vertices(g);
  size_t num_edges = num_edges(g);

  out << "# Number of nodes" << endl
       << num_verts << endl;
  for(size_t vidx=0; vidx<num_verts; vidx++)
    {
    out << vidx << endl;
    }
  out << endl;

  vertex_iterator_t verts = vertices(g);
  for(size_t vidx=0; vidx<num_verts; vidx++)
    {
    out << "# Edges coming out of node " << vidx << endl;
    vertex_t vi = verts[vidx];
    int deg = out_degree(vi, g);
    out << deg << endl;
    eiter_t edgs = out_edges(vi, g);
    for (size_t adji = 0; adji < deg; adji++)
      {
      edge_t e = edgs[adji];
      size_t other = get(vid_map, target(e, g));
      out << vidx << " " << other << endl;
      }
    out << endl;
    }
  }

// =============================================================================
template<typename T>
void generate_and_save_graph(T rmat_scale,     T rmat_edgefactor,
                             T num_subg_verts, T num_subg_edges,
                             char* filename)
  {
  typedef static_graph_adapter<bidirectionalS> Graph;
  assert(filename);
  ofstream ofs;

  string graph_id = generate_string();

  stringstream rmat_filename;
  stringstream rmat_subg_filename;
  stringstream rmat_subg_map_filename;

  rmat_filename << "rmat." << rmat_scale << "_" << rmat_edgefactor
                << "." << graph_id << ".gra";
  cout << rmat_filename.str() << endl;

  rmat_subg_filename << "rmat." << rmat_scale << "_" << rmat_edgefactor
                     << ".subg." << num_subg_verts << "_" << num_subg_edges
                     << "." << graph_id << ".gra";
  cout << rmat_subg_filename.str() << endl;

  rmat_subg_map_filename << "rmat." << rmat_scale << "_" << rmat_edgefactor
                     << ".subg." << num_subg_verts << "_" << num_subg_edges
                     << "." << graph_id << ".map";
  cout << rmat_subg_map_filename.str() << endl;

  Graph rmat;
  generate_rmat_graph(rmat, rmat_scale, rmat_edgefactor);

  // Create a subgraph from the R-MAT graph.
  subgraph_adapter<Graph> subg(rmat);
  generate_subgraph(num_subg_verts, num_subg_edges, rmat, subg);

  ofs.open(rmat_subg_map_filename.str().c_str(), ios_base::out);
  for(int i=0; i<num_vertices(subg); i++)
    {
    ofs << subg.local_to_global(i) << endl;
    }
  ofs.close();

  ofs.open(rmat_subg_filename.str().c_str(), ios_base::out);
  output_graph_edgelist(subg, ofs);
  ofs.close();

  ofs.open(rmat_filename.str().c_str(), ios_base::out);
  output_graph_edgelist(rmat, ofs);
  ofs.close();
  }

// =============================================================================
/* returns false if a match wasn't found
 * returns true if a match -was- found.
 */
bool test_rmat_graph(int graph_scale,      int edges_per_vert,
                     int subg_num_verts,   int subg_num_edges,
                     int vf2_search_type=VF2_SUBG_MONOMORPHISM,
                     int vf2_search_termination=VF2_FIND_ALL,
                     const char* tag=NULL,
                     int num_expected_matches=-1)
  {
  cout << "Executing VF2 Algorithm Test";
  if(tag) cout << " [" << tag << "]";
  cout << endl;
  string str_search_type, str_search_termination;

  if(vf2_search_termination==VF2_FIND_ALL)
    str_search_termination="FIND ALL";
  else if(vf2_search_termination==VF2_FIND_ONE)
    str_search_termination="FIND ONE";
  else
    {
    cerr << "***\tUnknown vf2 termination type provided." << endl;
    return false;
    }

  if(vf2_search_type==VF2_SUBG_ISOMORPHISM)
    str_search_type = "SUBGRAPH ISOMORPHISM";
  else if(vf2_search_type==VF2_FULL_ISOMORPHISM)
    str_search_type = "GRAPH ISOMORPHISM";
  else if(vf2_search_type==VF2_SUBG_MONOMORPHISM)
      str_search_type = "SUBGRAPH MONOMORPHISM";
  else
    {
    cerr << "***\tUnknown vf2 search type provided." << endl;
    return false;
    }

  cout << "\tsearch type : " << str_search_type << endl;
  cout << "\tsearch until: " << str_search_termination << endl;


  int  num_matches = 0;
  bool rval = true;

  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::size_type size_t;

  mt_timer timer;
  vf2_match_visitor_t vis;

  vector<size_t*> results;
  vis.set_result_vector( &results );

  // Generate a R-MAT graph
  Graph rmat;
  generate_rmat_graph(rmat, rmat_scale, rmat_edgefactor);

  cout << "generated graph" << endl;
  rmat.print();  fflush(stdout);

  // Create a subgraph from the R-MAT graph.
  subgraph_adapter<Graph> subg(rmat);
  generate_subgraph(subg_num_verts, subg_num_edges, rmat, subg);

  cout << "generated subgraph" << endl;
  subg.print();  fflush(stdout);

  cout << "\tGraph order : " << setw(5) << num_vertices(rmat) << endl;
  cout << "\tGraph size  : " << setw(5) << num_edges(rmat) << endl;

  cout << "\tSub-G order : " << setw(5) << num_vertices(subg)
       << "\t(" << subg_num_verts << ")" << endl;
  cout << "\tSub-G size  : " << setw(5) << num_edges(subg)
       << "\t(" << subg_num_edges << ")" << endl;

  size_t subg_size = num_vertices(subg);
  size_t * subg_mapping = (size_t*)malloc( sizeof(size_t)*subg_size );
  size_t * subg_flags   = (size_t*)malloc( sizeof(size_t)*subg_size );

  for(int i=0; i<subg_size; i++)
    {
    subg_mapping[i] = subg.local_to_global(i);
    cout << i << " : " << subg.local_to_global(i) << endl;  // mapping
    }

  sort(subg_mapping, subg_mapping+subg_size);

  vf2_isomorphism<Graph> vf2(rmat, subg, vis);

  timer.start();
  num_matches = vf2.run(vf2_search_type, vf2_search_termination);
  timer.stop();

  bool subgraph_found = false;

  // Sort results (and look for exact match against original subgraph)
  for(int i=0; i<results.size(); i++)
    {
    sort( results[i], results[i]+subg_size );

    //my_print_array( results[i], subg_size, "-------" );  // print a result

    bool result_matched = true;

    for(size_t j=0; j<subg_size; j++)
      {
      if(results[i][j] != subg_mapping[j])
        {
        result_matched = false;
        break;
        }
      }

    if(result_matched)
      {
      // cout << "\t|----- FOUND ORIGINAL SUBGRAPH ----->" << endl;
      subgraph_found = true;
      }
    }

  cout << "\tnumber of matches: " << num_matches;
  // If an expected match count is provided, check and flag error if mismatched.
  if(num_expected_matches != -1 && num_expected_matches != num_matches)
    {
    cout << " != {" << num_expected_matches << "}   (FAILURE FLAGGED!)";
    rval = false;
    }
  cout << endl;

  my_print_array( subg_mapping, subg_size, "\tsubg vertex map  : ");

  // Flag an error if we don't find the induced subgraph from the big graph.
  cout << "\tfound subgraph?  : ";
  if(subgraph_found) cout << "YES" << endl;
  else
    {
    cout << "NO  (FAILURE FLAGGED!)" << endl;
    rval = false;
    }

  cout << "\tsearch time (sec): "
       << fixed << setprecision(6) << timer.getElapsedSeconds() << endl;

  cout << "\ttest exit status : ";
  if(rval) cout << "PASS";
  else     cout << "FAIL";
  cout << endl;


  // some cleanup...
  vis.remove_result_vector();

  cout << endl;
  return rval;
  }

// =============================================================================
// =============================================================================
int main(int argc, char *argv[]) 
  {
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::size_type size_t;

  int num_failed = 0;

  #ifndef __MTA__
  srand48(0);
  #endif

  cout << "Testing VF2 (sub)graph isomorphism" << endl << endl;

  #if GENERATED_TESTS
  num_failed += false == test_rmat_graph(3,4, 4,9, VF2_SUBG_MONOMORPHISM, VF2_FIND_ALL, "rmat(3,4)");
  //num_failed += false == test_rmat_graph(4,4, 10,14, VF2_SUBG_MONOMORPHISM, VF2_FIND_ALL, "rmat(4,4)");
  //num_failed += false == test_rmat_graph(5,4, 10,14, VF2_SUBG_MONOMORPHISM, VF2_FIND_ALL, "rmat(5,4)");
  //num_failed += false == test_rmat_graph(5,8, 20,94, VF2_SUBG_MONOMORPHISM, VF2_FIND_ALL, "rmat(5,8)");
  cout << "Number of tests failed = " << num_failed << endl;
  #endif

  #if GENERATE_AND_SAVE
  generate_and_save_graph(6,8, 20,80, "RMAT");
  return 0;
  #endif

  #if LOAD_EDGE_INDUCED_GRAPH_FROM_FILE
  {
  // Create a timer & start it
  mt_timer timer;

  vector<size_t*> results;

  vf2_match_visitor_t vis;
  vis.set_result_vector(&results);

  Graph graph;
  Graph subg;

  bool ok = true;
  size_t * map = NULL;
  size_t   map_size = 0;

  #ifdef __MTA__

  ok &= load_graph_from_file("/scratch1/wcmclen/rmat.4_4.spj.gra", graph);
  ok &= load_graph_from_file("/scratch1/wcmclen/rmat.4_4.subg.8_22.spj.gra", subg);
  ok &= load_map_from_file("/scratch1/wcmclen/rmat.4_4.subg.8_22.spj.map", &map, &map_size);

  #else

  ok &= load_graph_from_file("rmat.4_4.spj.gra", graph);
  ok &= load_graph_from_file("rmat.4_4.subg.8_22.spj.gra", subg);
  ok &= load_map_from_file("rmat.4_4.subg.8_22.spj.map", &map, &map_size);

  //ok &= load_graph_from_file("rmat.4_8.twt.gra", graph);
  //ok &= load_graph_from_file("rmat.4_8.subg.8_22.twt.gra", subg);
  //ok &= load_map_from_file("rmat.4_8.subg.8_22.twt.map", &map, &map_size);

  //ok &= load_graph_from_file("rmat.5_8.swk.gra", graph);
  //ok &= load_graph_from_file("rmat.5_8.subg.8_22.swk.gra", subg);
  //ok &= load_map_from_file("rmat.5_8.subg.8_22.swk.map", &map, &map_size);

  //ok &= load_graph_from_file("rmat.5_8.evs.gra", graph);
  //ok &= load_graph_from_file("rmat.5_8.subg.10_40.evs.gra", subg);
  //ok &= load_map_from_file("rmat.5_8.subg.10_40.evs.map", &map, &map_size);

  //ok &= load_graph_from_file("rmat.6_8.kak.gra", graph);
  //ok &= load_graph_from_file("rmat.6_8.subg.10_40.kak.gra", subg);
  //ok &= load_map_from_file("rmat.6_8.subg.10_40.kak.map", &map, &map_size);

  //ok &= load_graph_from_file("rmat.6_8.xdd.gra", graph);
  //ok &= load_graph_from_file("rmat.6_8.subg.20_80.xdd.gra", subg);
  //ok &= load_map_from_file("rmat.6_8.subg.20_80.xdd.map", &map, &map_size);

  #endif
  
  if(!ok) { cerr << "Failed to load graph(s)." << endl; return 1; }

  cout << endl << "Graph" << endl << "-----" << endl;
  graph.print();
  cout << endl << "Subgraph" << endl << "--------" << endl;
  subg.print();
  cout << endl;
  my_print_array(map, map_size, "subg map");
  cout << endl;
  fflush(stdout);

  vf2_isomorphism<Graph> vf2(graph, subg, vis);

  timer.start();
  int status = vf2.run(VF2_SUBG_MONOMORPHISM, VF2_FIND_ALL);
  timer.stop();

  cout << "Time   = "
       << fixed << setprecision(4) << timer.getElapsedSeconds() << endl;
  cout << "Status = " << status << endl << endl;
  cout << "There are " << results.size() << " results." << endl;
  
  cout << "Validate results:" << endl;

  sort(map, map+map_size);

  bool match_found = false;

  for(int i=0; i<results.size(); i++)
    {
    sort(results[i], results[i]+num_vertices(subg));

    bool matched = match_arrays(map, results[i], num_vertices(subg));
    if( matched )
      {
      cout << "    |------ MATCHED ---------->" << endl;
      match_found = true;
      }
    }

  cout << endl;
  if(match_found)  cout << "Match found!" << endl;
  else             cout << "Match not found!" << endl;

  // do a bit of cleanup...
  vis.remove_result_vector();
  }
  
  #endif

  cout << "Done." << endl;
  return (num_failed!=0);
  }
