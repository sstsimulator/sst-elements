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

// This code is licensed under the Common Public License
// It originated from COIN-OR

// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef MTGL_UFL_H
#define MTGL_UFL_H

#define PARCUTOFF 5000

#include <vector>	// including sys/times.h before this led to compiler
                 	// errors on Linux
#include <cstdio>
#include <cfloat>
#include <string>
#include <map>
#include <cstring>

#include <mtgl/util.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/VolVolume.h>

#ifndef WIN32
#include <sys/times.h>
#endif

#define ALWAYS_SERVE 1
#define DONT_ALWAYS_SERVE 0

using namespace mtgl;

// parameters controlled by the user
class UFL_parms {
public:
  std::string fdata; // file with the data
  std::string dualfile; // file with an initial dual solution
  std::string dual_savefile; // file to save final dual solution
  std::string int_savefile; // file to save primal integer solution
  int h_iter; // number of times that the primal heuristic will be run
  // after termination of the volume algorithm
   
  UFL_parms(): fdata(""), dualfile(""), dual_savefile(""), int_savefile("sensors.txt"), h_iter(100){}

  UFL_parms(const char* filename);
  ~UFL_parms() {}
};



template <typename vtype, typename adjlist_t>
class UFL : public VOL_user_hooks {
public:
  typedef typename adjlist_t::column_iterator column_iterator;
  typedef typename adjlist_t::value_iterator value_iterator;
  // for all hooks: return value of -1 means that volume should quit
  // compute reduced costs
   int compute_rc(const VOL_dvector& u, VOL_dvector& rc);
  // solve relaxed problem
   int solve_subproblem(const VOL_dvector& u, const VOL_dvector& rc,
			double& lcost, VOL_dvector& x, VOL_dvector&v,
			double& pcost);
   //DEPRECATED
/*    int solve_subproblem_with_mask(const VOL_dvector& u, const VOL_dvector& rc, */
/* 			double& lcost, VOL_dvector& x, VOL_dvector&v, */
/* 				  double& pcost, int **load_balance_masks = 0, */
/* 				  int *load_balance_totals = 0, */
/* 				  int **load_balance_parallelize_flags = 0,     */
/* 				  int num_load_balance_masks = 0); */

   void process_opening_serving_of_nodes(int mask_iteration, int mask_vertex_index, 
					 int &num_open, double &sum_fcosts, double &value, 
					 VOL_ivector &sol, const double *rdist,
					 int **load_balance_masks, int *load_balance_totals, 
					 int **load_balance_parallelize_flags, int num_load_balance_masks);

  // primal heuristic
  // return DBL_MAX in heur_val if feas sol wasn't/was found 
   int heuristics(const VOL_problem& p, 
		  const VOL_dvector& x, double& heur_val, double lb);

   void init_u(VOL_dvector& u);

   double open_more_servers(VOL_ivector& nsol,
			    int *unserved, int num_unserved);
   double assign_service(VOL_ivector& nsol, const VOL_dvector& x);
   int * find_unserved(VOL_ivector& nsol, int num_servers, int& num_unserved);
   void separate_by_degree();
   void separate_by_degree(int *to_sep, int num_to_sep,
					 int *&hd, int& num_hd,
					 int *&ld, int& num_ld);

  // original data for uncapacitated facility location
public: 
  VOL_dvector fcost; // cost for opening facilities
  VOL_dvector dist; // cost for connecting a customer to a facility
  adjlist_t& _dist;
  std::vector<adjlist_t > _sidecon; // Additional side constr.
  std::vector<vtype> _ub; // Upper bounds for side constraints
  VOL_ivector fix; // vector saying if some variables should be fixed
  // if fix=-1 nothing is fixed
  int ncust, nloc; // number of customers, number of locations
  int nnz;      // number of nonzero (customer, facility) pairs
  int _p;          // _p>0 for p-median problems 
  VOL_ivector ix;   // best integer feasible solution so far
  double      icost;  // value of best integer feasible solution 
  double      gap;    // percentage (icost-lb)/lb
  double     *init_lagrange;  // user may want to initialize multipliers
  //int        *high_degree;    // indices of high degree locations
  //int        *low_degree;     // indices of low degree locations
  //int        n_hd;	      // number of high degree locations
  //int        n_ld;	      // number of low degree locations
  
  int        twenty_fifth;   //should take this out once we decide where it should go

public:
  UFL(adjlist_t& d, double *init_l=0) : _dist(d), icost(DBL_MAX),
					init_lagrange(init_l)//,
					//n_hd(0),n_ld(0),
                                        //high_degree(0), low_degree(0), 
                                        //twenty_fifth(0)
  {
  }
  virtual ~UFL() 
  {
	//delete[] high_degree;
	//delete[] low_degree;
  }  

  //int get_n_hd() const { return n_hd; }
  //int get_n_ld() const { return n_ld; }
  //int* get_high_degree() const { return high_degree; }
  //int* get_low_degree()  const { return low_degree;  }
};

//#############################################################################
//########  Member functions          #########################################
//#############################################################################


extern int newDummyID;
extern map<int, int> newID;
extern map<int, int>::iterator newIDiter;
extern vector<int> origID;
extern double gap;	// e.g. 0.1 --> stop when within 10% of optimal

char *UFL_preprocess_data(const char* fname,  int num_facilities);

