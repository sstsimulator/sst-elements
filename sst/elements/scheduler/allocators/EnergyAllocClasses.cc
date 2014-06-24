// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*functions that implement the actual calls to glpk for energy-aware allocators*/

#include "sst_config.h"
#include "EnergyAllocClasses.h"


#include <stdio.h>
#include <stdlib.h>
#include <vector>

#ifdef HAVE_GLPK
#include <glpk.h>
#endif 

#include "Job.h" 
#include "Machine.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"

namespace SST {
    namespace Scheduler {
        namespace EnergyHelpers {
            void roundallocarray(double * x, int processors, int numneeded, int* newx)
            {
                //rounds off the solution stored in x with the given number of processors
                //find the (numneeded) largest values in x, set these processors to value 1
                int count;
                for (count = 0; count < processors; count++)
                    newx[count] = 0;

                for (count = 0; count < numneeded; count++) {
                    int xcount = 0;
                    while(newx[xcount] != 0)
                        xcount++;
                    double max = x[xcount];
                    int maxpos = xcount;
                    for (; xcount < processors; xcount++) {
                        if (newx[xcount] == 0 && max < x[xcount]) {
                            maxpos = xcount;
                            max = x[xcount];
                        }
                    } 
                    newx[maxpos] = 1;
                }
            }

            void hybridalloc(int* oldx, int* roundedalloc, int processors, int requiredprocessors, MeshMachine* machine)
            {     

#ifdef HAVE_GLPK
                int Putil=1500;
                int Pidle=1000;
                    
                double Tsup=288.15;
                //double Tred=30;
                //double sum_inlet=0;
                //double max_inlet=0;
                
                double D[1600];
                int d_counter = 0;
                for(int i=0;i<40;i++){
                    for(int j=0;j<40;j++){
                        D[d_counter] = machine -> D_matrix[i][j];
                        d_counter++;
                    }
                }

                double Dsum[40]; //stores the sum of each row of D
                
                double oldprocessors = 0; //total number of already-allocated processors
                for(int x = 0; x < processors; x++)
                    oldprocessors += oldx[x];

                int ia[2000];
                int ja[2000];
                double ar[2000];


                int x,y;
                for (x = 1; x <= 40; x++) {
                    Dsum[x-1] = 0;
                    for(y = 1; y <= 40; y++) {
                        Dsum[x-1] += D[ARRAY(x,y)];
                    }
                }

                glp_prob *lp;
                lp = glp_create_prob();

                glp_set_prob_name(lp, "hybrid");
                glp_set_obj_dir(lp, GLP_MIN);

                //turn off glpk output
                glp_smcp parm;
                glp_init_smcp(&parm);
                parm.msg_lev = GLP_MSG_OFF;


                //rows
                glp_add_rows(lp, 81);

                int row;
                for (row = 1; row <= 40; row++) {
                    glp_set_row_bnds(lp, row, GLP_FX, Tsup + Dsum[row-1] * Pidle, Tsup + Dsum[row-1] * Pidle);
                } 

                for (row = 41; row <= 80; row++) {
                    glp_set_row_bnds(lp, row, GLP_LO, 0.0, 0.0);
                }

                glp_set_row_name(lp, 81, "Requiredprocs");
                glp_set_row_bnds(lp, 81, GLP_FX, requiredprocessors + oldprocessors, requiredprocessors + oldprocessors);


                //columns
                glp_add_cols(lp, 81);

                int col;
                for(col = 1; col <= 40; col++) {
                    if (oldx[col-1] == 0) {
                        glp_set_col_bnds(lp, col, GLP_DB, oldx[col-1], 1);
                    } else { 
                        glp_set_col_bnds(lp, col, GLP_FX, 1, 1);
                    }
                    glp_set_obj_coef(lp, col, 0);
                }

                for(col = 41; col <= 80; col++) {
                    glp_set_col_bnds(lp, col, GLP_FR, 0.0, 0.0);
                    glp_set_obj_coef(lp, col, 0);
                } 

                glp_set_col_bnds(lp, 81, GLP_FR, 0.0, 0.0);
                glp_set_obj_coef(lp, 81, 1);

                int arraycount = 1;
                //matrix
                for (row = 1; row <= 40; row++) {
                    //ia = row #, ja = col #, ar = value
                    for(col = 1; col <= 40; col++) {
                        ia[arraycount] = row;
                        ja[arraycount] = col;
                        ar[arraycount] =  D[ARRAY(row,col)] * -Putil; 
                        arraycount++;
                    }
                    ia[arraycount] = row; 
                    ja[arraycount] = 40 + row;
                    ar[arraycount] =  1; 
                    arraycount++;
                }

                for (row = 41; row <= 80; row++) {
                    ia[arraycount] = row, ja[arraycount] = row, ar[arraycount] = -1;
                    arraycount++;
                    ia[arraycount] = row, ja[arraycount] = 81, ar[arraycount] = 1;
                    arraycount++;
                }

                for (col = 1; col <= 40; col++) {
                    ia[arraycount] = 81, ja[arraycount] = col, ar[arraycount] = 1;
                    arraycount++;
                }

                //printf("arraycount is %d\n", arraycount);
                glp_load_matrix(lp, arraycount-1, ia, ja, ar);
                glp_simplex(lp, &parm);

                double allocarray[40];
                for (x = 1; x <= 40; x++) {
                    allocarray[x-1] = glp_get_col_prim(lp, x);
                }

                for(x = 0; x < 40; x++)
                    roundedalloc[x] = 0;

                roundallocarray(allocarray, 40, requiredprocessors+oldprocessors, roundedalloc);
#else
                schedout.init("", 10, 0, Output::STDOUT);
                schedout.fatal(CALL_INFO,1,"GLPK required for energy-aware scheduler");
#endif
            }

            std::vector<MeshLocation*>* getEnergyNodes(std::vector<MeshLocation*>* available, int numProcs, MeshMachine * machine)
            { 
                std::vector<MeshLocation*>* ret = new std::vector<MeshLocation*>();

                int* oldx = new int[40];
                int* newx = new int[40];
                for (int x= 0; x < 40; x++) {
                    oldx[x] = 1;
                    newx[x] = 0;
                }
                for (unsigned int x = 0; x < available -> size(); x++) { 
                    oldx[(*available)[x]->toInt(machine)] = 0;
                }
                
                hybridalloc(oldx, newx, 40, numProcs, machine);
                for (int x = 0; x < 40; x++) {
                    if (newx[x] == 1 && oldx[x] == 0) {
                        ret -> push_back(new MeshLocation(x, machine));
                    }
                }
                return ret;
            }
        }
    }
}
