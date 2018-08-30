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

            void hybridalloc(int* oldx, int* roundedalloc, int processors, int requiredprocessors, const Machine & machine)
            {     

#ifdef HAVE_GLPK
                int numNodes = machine.numNodes;
                int Putil=1500;
                int Pidle=1000;
                    
                double Tsup=288.15;
                
                double* D = new double[numNodes*numNodes];
                int d_counter = 0;
                if(machine.D_matrix == NULL){
                    schedout.fatal(CALL_INFO, 1, "heat recirculation matrix D is required for energy and hybrid allocators\n");
                }
                for(int i=0;i<numNodes;i++){
                    for(int j=0;j<numNodes;j++){
                        D[d_counter] = machine.D_matrix[i][j];
                        d_counter++;
                    }
                }

                double Dsum[numNodes]; //stores the sum of each row of D
                
                double oldprocessors = 0; //total number of already-allocated processors
                for(int x = 0; x < processors; x++)
                    oldprocessors += oldx[x];

                int* ia = new int[numNodes*(numNodes+6)];
                int* ja = new int[numNodes*(numNodes+6)];
                double* ar = new double[numNodes*(numNodes+6)];

                int x,y;
                for (x = 0; x < numNodes; x++) {
                    Dsum[x] = 0;
                    for(y = 0; y < numNodes; y++) {
                        Dsum[x] += D[numNodes*x + y];
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
                glp_add_rows(lp, numNodes*2 + 1);

                int row;
                for (row = 1; row <= numNodes; row++) {
                    glp_set_row_bnds(lp, row, GLP_FX, Tsup + Dsum[row-1] * Pidle, Tsup + Dsum[row-1] * Pidle);
                } 

                for (row = numNodes+1; row <= 2*numNodes; row++) {
                    glp_set_row_bnds(lp, row, GLP_LO, 0.0, 0.0);
                }

                glp_set_row_name(lp, 2*numNodes + 1, "Requiredprocs");
                glp_set_row_bnds(lp, 2*numNodes + 1, GLP_FX, requiredprocessors + oldprocessors, requiredprocessors + oldprocessors);


                //columns
                glp_add_cols(lp, 2*numNodes + 1);

                int col;
                for(col = 1; col <= numNodes; col++) {
                    if (oldx[col-1] == 0) {
                        glp_set_col_bnds(lp, col, GLP_DB, oldx[col-1], 1);
                    } else { 
                        glp_set_col_bnds(lp, col, GLP_FX, 1, 1);
                    }
                    glp_set_obj_coef(lp, col, 0);
                }

                for(col = numNodes + 1; col <= 2*numNodes; col++) {
                    glp_set_col_bnds(lp, col, GLP_FR, 0.0, 0.0);
                    glp_set_obj_coef(lp, col, 0);
                } 

                glp_set_col_bnds(lp, 2*numNodes + 1, GLP_FR, 0.0, 0.0);
                glp_set_obj_coef(lp, 2*numNodes + 1, 1);

                int arraycount = 1;
                //matrix
                for (row = 0; row < numNodes; row++) {
                    //ia = row #, ja = col #, ar = value
                    for(col = 0; col < numNodes; col++) {
                        ia[arraycount] = row + 1;
                        ja[arraycount] = col + 1;
                        ar[arraycount] =  D[numNodes*row + col] * -Putil; 
                        arraycount++;
                    }
                    ia[arraycount] = row + 1; 
                    ja[arraycount] = numNodes + row + 1;
                    ar[arraycount] =  1; 
                    arraycount++;
                }

                for (row = numNodes + 1; row <= 2*numNodes; row++) {
                    ia[arraycount] = row, ja[arraycount] = row, ar[arraycount] = -1;
                    arraycount++;
                    ia[arraycount] = row, ja[arraycount] = 81, ar[arraycount] = 1;
                    arraycount++;
                }

                for (col = 1; col <= numNodes; col++) {
                    ia[arraycount] = 2*numNodes + 1, ja[arraycount] = col, ar[arraycount] = 1;
                    arraycount++;
                }

                //printf("arraycount is %d\n", arraycount);
                glp_load_matrix(lp, arraycount-1, ia, ja, ar);
                glp_simplex(lp, &parm);

                double allocarray[numNodes];
                for (x = 1; x <= numNodes; x++) {
                    allocarray[x-1] = glp_get_col_prim(lp, x);
                }

                for(x = 0; x < numNodes; x++)
                    roundedalloc[x] = 0;

                roundallocarray(allocarray, numNodes, requiredprocessors+oldprocessors, roundedalloc);
                
                delete [] D;
                delete [] ia;
                delete [] ja;
                delete [] ar;
#else
                schedout.init("", 10, 0, Output::STDOUT);
                schedout.fatal(CALL_INFO,1,"GLPK is required for energy-aware scheduler\n");
#endif
            }

            std::vector<int>* getEnergyNodes(std::vector<int>* available, int numProcs, const Machine & machine)
            { 
                std::cout << "Getting energy nodes\n";
                std::vector<int>* ret = new std::vector<int>();

                int numNodes = machine.numNodes;
                int* oldx = new int[numNodes];
                int* newx = new int[numNodes];
                for (int x= 0; x < numNodes; x++) {
                    oldx[x] = 1;
                    newx[x] = 0;
                }
                for (unsigned int x = 0; x < available -> size(); x++) { 
                    oldx[available->at(x)] = 0;
                }
                
                hybridalloc(oldx, newx, numNodes, numProcs, machine);
                for (int x = 0; x < numNodes; x++) {
                    if (newx[x] == 1 && oldx[x] == 0) {
                        ret -> push_back(x);
                    }
                }
                std::cout << "Got energy nodes\n";
                return ret;
            }
        }
    }
}