template <typename vtype, typename adjlist>
int ufl_module(int nloc, int ncust, int nnz,
	   int& num_facilities_to_open, double *opening_costs, 
	   adjlist& service_costs, double gap,
	   bool* open_facilities, int *server,
	   double *frac_solution=0,
           double *init_lagrange=0,
	   int **load_balance_masks = 0,
	   int *load_balance_totals = 0,
           int **load_balance_parallelize_flags = 0,    
	   int num_load_balance_masks = 0
)
{
   typedef typename adjlist::column_iterator column_iterator;
   typedef typename adjlist::value_iterator  value_iterator;


   printf("sizeof(volp): %lu\n", (long unsigned)sizeof(VOL_problem));
   //UFL_parms ufl_par("ufl.par"); // for debugging
   UFL_parms ufl_par;   // SPOT - omit parameter file
   UFL<vtype, adjlist>  ufl_data(service_costs, init_lagrange);
   int num_sidecon = 0; // Default is no side constraints

   newDummyID = nloc;   // dummy concept irrelevant in this context
   // read data 
   printf("ufl_module: nloc: %d\n", nloc);
   ufl_data.nloc = nloc;
   ufl_data.ncust = ncust;
   ufl_data.nnz = nnz;
   ufl_data.gap = gap;
   VOL_dvector& fcost = ufl_data.fcost;
   fcost.allocate(nloc);

   if (opening_costs) {
	#pragma mta assert nodep
	for (int i=0; i<nloc; i++) {
		fcost[i] = opening_costs[i];
	}
   	ufl_data._p = 0;
   } else {
   	ufl_data._p = num_facilities_to_open;
	#pragma mta assert nodep
	for (int i=0; i<nloc; i++) {
		fcost[i] = 0.0;
	}
   }
   //ufl_data.separate_by_degree();

   // create the VOL_problem from the parameter file
   //VOL_problem volp("ufl.par"); // for debugging
   VOL_problem volp;    // SPOT - omit parameter file
   // volp.psize = ufl_data.nloc + ufl_data.nloc*ufl_data.ncust;
   volp.psize = ufl_data.nloc + ufl_data.nnz;
   volp.dsize = ufl_data.ncust + num_sidecon;
   bool ifdual = false;
   int state = 12345;
   if (ufl_par.dualfile.length() > 0) {
     // read dual solution
      ifdual = true;
      VOL_dvector& dinit = volp.dsol;
      dinit.allocate(volp.dsize);

      FILE * file = fopen(ufl_par.dualfile.c_str(), "r");
      if (!file) {
	 printf("Failure to open file: %s\n ", ufl_par.dualfile.c_str());
	 abort();
      }
      const int dsize = volp.dsize;
      int idummy;
      for (int i = 0; i < dsize; ++i) {
	 int rc = fscanf(file, "%i%lf", &idummy, &dinit[i]);
         if (rc != 2) {
           printf("ERROR: ufl_module: failed to scan all fields\n");
         }
      }
      fclose(file);
   } else {
	// initialize lagrange multipliers in a slightly random way to 
	// avoid the cycling problem associated with self-loops
   	VOL_dvector& dinit = volp.dsol;  
      	dinit.allocate(volp.dsize);
   	int state = 12345;
	int vdsize = volp.dsize;
	double r;

  random_value* rand =
    (random_value*) malloc(volp.dsize * sizeof(random_value));
	rand_fill::generate(volp.dsize, rand);

	#pragma mta assert nodep
   	for (int i = 0; i < vdsize; ++i) {
     		r= rand[i] / (double) INT_MAX;
		dinit[i] = r;
	}
   }

   // We set dual bounds for side constraints. Assume primal <= constraints.

   // This would be the right place to set bounds on the dual variables
   // For UFL all the relaxed constraints are equalities, so the bounds are 
   // -/+inf, which happens to be the Volume default, so we don't have to do 
   // anything.
   // Otherwise the code to change the bounds would look something like this:

   // first the lower bounds to -inf, upper bounds to inf
   volp.dual_lb.allocate(volp.dsize);
   volp.dual_lb = -1e31;
   volp.dual_ub.allocate(volp.dsize);
   volp.dual_ub = 1e31;
   // now go through the relaxed constraints and change the lb of the ax >= b 
   // constrains to 0, and change the ub of the ax <= b constrains to 0.
/*
   for (int i = ufl_data.ncust; i < volp.dsize; ++i) {
     volp.dual_ub[i] = 0;  // Assume all (primal) <= ineq.
   }
*/

#ifndef WIN32
   // start time measurement
   double t0;
   struct tms timearr; clock_t tres;
   tres = times(&timearr); 
   t0 = timearr.tms_utime; 
#endif

   // invoke volume algorithm
   int vs;
   //DEPRECATED
   //   if (num_load_balance_masks == 0) {
     printf("solving without masks\n");
     bool initu = (init_lagrange != 0);
     vs = volp.solve(ufl_data, initu);   // JWB ifdual);
   /* } else { */
/*      printf("solving with masks\n"); */
/*      vs = volp.solve_with_mask(ufl_data,   */
/* 			       load_balance_masks, */
/* 			       load_balance_totals, */
/* 			       load_balance_parallelize_flags,     */
/* 			       num_load_balance_masks, */
/* 			       ifdual); */
/*    } */

   if (vs == 0) {
      printf("no integer solution during vol run...\n");
   } else if (vs == -1) {	// done
      printf("solve succeeded within gap...\n");
   } else {
      // recompute the violation
      const int n = ufl_data.nloc;
      const int m = ufl_data.ncust;

      VOL_dvector v(volp.dsize);
      const VOL_dvector& psol = volp.psol;
      v = 1;

/***  JWB
      int i,j,k=n;
      for (j = 0; j < n; ++j){
	for (i = 0; i < m; ++i) {
	  v[i] -= psol[k];
	  ++k;
	}
      }
***/

   int k=0;
   adjlist& _dist = ufl_data._dist;
   //int n_hd = ufl_data.get_n_hd();
   //int n_ld = ufl_data.get_n_ld();
   //int *high_degree = ufl_data.get_high_degree();
   //int *low_degree  = ufl_data.get_low_degree();
   int rows = _dist.MatrixRows();
   int total = 0;		// debug

   #pragma mta assert parallel
   for (int i=0; i < nloc; i++) {
     int i_index = _dist.col_index(i);
     int next_index = _dist.col_index(i+1);
     column_iterator begin_cols = _dist.col_indices_begin(i);
     column_iterator end_cols = _dist.col_indices_end(i);
     column_iterator ptr2 = begin_cols;
     double pcost_incr = 0.0;

	int offs=0;
     	for (int k=i_index; k<next_index; k++, offs++) {
     		column_iterator my_ptr2 = ptr2;
		my_ptr2.set_position(offs);
		int j = *my_ptr2;
		if (ufl_data.ix[n+k] == 1) {
			server[j] = i;
			//printf("ufl finishing: server[%d]: %d (index %d)\n", j, i,n+k);
		}
		v[j] -= psol[n+k];
     	}
   }


      double vc = 0.0;
      for (int i = 0; i < m; ++i) {
         double b = ((v[i] < 0) * -v[i]) + ((v[i] >= 0) * v[i]); // MTA
         if (b > 0.0)
                vc += b;
      }
      vc /= m;
      printf(" Average violation of final solution: %f\n", vc);

      if (ufl_par.dual_savefile.length() > 0) {
	// save dual solution
	 FILE* file = fopen(ufl_par.dual_savefile.c_str(), "w");
	 const VOL_dvector& u = volp.dsol;
	 int n = u.size();
	 int i;
	 fclose(file);
      }

      // run a couple more heuristics
/*
      double heur_val;
      for (int i = 0; i < ufl_par.h_iter; ++i) {
	 heur_val = DBL_MAX;
	 ufl_data.heuristics(volp, psol, heur_val,0.0);
      }
*/
      // save integer solution
      if (ufl_par.int_savefile.length() > 0) {
	 FILE* file = fopen(ufl_par.int_savefile.c_str(), "w");
	 const VOL_ivector& x = ufl_data.ix;
	 const int n = ufl_data.nloc;
	 const int m = ufl_data.ncust;
	 int i,j,k=n;
         //   SPOT: Need to check both of these, I just guessed:
         //         omit the dummy location in this list
         //         we need to print out input IDs, not our IDs
         fprintf(file, "Best integer solution value: %f\n", 
				ufl_data.icost);
         fprintf(file, "Lower bound: %f\n", volp.value);
	 fprintf(file, "Sensor locations\n");

	 if (origID.size() > 0) {
	 	for (i = 0,j=0; i < n; ++i) {
	   		if ( x[i]==1 && (i != newDummyID)){
 	     			fprintf(file, "%6d ", origID[i]);
             			if (j && (j%10 == 0)) fprintf(file, "\n");
             			j++;
           		} 
	 	}
	} else if (open_facilities) {
		#pragma mta assert nodep
	 	for (i = 0; i < n; ++i) {
	   		if ( x[i]==1 && (i != newDummyID)){
	     			open_facilities[i] = true;
           		} else {
	     			open_facilities[i] = false;
			}
	 	}
	}
	 fclose(file);
      }
   }

   if (frac_solution) {
	#pragma mta assert nodep
   	for (int i=0; i<nloc+nnz; i++) {
		frac_solution[i] = volp.psol[i];
   	}
   }
   printf(" ncust: %d\n", ufl_data.ncust);
   printf(" Best integer solution value: %f\n", ufl_data.icost);
   printf(" Lower bound: %f\n", volp.value);
   
#ifndef WIN32
   // end time measurement
   tres = times(&timearr);
   double t = (timearr.tms_utime-t0)/100.;
   printf(" Total Time: %f secs\n", t);
#endif

   return 0;
}

