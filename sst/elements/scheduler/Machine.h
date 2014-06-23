// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Abstract base class for machines
 */

#ifndef SST_SCHEDULER_MACHINE_H__
#define SST_SCHEDULER_MACHINE_H__

namespace SST {
    namespace Scheduler {
        class AllocInfo;

        class Machine{
            public:

                Machine(int numProcs, double** D_matrix)
                {
                    if(D_matrix == NULL){ //use default: no heat recirculation
                        this -> D_matrix = new double*[numProcs];
                        for(int i=0; i < numProcs; i++){
                            this -> D_matrix[i] = new double[numProcs];
                            for(int j=0; j < numProcs; j++){
                                this -> D_matrix[i][j] = 0;
                            }
                        } 
                    } else {
                        this -> D_matrix = D_matrix;
                    }
                }

                virtual ~Machine()
                {
                    if(D_matrix != NULL){
                        for (int i = 0; i < numProcs; i++)
	                        delete[] D_matrix[i];
                        delete[] D_matrix;
                    }
                }

                virtual std::string getSetupInfo(bool comment) = 0;

                int getNumFreeProcessors() 
                {
                    return numAvail;
                }

                int getNumProcs() 
                {
                    return numProcs;
                }
                
                virtual void reset() = 0;

                virtual void allocate(AllocInfo* allocInfo) = 0;

                virtual void deallocate(AllocInfo* allocInfo) = 0;
                
                double** D_matrix;

            protected:
                int numProcs;          //total number of processors
                int numAvail;          //number of available processors
        };

        Machine* getMachine();     //defined in Main.cc

    }
}
#endif