// Read cost objective data or side constraint data.
// Output: Sparse matrix, Matrix.
template <typename vtype, typename adjlist_t>
void UFL_read_data(const char* fname, UFL<vtype,adjlist_t>& data,
                   adjlist_t& Matrix) 
{
   FILE * file = fopen(fname, "r");
   if (!file) {
      printf("Failure to open ufl datafile: %s\n ", fname);
      abort();
   }


   VOL_dvector& fcost = data.fcost;
   //VOL_dvector& dist = data.dist;
   //SparseMatrixCSR<double>& _dist = data._dist;

   int& nloc = data.nloc;
   int& ncust = data.ncust;
   int& nnz = data.nnz;
   int& p = data._p;
   int len;
   int nloc2, ncust2, nnz2;
#if 1
   char s[500];
   fgets(s, 500, file);
   len = strlen(s) - 1;
   if (s[len] == '\n')
      s[len] = 0;

   // read number of locations, number of customers, and no. of nonzeros
   sscanf(s,"%d%d%d",&nloc2,&ncust2,&nnz2);
   // check consistency, all constraints must have same sparsity pattern!
   // TODO: We only check nnz for now, since nloc may shrink after ID mapping
   if (nloc==0)
     nloc = nloc2;
#if 0
   else if (nloc2 != nloc){
     // TODO Standardize error handling 
     printf("FATAL: nloc=%i is inconsistent with previous value %i\n",
       nloc2, nloc);
     exit(-1);
   }
#endif
   if (ncust==0)
     ncust = ncust2;
#if 0
   else if (ncust2 != ncust){
     // TODO Standardize error handling 
     printf("FATAL: ncust=%i is inconsistent with previous value %i\n",
       ncust2, ncust);
     exit(-1);
   }
#endif
   if (nnz==0)
     nnz = nnz2;
   else if (nnz2 != nnz){
     // TODO Standardize error handling 
     printf("FATAL: nnz=%i is inconsistent with previous value %i\n",
       nnz2, nnz);
     exit(-1);
   }

   fcost.allocate(nloc);
   //dist.allocate(nloc*ncust);
   int    *m_index  =  (int*)    malloc((nloc+1) * sizeof(int));
   double *m_values =  (double*) malloc(nnz    * sizeof(double));
   int    *m_columns = (int*)    malloc(nnz    * sizeof(int));
   double cost;
   int i,j,k;
   if (p==0) { // UFL 
     // read location costs
     for (i = 0; i < nloc; ++i) { 
       fgets(s, 500, file);
       len = strlen(s) - 1;
       if (s[len] == '\n')
  	s[len] = 0;
       sscanf(s,"%lf",&cost);
       fcost[i]=cost;
     }
   }
   else { // p-median
     // No facility open costs; set them to zero.
     for (i = 0; i < nloc; ++i) 
       fcost[i]=0.0;
   }

   //dist=1.e7;
   nnz = 0;
   int nextID = 0;
   int newi = 0;
   char *fgr = fgets(s, 500, file);
   k=sscanf(s,"%d%d%lf",&i,&j,&cost);
   assert(k==3);
   int row = 0;
   int cur_loc = i;
   int next_loc = i;
   int nz_count = 0;
   m_index[row] = 0;
   while(fgr) {
     len = strlen(s) - 1;
     if (s[len] == '\n')
	s[len] = 0;
     if (next_loc != cur_loc) {
	row++;
	m_index[row] = nz_count;
	cur_loc = next_loc;
      }
     // read cost of serving a customer from a particular location
     // i=location, j=customer

     // SPOT:
     // Create a set of location IDs that is consecutive beginning at 0.
     // Customer (event) IDs are already consecutive beginning at 1.
     // Dummy location is "nloc".
     // "p" (number of sensors) was incremented by 1 because dummy
     // location will be one of the those found by ufl.

     newIDiter = newID.find(i);
     if (newIDiter == newID.end()){

       newi = newID[i] = nextID;

       origID.push_back(i);
       if (i == nloc){         // sp sends dummy location as nloc
         newDummyID = nextID;
       }
       nextID++;
     }
     else{
       newi = newIDiter->second;
     }

     m_values[nz_count] = cost;
     m_columns[nz_count] = j-1;

     ++nnz;  
     nz_count++;

     fgr = fgets(s, 500, file);
     k=sscanf(s,"%d%d%lf",&i,&j,&cost);
     if(k!=3) break;
     if(i==-1)break;
     next_loc = i;
   }
   row++;
   m_index[row] = nz_count;
   newID.erase(newID.begin(), newID.end());

   nloc = row;

   Matrix.init(nloc,ncust,nnz,m_index,m_values,m_columns);

// DEBUG
/*
   for (int i=0; i<nloc; i++) {
     vtype * begin_vals = Matrix.col_values_begin(i);
     int    * begin_cols = Matrix.col_indices_begin(i);
     vtype * end_vals = Matrix.col_values_end(i);
     int    * end_cols = Matrix.col_indices_end(i);
     vtype *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       vtype distij = *ptr1;
       printf("(%d:%d) ", j, (int) distij);
     }
     printf("\n");
   }
*/
// END DEBUG

#else // DEAD CODE!
   fscanf(file, "%i%i", &ncust, &nloc);
   fcost.allocate(nloc);
   //dist.allocate(nloc*ncust);
   int i,j;
   for ( j=0; j<ncust; ++j){
     for ( i=0; i<nloc; ++i){
       fscanf(file, "%f", &Matrix[i][j]);
     }
   }
   for ( i=0; i<nloc; ++i)
     fscanf(file, "%f", &fcost[i]);
#endif
   fclose(file);

   data.fix.allocate(nloc);
   data.fix = -1;
   //if (!verifyMap(dist,ncust,Matrix))  
//	fprintf(stderr, "INCORRECT MAP\n");
}



template <typename vtype, typename adjlist_t>
void UFL<vtype,adjlist_t>::init_u(VOL_dvector& u)
{
	printf("UFL::init_u called\n");
	if (!init_lagrange) return;
	int usize = u.size();
	#pragma mta assert nodep
	for (int i=0; i<usize; i++) {
		u[i] = init_lagrange[i];
	}
}

template <typename vtype, typename adjlist_t>
double UFL<vtype,adjlist_t>::open_more_servers(VOL_ivector& nsol, 
				int*unserved, int num_unserved)
{
   printf("open_more_servers()\n");
   int num_extra=0;
   double extra_cost = 0.0;
   int *best_server = new int[num_unserved];
   memset(best_server, -1, num_unserved*sizeof(int));
   #pragma mta assert parallel
   for (int i=0; i<nloc; i++) {
     	int i_index = _dist.col_index(i);
     	int next_index = _dist.col_index(i+1);
     	column_iterator begin_cols = _dist.col_indices_begin(i);
     	column_iterator end_cols = _dist.col_indices_end(i);
     	column_iterator ptr2 = begin_cols;
     	double best_cost = fcost[i];
     	for (int k=i_index; k<next_index; k++) {
     		column_iterator my_ptr2 = ptr2;
		my_ptr2.set_position(k-i_index);
       		int j = *my_ptr2;
		for (int p=0; p<num_unserved; p++) {
#ifdef DEBUG
			if (j == unserved[p]) {
				printf("ufl: %d is unserved[%d]\n", 
						unserved[p], p);
				fflush(stdout);
				printf("ufl: best_server[%d] is %d\n", 
						p, best_server[p]);
				fflush(stdout);
				if (best_server[p] >= 0) {
				printf("ufl: fcost[best_server[%d]] is %f\n", 
						p, fcost[best_server[p]]);
				fflush(stdout);
				printf("ufl: fcost[server %d] is %f\n", 
						i, fcost[i]);
				fflush(stdout);
				}
			}
#endif
			if ((j == unserved[p]) &&
			    ((best_server[p] == -1) ||
			     (fcost[i] < fcost[best_server[p]]))) {
					best_server[p] = i;
			}
		}
     	}
   }
   #pragma mta assert parallel
   for (int p=0; p<num_unserved; p++) {
	int uns = unserved[p];
	int serv= best_server[p];
	int token = mt_incr(nsol[serv], 1);
	if (token == 0) {
		extra_cost += fcost[serv];
	} else {
		mt_incr(nsol[serv], -1);
	}
   }
   return extra_cost;
}

template <typename vtype, typename adjlist_t>
int * UFL<vtype,adjlist_t>::find_unserved(VOL_ivector& nsol, int num_servers,
					  int& num_unserved)
{
   int *server = new int[num_servers];
   int rows = _dist.MatrixRows();
   bool *unserved = new bool[ncust];
   int next_server_index = 0;

   #pragma mta assert nodep
   for (int i=0; i<ncust; i++) {
	unserved[i] = true;
   }
   #pragma mta assert parallel
   for (int i=0; i<rows; i++) {
	if (nsol[i] == 1) {
		int next = mt_incr(next_server_index, 1);
		server[next] = i;
	}
   }
   #pragma mta assert parallel
   #pragma mta loop future
   for (int i=0; i<num_servers; i++) {
     int i_index = _dist.col_index(server[i]);
     int next_index = _dist.col_index(server[i]+1);
     column_iterator begin_cols = _dist.col_indices_begin(server[i]);
     column_iterator ptr2 = begin_cols;
     //printf("funs: server %d of %d: (%d-%d)\n", i, num_servers,next_index,
//			i_index);
     //fflush(stdout);
     if (next_index-i_index > PARCUTOFF) {
	#pragma mta assert parallel
     	for (int k=i_index; k<next_index; k++) {
		int offs = k-i_index;
		ptr2.set_position(offs);
       		int j = *ptr2;
		unserved[j] = false;
     	}
     } else {
     	for (int k=i_index; k<next_index; k++) {
     		//printf("funs: server %d of %d: adj %d\n", i, 
		//		num_servers,k);
     		fflush(stdout);
		int offs = k-i_index;
		ptr2.set_position(offs);
       		int j = *ptr2;
		unserved[j] = false;
     	}
     }
   }
   int ucount=0;
   #pragma mta assert parallel
   for (int j=0; j<ncust; j++) {
	if (unserved[j]) { 
		ucount += 1;
	}
   }
   num_unserved = ucount;
   int *the_unserved = new int[ucount];
   ucount = 0;
   #pragma mta assert parallel
   for (int j=0; j<ncust; j++) {
	if (unserved[j]) {   
		int ind = mt_incr(ucount,1);
		the_unserved[ind] = j;
		//printf("unserved: %d\n", j);
	}
  }
  printf("find_unserved: unserved: %d\n", ucount);
  delete[] unserved;
  delete[] server;
  return the_unserved;
}

template <typename vtype, typename adjlist_t>
double UFL<vtype,adjlist_t>::assign_service(VOL_ivector& nsol, 
					    const VOL_dvector& x)
{
   int rows = _dist.MatrixRows();
   bool *used = new bool[rows];
   double *best_serverness = new double[ncust];
   int next_server_index = 0;

   #pragma mta assert nodep
   for (int i=0; i<rows; i++) {
	used[i] = false;
   }
   #pragma mta assert nodep
   for (int i=0; i<ncust; i++) {
	best_serverness[i] = 0.0;
   }

   #pragma mta assert parallel
   for (int i=0; i<nloc; i++) {
     int i_index = _dist.col_index(i);
     int next_index = _dist.col_index(i+1);
     column_iterator begin_cols = _dist.col_indices_begin(i);
     column_iterator end_cols = _dist.col_indices_end(i);
     column_iterator ptr2 = begin_cols;
     double serverness = nsol[i] * x[i];
     //printf("assign_serv: serverness[%d]: %f\n", i, serverness);
     //printf("assign_serv: %d:  index[%d,%d]\n", i, i_index, next_index);
     for (int k=i_index; k<next_index; k++) {
     		double my_serverness = serverness;
		column_iterator my_ptr = ptr2;
		my_ptr.set_position(k-i_index);
       		int j = *my_ptr;
     //		printf("assign_serv: k: %d, pos: %d, (%d,%d): serverness: %f\n", k, k-i_index, i, j,serverness);
		if (my_serverness > best_serverness[j]) {
			best_serverness[j] = my_serverness;
//			printf("assign_serv: updating best_serv[%d] to be %d (%f)\n",
//				j,i,my_serverness);
		}
     }
     int token = 0;
     #pragma mta assert parallel
     for (int k=i_index; k<next_index; k++) {
		column_iterator my_ptr = ptr2;
		my_ptr.set_position(k-i_index);
       		int j = *my_ptr;
		if (nsol[i] && (x[i] == best_serverness[j])) {
			int got_it = mt_incr(token,1);
			if (got_it==0) {
				used[i] = true;
			}
		}
     }
   }
   int s_ind = 0;
   #pragma mta assert parallel
   for (int i=0; i<rows; i++) { 
	if ((nsol[i] == 1) && used[i]) {
		int next = mt_incr(s_ind, 1);
	} else {
		nsol[i] = 0;
	}
   }
   int num_servers = s_ind;
   int *served = new int[ncust];
   int *server = new int[num_servers];
   s_ind = 0;

   memset(served,0,ncust*sizeof(int));

   #pragma mta assert parallel
   for (int i=0; i<rows; i++) { 
	if (nsol[i] == 1) {
		int next = mt_incr(next_server_index, 1);
		server[next] = i;
	}
   }
   double cost = 0.0;

   #pragma mta assert parallel
   for (int ind=0; ind<next_server_index; ind++) {
     int i = server[ind];
     int i_index = _dist.col_index(i);
     int next_index = _dist.col_index(i+1);
     column_iterator begin_cols = _dist.col_indices_begin(i);
     column_iterator end_cols = _dist.col_indices_end(i);
     value_iterator begin_vals = _dist.col_values_begin(i);
     value_iterator end_vals = _dist.col_values_end(i);
     column_iterator ptr2 = begin_cols;
     value_iterator ptr1 = begin_vals;
     double tmp_cost = 0;
     	for (int k=i_index; k<next_index; k++) {
     		column_iterator my_ptr = ptr2;
		my_ptr.set_position(k-i_index);
       		int j = *my_ptr;
		int got_it = 0;
		got_it = mt_incr(served[j], 1);  
		//printf("assign_serv: %d trying to serve %d\n", i, j);
		if (got_it == 0) {
		//	printf("assign_serv: %d succeeds in serving %d (index %d)\n",i,j,nloc+k);
			ptr1.set_position(k-i_index);
			double distij = *ptr1;
			tmp_cost += distij;
			nsol[nloc+k] = 1;
		}
     	}
     cost += tmp_cost;
   }
   delete [] used;
   delete [] served;
   delete [] server;
   delete [] best_serverness;
   //delete [] hd;
   //delete [] ld;
   return cost;
}

// IN:  fractional primal solution (x),
//      best feasible soln value so far (icost)
// OUT: integral primal solution (ix) if better than best so far
//      and primal value (icost)
// returns -1 if Volume should stop, 0/1 if feasible solution wasn't/was
//         found.
// We use randomized rounding. We look at x[i] as the probability
// of opening facility i.

template <typename vtype, typename adjlist_t>
int
UFL<vtype,adjlist_t>::heuristics(const VOL_problem& p,
		const VOL_dvector& x, double& new_icost, double lb)
{
   VOL_ivector nsol(nloc + nnz);
   nsol=0;
   int i,j;
   double r,value=0;
   int state = 12345;
   bool int_sol_found=false;
   double *xmin = new double[ncust];
   int *kmin = new int[ncust];
   int fail = 0;
   int nopen=0;

  if (fix.size() > 0 ) {
	printf("WARNING: heuristics currently ignores fixed locations\n");
  }

  nopen=0;
  value = 0;

  random_value* rand = (random_value*) malloc(nloc * sizeof(random_value));
  rand = rand_fill::generate(nloc, rand);

  if (_p > 0) {  // p_median
   for (int i=0; i < nloc; ++i){ // open or close facilities
     r=rand[i]/(double)rand_fill::get_max();
     // Simple randomized rounding
     // TODO: Improve for p-median case (Jon & Cindy)
     if (r < x[i]) 
         if (nopen < _p){
           nsol[i]=1;
           ++nopen;
         }
   }
   if (nopen < _p) fail = true;
  } else {	 // UFL
   #pragma mta assert parallel
   for (int i=0; i < nloc; ++i){ // open or close facilities
     r=rand[i]/(double)rand_fill::get_max();
     if (r < x[i]) {
           nsol[i]=1;
           mt_incr(nopen, 1);
	   value += fcost[i];
     }
   }
   int num_unserved = 0;
   int *the_unserved = find_unserved(nsol, nopen, num_unserved);
   value += open_more_servers(nsol, the_unserved, num_unserved);
   value += assign_service(nsol, x);
  }

   new_icost = value;
   if (value < icost) {
      icost = value;
      ix = nsol;
    //printf("heuristics recording a feasible solution:\n");
    //for (int i=0; i<nloc; i++) {
//	printf("s[%d] == %d\n", i, ix[i]);
    //}
   }
   /*
   printf("ICOST: %d\n", icost);
   printf("LB   : %lf\n",lb);
   printf("GAP  : %lf\n",(icost-lb)/lb);
   */
   if (lb > icost) {
   	printf("lb: %lf exceeds icost: %lf\n", lb, icost);
   }
   if (lb > 0 && ((icost - lb)/lb < gap) || (lb == 0 && icost < gap)) {
   	printf("HEURISTICS SAYS TO QUIT! (%lf - %lf)/%lf < %lf\n",
			icost, lb, lb, gap);
	return -1;
   }
   //printf("int sol %f\n", new_icost);
   delete [] xmin;
   delete [] kmin;

   return 1;
}

// We assume the side constraint sparsity pattern is the same as 
// the costs _dist, without checking.
// TODO: If not, we need merge the two sparsity patterns.
// This affects the length of the rc vector!
template <typename vtype, typename adjlist_t>
int
UFL<vtype,adjlist_t>::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
{
   int i,j,k=0;

   //printf("Debug in comp_rc: Multipliers u (size=%i) = \n", u.size());
   //for (i=0; i<u.size(); i++)
   //  printf("%lf ", u[i]);
   //printf("\n");
 
// Standard UFL or p-median objective

   #pragma mta assert parallel
   #pragma mta loop future
   for ( i=0; i < nloc; i++){
     rc[i]= fcost[i];
     int i_index = _dist.col_index(i);
     int next_index = _dist.col_index(i+1);
     value_iterator  begin_vals = _dist.col_values_begin(i);
     column_iterator begin_cols = _dist.col_indices_begin(i);
     value_iterator  end_vals = _dist.col_values_end(i);
     column_iterator end_cols = _dist.col_indices_end(i);
     value_iterator  ptr1 = begin_vals; 
     column_iterator ptr2 = begin_cols;
     if (next_index-i_index > PARCUTOFF) {
	int offs = 0;
	#pragma mta assert parallel
     	for (int k=i_index; k<next_index; k++, offs++) {
		value_iterator my_ptr1 = ptr1;
		column_iterator my_ptr2 = ptr2;
		my_ptr1.set_position(offs);
		my_ptr2.set_position(offs);
       		int j = *my_ptr2;
       		vtype distij = *my_ptr1;
       		rc[nloc+k]= distij - u[j];
     	}
     } else {
     	for (int k=i_index; k<next_index; k++, ptr1++, ptr2++) {
       		int j = *ptr2;
       		vtype distij = *ptr1;
       		rc[nloc+k]= distij - u[j];
     	}
     }
   }

   // Additional (linear) side constraints
   int c=0;
   typename vector<adjlist_t>::iterator sc; 
   for (sc = _sidecon.begin(), c=0; sc != _sidecon.end(); ++sc,++c) {
     k= 0;
     double mult = u[ncust+c]; // Multiplier for side constraint c
     //printf("Compute_rc for constraint %d: lambda = %f\n", c, lambda);
     for ( i=0; i < nloc; i++){
       value_iterator begin_vals = sc->col_values_begin(i);
       value_iterator end_vals = sc->col_values_end(i);
       value_iterator ptr1 = begin_vals;
       for (; ptr1 < end_vals; ptr1++) {
         rc[nloc+k] += mult * (*ptr1);
         ++k;
       }
     }
   }

   return 0;
}

/*
template <typename vtype, typename adjlist_t>
void 
UFL<vtype,adjlist_t>::separate_by_degree()
{
        int *index = _dist.get_index();
	
	if (twenty_fifth == 0) {
	  int *degs = new int[nloc];
     
          #pragma mta assert nodep
	  for (int i=0; i<nloc; i++) {
	    degs[i] = index[i+1] - index[i];
	  }
	  int maxval = 0;
          #pragma mta assert nodep
	  for (int i=0; i<nloc; i++) {
	    if (degs[i] > maxval)
	      maxval = degs[i];  // compilers removes reduction
	  }
	  mtgl::countingSort(degs, nloc, maxval);
	  twenty_fifth = degs[nloc-25];
	  //twenty_fifth = 5000;
	  delete [] degs;
	  printf("set twenty_fifth to %d vs PARCUTOFF of %d\n", twenty_fifth, PARCUTOFF);
	}
	

	int next_hd =0;
	int next_ld =0;
	
	#pragma mta assert nodep
	for (int i=0; i<nloc; i++) {
	  	if ((index[i+1] - index[i]) > twenty_fifth) {     
			next_hd++;
		} else {
			next_ld++;
		}
	}
	n_hd = next_hd;
	n_ld = next_ld;
	if (n_hd > 0) high_degree = new int[n_hd];
	if (n_ld > 0) low_degree  = new int[n_ld];
	next_hd = next_ld = 0;
	#pragma mta assert parallel
	for (int i=0; i<nloc; i++) {
	  	if ((index[i+1] - index[i]) > twenty_fifth) {     
			int ind = mt_incr(next_hd,1);
			high_degree[ind] = i;
		} else {
			int ind = mt_incr(next_ld,1);
			low_degree[ind] = i;
		}
	}
	printf("n_hd: %d\n", n_hd);
	printf("n_ld: %d\n", n_ld);
}

template <typename vtype, typename adjlist_t>
void 
UFL<vtype,adjlist_t>::separate_by_degree(int *to_sep, int num_to_sep,
					 int *&hd, int& num_hd,
					 int *&ld, int& num_ld)
{
	int next_hd =0;
	int next_ld =0;
	int *index = _dist.get_index();
	#pragma mta assert nodep
	for (int ind=0; ind<num_to_sep; ind++) {
		int i = to_sep[ind];
		if ((index[i+1] - index[i]) > PARCUTOFF) {
			next_hd++;
		} else {
			next_ld++;
		}
	}
	num_hd = next_hd;
	num_ld = next_ld;
	hd = new int[num_hd];
	ld  = new int[num_ld];
	next_hd = next_ld = 0;
	#pragma mta assert parallel
	for (int ind=0; ind<num_to_sep; ind++) {
		int i = to_sep[ind];
		if ((index[i+1] - index[i]) > PARCUTOFF) {
			int ind = mt_incr(next_hd,1);
			hd[ind] = i;
		} else {
			int ind = mt_incr(next_ld,1);
			ld[ind] = i;
		}
	}
}
*/
		

// IN: dual vector u
// OUT: primal solution to the Lagrangian subproblem (x)
//      optimal value of Lagrangian subproblem (lcost)
//      v = difference between the rhs and lhs when substituting
//                  x into the relaxed constraints (v)
//      objective value of x substituted into the original problem (pcost)
//      xrc
// return value: -1 (volume should quit) 0 infeasible 1 feasible

template <typename vtype, typename adjlist_t>
int 
UFL<vtype,adjlist_t>::solve_subproblem(const VOL_dvector& u, 
				       const VOL_dvector& rc,
		      		       double& lcost, 
		      		  VOL_dvector& x, VOL_dvector& v, double& pcost)
{
   int i,j;

   double lc=0.0;
   double *u_v = u.v;
   for (i = 0; i < ncust; ++i) {
      double incr = u_v[i];
      lc += incr;
   }
   lcost = lc;

   int my_nloc = nloc;

   // VOL_ivector sol(nloc + nloc*ncust);
   VOL_ivector sol(x.size());
   int *sol_v = sol.v;
   double *fcost_v = fcost.v;

   // Produce a primal solution of the relaxed problem
   // For p-median, we can only open p facilities
 
   const double * rdist = rc.v + my_nloc;
   int k=0, k1=0;
   double value=0.;
   int    num_open=0;
   int xi;
   std::map<int,double>::iterator col_it;
   double sum_fcosts = 0.0;

   if (_p == 0) { // UFL
     //int first_loop_concur = mta_get_task_counter(RT_CONCURRENCY);
     //mt_timer mttimer;
     //mttimer.start();
     // first treat the high degree nodes in serial
     // NOTE: the version that tried load balance masks is in svn 1330

	mt_timer timer;
	int issues, memrefs, concur, streams, traps;

     int *fix_v = fix.v;
     double *fcost_v = fcost.v;
     int fix_size = fix.size();

     #pragma mta assert parallel
     for (int i=0; i < my_nloc; ++i ) {
       double sum=0.;
       double tmp_value=0.;
       int xi;
       int i_index = _dist.col_index(i);
       int next_index = _dist.col_index(i+1);
       #pragma mta assert nodep
       for (int k=i_index; k<next_index; k++) {	
         if ( rdist[k]<0. ) { 
	   sum += rdist[k];
	 }
	 sol_v[my_nloc+k] = 0;
       }
       if (fix_size > 0) {
	 if (fix_v[i]==0) xi=0;
	 else 
	   if (fix_v[i]==1) xi=1;
	   else 
	     if ( fcost_v[i]+sum >= 0. ) xi=0;
	     else xi=1;
       } else {
	 if ( fcost_v[i]+sum >= 0. ) 
	   xi=0;
	 else 
	   xi=1;
       }
       sol_v[i]=xi;
       double cost_incr = xi * fcost_v[i];
       sum_fcosts += cost_incr;
       if (xi == 1) {
	 mt_incr(num_open,1);
	 
	 int i_index = _dist.col_index(i);
	 int next_index = _dist.col_index(i+1);
	 column_iterator begin_cols = _dist.col_indices_begin(i);
	 column_iterator end_cols   = _dist.col_indices_end(i);
	 column_iterator ptr1 = begin_cols;
         #pragma mta assert parallel
	 for (int k=i_index; k<next_index; k++) {
	   ptr1.set_position(k-i_index);
	   int j = *ptr1;
	   if ( rdist[k] < 0. ) {
	     ptr1.set_position(k-i_index);
	     int j = *ptr1;
	     sol_v[my_nloc+k]=(j+1); // remember who's served
	     tmp_value += rdist[k];
	     // value += rdist[k] gives internal compiler error with loop future
	   } 
	 }
       }
       value += tmp_value;
	//sample_mta_counters(timer, issues, memrefs, concur, streams);
	//printf("---------------------------------------------\n");
	//printf("solve_sub hd loop: \n");
	//printf("---------------------------------------------\n");
	//print_mta_counters(timer, nnz, issues,memrefs,concur,streams);
     }


	//printf("---------------------------------------------\n");
	//printf("solve_sub ld loop: \n");
	//printf("---------------------------------------------\n");
	//print_mta_counters(timer, nnz, issues,memrefs,concur,streams);
     value += sum_fcosts;
   }
   else { // _p>0, p-median 
     // p lowest rho, as in Avella et al.
     // (index, value) pairs
     std::vector<std::pair<int,double> > rho(_p+1); 
     std::pair<int,double> rho_temp; 
     int q;

     for (q=0; q<_p; q++) {
       rho[q].first = -1;
       rho[q].second = 0;
     }

     for ( i=0; i < my_nloc; ++i ) {
       double sum=0.;
       column_iterator begin_cols = _dist.col_indices_begin(i);
       column_iterator end_cols   = _dist.col_indices_end(i);
       for (column_iterator ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         if ( rdist[k]<0. ) sum+=rdist[k];
         ++k;
       }

       // Save p lowest rho values
       rho_temp.first = i;
       // rho_temp.second = sum - u[i]; // Avelli et al. use y[j] for x[j,j]
       rho_temp.second = sum;
       rho[_p] = rho_temp;
       for (q=_p; q>0; q--){
         if (rho[q-1].first == -1 || (rho[q].second < rho[q-1].second)){
           rho_temp = rho[q-1];
           rho[q-1] = rho[q];
           rho[q] = rho_temp;
         }
         else
           break;
       }
     }

     // Compute x and function value based on smallest rho values
     value = 0;
     for ( i=0; i < my_nloc+nnz; ++i ) {
       sol[i] = 0;
     }
     // Loop over open locations
     int ii;
     for ( ii=0; ii < _p; ++ii ) {
       i = rho[ii].first;
       // Open a facility at location i
       sol[i]=1;
       //printf("solvesp: Debug: Open facility at location %d, rho= %f\n",
       //   i, rho[ii].second);
       value+= rho[ii].second;
       int i_index = _dist.col_index(i);
       int next_index = _dist.col_index(i+1);
       column_iterator begin_cols = _dist.col_indices_begin(i);
       column_iterator end_cols   = _dist.col_indices_end(i);
       column_iterator ptr1 = begin_cols;
       for (int k=i_index; k<next_index; k++, ptr1++) {
         // Set customer-facility variables
         if ( rdist[k-my_nloc] < 0. ) {
		int j = *ptr1;
		//printf("assigning customer %d to %d\n", j, i);
		sol[k] = 1;
	 }
       }
     }
   }

   lcost += value; 

   double pcost_tmp = 0.0;
   x = 0.0;
   double *x_v = x.v;
   #pragma mta assert parallel
   for (i = 0; i < my_nloc; ++i) {
     if (_p == 0) { // Include facility open costs for UFL, not p-median
       double incr = fcost_v[i] * sol_v[i];
       pcost_tmp += incr;
     }
     x_v[i] = sol_v[i];
   }

   // Compute x, v, pcost from sol
   k = 0;
   #pragma mta assert parallel
   for (int i=0; i < my_nloc; i++) {
     int i_index = _dist.col_index(i);
     int next_index = _dist.col_index(i+1);
     value_iterator  begin_vals = _dist.col_values_begin(i);
     column_iterator begin_cols = _dist.col_indices_begin(i);
     value_iterator  end_vals = _dist.col_values_end(i);
     column_iterator end_cols = _dist.col_indices_end(i);
     value_iterator  ptr1 = begin_vals; 
     column_iterator ptr2 = begin_cols;
     double pcost_incr = 0.0;

	int offs=0;
     	for (int k=i_index; k<next_index; k++, offs++) {
     		value_iterator  my_ptr1 = ptr1;
     		column_iterator my_ptr2 = ptr2;
		my_ptr1.set_position(offs);
		my_ptr2.set_position(offs);
       		int j = *my_ptr2;
       		vtype distij = *my_ptr1;
		double incr = distij*sol_v[my_nloc+k];
		double solnk = (sol_v[my_nloc+k] != 0);
       		pcost_incr += incr;
       		x_v[my_nloc+k]=solnk;
     	}
    pcost_tmp += pcost_incr;
   }
   pcost = pcost_tmp;


   int *tmp_v = new int[ncust];
   #pragma mta assert nodep
   for (int j=0; j<ncust; j++) {
	tmp_v[j] = 1;
   }

   int upper = my_nloc+nnz;
   #pragma mta assert nodep
   for (int k=my_nloc; k<upper; k++) {
	if (sol_v[k]) { 
		int& v = tmp_v[sol_v[k]-1];
		v -= 1;
	}
   }

   #pragma mta assert parallel
   for (int j=0; j<ncust; j++) {
	v[j] = tmp_v[j];
   }
   delete [] tmp_v;

   // Compute slack/violations of side constraints
   #pragma mta assert nodep
   for (i=0; i<_ub.size(); i++)
     v[ncust+i] = _ub[i]; 

   int c;
   typename vector<adjlist_t>::iterator sc;
   for (sc= _sidecon.begin(), c=0; sc != _sidecon.end(); ++sc,++c) {
     k = 0;
     for ( i=0; i < my_nloc; i++){
       int i_index = _dist.col_index(i);
       int next_index = _dist.col_index(i+1);
       value_iterator  begin_vals = sc->col_values_begin(i);
       column_iterator begin_cols = sc->col_indices_begin(i);
       value_iterator  end_vals = sc->col_values_end(i);
       column_iterator end_cols = sc->col_indices_end(i);
       value_iterator  ptr1 = begin_vals; 
       column_iterator ptr2 = begin_cols;
       for (int k=i_index; k<next_index; k++, ptr1++, ptr2++) {
	 ptr1.set_position(k);
	 ptr2.set_position(k);
         int j = *ptr2;
         double distij = *ptr1;
         v[ncust+c] -= distij * x[my_nloc+k]; // Update side constraint c
       }
     }
     // Update lcost (Lagrangian value)
     lcost -= u[ncust+c] * _ub[c]; // TODO Check + or - ?
   }

   return 1;
}





// DEPRECATED
// THIS USES THE MASKS
// IN: dual vector u
// OUT: primal solution to the Lagrangian subproblem (x)
//      optimal value of Lagrangian subproblem (lcost)
//      v = difference between the rhs and lhs when substituting
//                  x into the relaxed constraints (v)
//      objective value of x substituted into the original problem (pcost)
//      xrc
// return value: -1 (volume should quit) 0 infeasible 1 feasible

/* template <typename vtype, typename adjlist_t> */
/* int  */
/* UFL<vtype,adjlist_t>::solve_subproblem_with_mask(const VOL_dvector& u,  */
/* 				       const VOL_dvector& rc, */
/* 		      		       double& lcost,  */
/* 				       VOL_dvector& x, VOL_dvector& v, double& pcost, 	    */
/* 				       int **load_balance_masks, */
/* 				       int *load_balance_totals, */
/*              			       int **load_balance_parallelize_flags,     */
/* 	                               int num_load_balance_masks */
/* 						 ) */
/* { */
/*    int i,j; */

/*    double lc=0.0; */
/*    for (i = 0; i < ncust; ++i) { */
/*       double incr = u[i]; */
/*       lc += incr; */
/*    } */
/*    lcost = lc; */

/*    // VOL_ivector sol(nloc + nloc*ncust); */
/*    VOL_ivector sol(x.size()); */

/*    // Produce a primal solution of the relaxed problem */
/*    // For p-median, we can only open p facilities */
 
/*    const double * rdist = rc.v + nloc; */
/*    int k=0, k1=0; */
/*    double value=0.; */
/*    int    num_open=0; */
/*    //   int xi; */
/*    std::map<int,double>::iterator col_it; */
/*    double sum_fcosts = 0.0; */

/*    if (_p == 0) { // UFL */
/*      //int first_loop_concur = mta_get_task_counter(RT_CONCURRENCY); */
/*      //nt_timer mttimer; */
/*      //mttimer.start(); */
/*      // first treat the high degree nodes in serial */
/*      // NOTE: the version that tried load balance masks is in svn 1330 */

/*      //start new code */

/*      //could there be time savings if we index into the part of sol that we need for each vertex? */
/*      //eg. ptr = sol[offset+k]? */
/*      //also check for hotspots!!! */
/*      //we should also have a better way to pass in the mask info (struct maybe?) */
/*      for (int i = 0; i < num_load_balance_masks; i++) { */
/*        if (load_balance_parallelize_flags[i][0] == SERIAL) {     */
/* 	 for (int j = 0; j < load_balance_totals[i]; j++){ */
/* 	   //using tmp variables to see if that would cut down on hotspots */
/* 	   double tmp_sum_fcosts = 0, tmp_value = 0; */
/* 	   process_opening_serving_of_nodes(i, j, num_open, tmp_sum_fcosts, tmp_value, sol, rdist,  */
/* 					    load_balance_masks, load_balance_totals,  */
/* 					    load_balance_parallelize_flags, num_load_balance_masks); */
/* 	   sum_fcosts += tmp_sum_fcosts; */
/* 	   value      += tmp_value; */
/* 	 } */
/*        } else if (load_balance_parallelize_flags[i][0] == PARALLEL) {     */
/*        #pragma mta assert parallel */
/* 	 for (int j = 0; j < load_balance_totals[i]; j++){ */
/* 	   //using tmp variables to see if that would cut down on hotspots */
/* 	   double tmp_sum_fcosts = 0, tmp_value = 0; */
/* 	   process_opening_serving_of_nodes(i, j, num_open, tmp_sum_fcosts, tmp_value, sol, rdist, */
/* 					    load_balance_masks, load_balance_totals,  */
/* 					    load_balance_parallelize_flags, num_load_balance_masks); */
/* 	   sum_fcosts += tmp_sum_fcosts; */
/* 	   value      += tmp_value; */
/* 	 } */
/*        } else if (load_balance_parallelize_flags[i][0] == PARALLEL_FUTURE) {     */
/*          #pragma mta loop future */
/* 	 for (int j = 0; j < load_balance_totals[i]; j++){ */
/* 	   //using tmp variables to see if that would cut down on hotspots */
/* 	   double tmp_sum_fcosts = 0, tmp_value = 0; */
/* 	   process_opening_serving_of_nodes(i, j, num_open, tmp_sum_fcosts, tmp_value, sol, rdist, */
/* 					    load_balance_masks, load_balance_totals,  */
/* 					    load_balance_parallelize_flags, num_load_balance_masks); */
/* 	   sum_fcosts += tmp_sum_fcosts; */
/* 	   value      += tmp_value; */
/* 	 } */
/*        } */
/*      } */
     
/*      value += sum_fcosts; */
     
/*      //end new code */

/*      /\*     for (int ind=0; ind < n_hd; ++ind ) { */
/*        int i = high_degree[ind]; */
/*        k1 = k; */
/*        double sum=0.; */
/*        double tmp_value=0.; */
/*        int i_index = _dist.col_index(i); */
/*        int next_index = _dist.col_index(i+1); */
/*        #pragma mta assert nodep */
/*        for (int k=i_index; k<next_index; k++) {	 */
/*          if ( rdist[k]<0. ) {  */
/* 	   sum += rdist[k]; */
/* 	 } */
/* 	 sol[nloc+k] = 0; */
/*        } */
/*        if (fix.size() > 0) { */
/* 	 if (fix[i]==0) xi=0; */
/* 	 else  */
/* 	   if (fix[i]==1) xi=1; */
/* 	   else  */
/* 	     if ( fcost[i]+sum >= 0. ) xi=0; */
/* 	     else xi=1; */
/*        } else { */
/* 	 if ( fcost[i]+sum >= 0. )  */
/* 	   xi=0; */
/* 	 else  */
/* 	   xi=1; */
/*        } */
/*        sol[i]=xi; */
/*        double cost_incr = xi * fcost[i]; */
/*        sum_fcosts += cost_incr; */
/*        if (xi == 1) { */
/* 	 mt_incr(num_open,1); */
	 
/* 	 int i_index = _dist.col_index(i); */
/* 	 int next_index = _dist.col_index(i+1); */
/* 	 column_iterator begin_cols = _dist.col_indices_begin(i); */
/* 	 column_iterator end_cols   = _dist.col_indices_end(i); */
/* 	 column_iterator ptr1 = begin_cols; */
/*          #pragma mta assert parallel */
/* 	 for (int k=i_index; k<next_index; k++) { */
/* 	   if ( rdist[k] < 0. ) { */
/* 	     ptr1.set_position(k-i_index); */
/* 	     int j = *ptr1; */
/* 	     sol[nloc+k]=(j+1); // remember who's served */
/* 	     tmp_value += rdist[k]; */
/* 	     // value += rdist[k] gives internal compiler error with loop future */
/* 	   }  */
/* 	 } */
/*        } */
/*        value += tmp_value; */
/*      } */
/*      //int total_first_loop_concur = mta_get_task_counter(RT_CONCURRENCY) - */
/*      //					first_loop_concur; */
/*      //mttimer.stop(); */
/*      //int ticks = mttimer.getElapsedTicks(); */
/*      //printf("solve_sub: first loop concur: %lf\n",  */
/*      //			total_first_loop_concur / (double) ticks); */
/*      #pragma mta assert parallel */
/*      for (int ind=0; ind < n_ld; ++ind ) { */
/*        int i = low_degree[ind]; */
/*        k1 = k; */
/*        double sum=0.; */
/*        double tmp_value=0.; */
/*        int i_index = _dist.col_index(i); */
/*        int next_index = _dist.col_index(i+1); */
/*        #pragma mta assert nodep */
/*        for (int k=i_index; k<next_index; k++) {	 */
/*          if ( rdist[k]<0. ) {  */
/* 	   sum += rdist[k]; */
/* 	 } */
/* 	 sol[nloc+k] = 0; */
/*        } */
/*        if (fix.size() > 0) { */
/* 	 if (fix[i]==0) xi=0; */
/* 	 else  */
/* 	   if (fix[i]==1) xi=1; */
/* 	   else  */
/* 	     if ( fcost[i]+sum >= 0. ) xi=0; */
/* 	     else xi=1; */
/*        } else { */
/* 	 if ( fcost[i]+sum >= 0. )  */
/* 	   xi=0; */
/* 	 else  */
/* 	   xi=1; */
/*        } */
/*        sol[i]=xi; */
/*        double cost_incr = xi * fcost[i]; */
/*        sum_fcosts += cost_incr; */
/*        if (xi == 1) { */
/* 	 mt_incr(num_open,1); */
	 
/* 	 int i_index = _dist.col_index(i); */
/* 	 int next_index = _dist.col_index(i+1); */
/* 	 column_iterator begin_cols = _dist.col_indices_begin(i); */
/* 	 column_iterator end_cols   = _dist.col_indices_end(i); */
/* 	 column_iterator ptr1 = begin_cols; */
/* 	 for (int k=i_index; k<next_index; k++) { */
/* 	   if ( rdist[k] < 0. ) { */
/* 	     ptr1.set_position(k-i_index); */
/* 	     int j = *ptr1; */
/* 	     sol[nloc+k]=(j+1); // remember who's served */
/* 	     tmp_value += rdist[k]; */
/* 	     // value += rdist[k] gives internal compiler error with loop future */
/* 	   }  */
/* 	 } */
/*        } */
/*        value += tmp_value; */
/*      } */
/*      value += sum_fcosts; */

/* *\/ */
/*    } */
/*    else { // _p>0, p-median  */
/*      // p lowest rho, as in Avella et al. */
/*      // (index, value) pairs */
/*      std::vector<std::pair<int,double> > rho(_p+1);  */
/*      std::pair<int,double> rho_temp;  */
/*      int q; */

/*      for (q=0; q<_p; q++) { */
/*        rho[q].first = -1; */
/*        rho[q].second = 0; */
/*      } */

/*      for ( i=0; i < nloc; ++i ) { */
/*        double sum=0.; */
/*        column_iterator begin_cols = _dist.col_indices_begin(i); */
/*        column_iterator end_cols   = _dist.col_indices_end(i); */
/*        for (column_iterator ptr1 = begin_cols; ptr1 < end_cols; ptr1++) { */
/*          if ( rdist[k]<0. ) sum+=rdist[k]; */
/*          ++k; */
/*        } */

/*        // Save p lowest rho values */
/*        rho_temp.first = i; */
/*        // rho_temp.second = sum - u[i]; // Avelli et al. use y[j] for x[j,j] */
/*        rho_temp.second = sum; */
/*        rho[_p] = rho_temp; */
/*        for (q=_p; q>0; q--){ */
/*          if (rho[q-1].first == -1 || (rho[q].second < rho[q-1].second)){ */
/*            rho_temp = rho[q-1]; */
/*            rho[q-1] = rho[q]; */
/*            rho[q] = rho_temp; */
/*          } */
/*          else */
/*            break; */
/*        } */
/*      } */

/*      // Compute x and function value based on smallest rho values */
/*      value = 0; */
/*      for ( i=0; i < nloc+nnz; ++i ) { */
/*        sol[i] = 0; */
/*      } */
/*      // Loop over open locations */
/*      int ii; */
/*      for ( ii=0; ii < _p; ++ii ) { */
/*        i = rho[ii].first; */
/*        // Open a facility at location i */
/*        sol[i]=1; */
/*        //printf("solvesp: Debug: Open facility at location %d, rho= %f\n", */
/*        //   i, rho[ii].second); */
/*        value+= rho[ii].second; */
/*        int i_index = _dist.col_index(i); */
/*        int next_index = _dist.col_index(i+1); */
/*        column_iterator begin_cols = _dist.col_indices_begin(i); */
/*        column_iterator end_cols   = _dist.col_indices_end(i); */
/*        column_iterator ptr1 = begin_cols; */
/*        for (int k=i_index; k<next_index; k++, ptr1++) { */
/*          // Set customer-facility variables */
/*          if ( rdist[k-nloc] < 0. ) { */
/* 		int j = *ptr1; */
/* 		//printf("assigning customer %d to %d\n", j, i); */
/* 		sol[k] = 1; */
/* 	 } */
/*        } */
/*      } */
/*    } */

/*    lcost += value;  */

/*    double pcost_tmp = 0.0; */
/*    x = 0.0; */
/*    #pragma mta assert parallel */
/*    for (i = 0; i < nloc; ++i) { */
/*      if (_p == 0) { // Include facility open costs for UFL, not p-median */
/*        double incr = fcost[i] * sol[i]; */
/*        pcost_tmp += incr; */
/*      } */
/*      x[i] = sol[i]; */
/*    } */

/*    // Compute x, v, pcost from sol */
/*    k = 0; */
/*    #pragma mta assert parallel */
/*    for (int i=0; i < nloc; i++) { */
/*      int i_index = _dist.col_index(i); */
/*      int next_index = _dist.col_index(i+1); */
/*      value_iterator  begin_vals = _dist.col_values_begin(i); */
/*      column_iterator begin_cols = _dist.col_indices_begin(i); */
/*      value_iterator  end_vals = _dist.col_values_end(i); */
/*      column_iterator end_cols = _dist.col_indices_end(i); */
/*      value_iterator  ptr1 = begin_vals;  */
/*      column_iterator ptr2 = begin_cols; */
/*      double pcost_incr = 0.0; */

/* 	int offs=0; */
/*      	for (int k=i_index; k<next_index; k++, offs++) { */
/* 		ptr1.set_position(offs); */
/* 		ptr2.set_position(offs); */
/*        		int j = *ptr2; */
/*        		vtype distij = *ptr1; */
/* 		double incr = distij*sol[nloc+k]; */
/* 		double solnk = (sol[nloc+k] != 0); */
/*        		pcost_incr += incr; */
/*        		x[nloc+k]=solnk; */
/*      	} */
/*     pcost_tmp += pcost_incr; */
/*    } */
/*    pcost = pcost_tmp; */


/*    int *tmp_v = new int[ncust]; */
/*    #pragma mta assert nodep */
/*    for (int j=0; j<ncust; j++) { */
/* 	tmp_v[j] = 1; */
/*    } */

/*    int upper = nloc+nnz; */
/*    #pragma mta assert nodep */
/*    for (int k=nloc; k<upper; k++) { */
/* 	if (sol[k]) {  */
/* 		int& v = tmp_v[sol[k]-1]; */
/* 		v -= 1; */
/* 	} */
/*    } */

/*    #pragma mta assert parallel */
/*    for (int j=0; j<ncust; j++) { */
/* 	v[j] = tmp_v[j]; */
/*    } */
/*    delete [] tmp_v; */

/*    // Compute slack/violations of side constraints */
/*    #pragma mta assert nodep */
/*    for (i=0; i<_ub.size(); i++) */
/*      v[ncust+i] = _ub[i];  */

/*    int c; */
/*    typename vector<adjlist_t>::iterator sc; */
/*    for (sc= _sidecon.begin(), c=0; sc != _sidecon.end(); ++sc,++c) { */
/*      k = 0; */
/*      for ( i=0; i < nloc; i++){ */
/*        int i_index = _dist.col_index(i); */
/*        int next_index = _dist.col_index(i+1); */
/*        value_iterator  begin_vals = sc->col_values_begin(i); */
/*        column_iterator begin_cols = sc->col_indices_begin(i); */
/*        value_iterator  end_vals = sc->col_values_end(i); */
/*        column_iterator end_cols = sc->col_indices_end(i); */
/*        value_iterator  ptr1 = begin_vals;  */
/*        column_iterator ptr2 = begin_cols; */
/*        for (int k=i_index; k<next_index; k++, ptr1++, ptr2++) { */
/* 	 ptr1.set_position(k); */
/* 	 ptr2.set_position(k); */
/*          int j = *ptr2; */
/*          double distij = *ptr1; */
/*          v[ncust+c] -= distij * x[nloc+k]; // Update side constraint c */
/*        } */
/*      } */
/*      // Update lcost (Lagrangian value) */
/*      lcost -= u[ncust+c] * _ub[c]; // TODO Check + or - ? */
/*    } */

/*    return 1; */
/* } */

template <typename vtype, typename adjlist_t>
  void UFL<vtype,adjlist_t>::process_opening_serving_of_nodes(int mask_iteration, int mask_vertex_index, int &num_open, double &sum_fcosts, double &value, VOL_ivector &sol, const double *rdist, int **load_balance_masks, int *load_balance_totals, int **load_balance_parallelize_flags, int num_load_balance_masks) {
  
  int vertex_id = load_balance_masks[mask_iteration][mask_vertex_index];
  int i_index = _dist.col_index(vertex_id);
  int next_index = _dist.col_index(vertex_id + 1);
  int xi;
  double sum = 0.0;

  if (load_balance_parallelize_flags[mask_iteration][1] == SERIAL) {    
    for (int k = i_index; k < next_index; k++){
      if (rdist[k] < 0.0) {
	sum += rdist[k];
      }
      sol[nloc+k] = 0;
    }
  } else if (load_balance_parallelize_flags[mask_iteration][1] == PARALLEL) {    
  #pragma mta assert parallel
    for (int k = i_index; k < next_index; k++){   
      if (rdist[k] < 0.0) {
	sum += rdist[k];
      }
      sol[nloc+k] = 0;
    }
  } else if (load_balance_parallelize_flags[mask_iteration][1] == PARALLEL_FUTURE) {    
  #pragma mta loop future
    for (int k = i_index; k < next_index; k++){
      if (rdist[k] < 0.0) {
	sum += rdist[k];
      }
      sol[nloc+k] = 0;  
    }
  }

  if (fix.size() > 0) {
    if (fix[vertex_id] == 0)
      xi = 0;
    else if (fix[vertex_id] == 1)
      xi = 1;
    else if (fcost[vertex_id] + sum >= 0.0)
      xi = 0;
    else
      xi = 1;
  } else {
    if (fcost[vertex_id] + sum >= 0.0)
      xi = 0;
    else
      xi = 1;
  }

  sol[vertex_id] = xi;
  double cost_incr = xi * fcost[vertex_id];

  //TODO: make sure this is a safe addition
  sum_fcosts += cost_incr;

  //now update neighbors
  double tmp_value = 0.0;
  if (xi == 1) {
    mt_incr(num_open, 1);
    int i_index = _dist.col_index(vertex_id);
    int next_index = _dist.col_index(vertex_id+1);
    column_iterator ptr1 = _dist.col_indices_begin(vertex_id);


    // TODO: make sure that these are multithreaded safe additions
    if (load_balance_parallelize_flags[mask_iteration][1] == SERIAL) {    
      for (int k = i_index; k < next_index; k++){
	if (rdist[k] < 0.0) {
	  ptr1.set_position(k - i_index);
	  int j = *ptr1;
	  sol[nloc+k] = (j+1);
	  tmp_value += rdist[k];
	}
      }
    } else if (load_balance_parallelize_flags[mask_iteration][1] == PARALLEL) {    
      #pragma mta assert parallel
      for (int k = i_index; k < next_index; k++){   
	if (rdist[k] < 0.0) {
	  ptr1.set_position(k - i_index);
	  int j = *ptr1;
	  sol[nloc+k] = (j+1);
	  tmp_value += rdist[k];
	}
      }
    } else if (load_balance_parallelize_flags[mask_iteration][1] == PARALLEL_FUTURE) {    
      #pragma mta loop future
      for (int k = i_index; k < next_index; k++){
	if (rdist[k] < 0.0) {
	  ptr1.set_position(k - i_index);
	  int j = *ptr1;
	  sol[nloc+k] = (j+1);
	  tmp_value += rdist[k];
	}
      }
    }
    
    value += tmp_value;

  }
  
}

#endif
