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

/* Factory file helps parse the parameters in the sdl file
 * returns correct type of machine, allocator, and scheduler
 */

#ifndef SST_SCHEDULER_FACTORY_H__
#define SST_SCHEDULER_FACTORY_H__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include "sst/core/element.h"
#pragma clang diagnostic pop
#include <string>
#include <vector>

namespace SST {
    class Params;
    namespace Scheduler {

        class Allocator;
        class AllocMapper;
        class FST;
        class Machine;
        class schedComponent;
        class Scheduler;
        class TaskMapper;

        class Factory{
            public:
                Factory(); //only sets up the output class
                Scheduler* getScheduler(SST::Params& params, int numNodes, const Machine & mach);
                Machine* getMachine(SST::Params& params, int numNodes);
                Allocator* getAllocator(SST::Params& params, Machine* m, schedComponent* sc);
                TaskMapper* getTaskMapper(SST::Params& params, Machine* mach);
                int getFST(SST::Params& params);
                std::vector<double>* getTimePerDistance(SST::Params& params);
            private:
                std::vector<std::string>* parseparams(std::string inparam);

                //the following is for easy conversion from strings to ints
                //to add a new (say) scheduler, add its string to Schedulertype
                //and schedTable[], and increment numSchedTableEntries and the
                //size of schedTable.  Then add a case to getScheduler() that calls
                //its constructor
                enum SchedulerType{
                    PQUEUE = 0,
                    EASY = 1,
                    CONS = 2,
                    PRIORITIZE = 3,
                    DELAYED = 4,
                    ELC = 5,
                };
                enum MachineType{
                    SIMPLEMACH = 0,
                    MESH = 1,
                    TORUS = 2,
                    DRAGONFLY = 3,
                };
                enum AllocatorType{
                    SIMPLEALLOC = 0,
                    RANDOM = 1,
                    NEAREST = 2,
                    GENALG = 3,
                    MM = 4,
                    MC1X1 = 5,
                    OLDMC1X1 = 6,
                    MBS = 7,
                    GRANULARMBS = 8,
                    OCTETMBS = 9,
                    FIRSTFIT = 10,
                    BESTFIT = 11,
                    SORTEDFREELIST = 12,
                    CONSTRAINT = 13,
                    ENERGY = 14,
                    HYBRID = 15,
                    NEARESTAMAP = 16,
                    SPECTRALAMAP = 17,
                    SIMPLESPREAD = 18,
                };
                enum TaskMapperType{
                    SIMPLEMAP = 0,
                    RCBMAP = 1,
                    RANDOMMAP = 2,
                    TOPOMAP = 3,
                    RCMMAP = 4,
                    NEARESTAMT = 5,
                    SPECTRALAMT = 6,
                };
                
                enum FSTType{
                    NONE = 0,
                    STRICT = 1,
                    RELAXED = 2,
                };

                struct schedTableEntry{
                    SchedulerType val;
                    std::string name;
                };
                struct machTableEntry{
                    MachineType val;
                    std::string name;
                };
                struct allocTableEntry{
                    AllocatorType val;
                    std::string name;
                };
                struct taskMapTableEntry{
                    TaskMapperType val;
                    std::string name;
                };

                struct FSTTableEntry{
                    FSTType val;
                    std::string name;
                };

                static const int numMachTableEntries = 4;
                static const int numSchedTableEntries = 6;
                static const int numFSTTableEntries = 3;
                static const int numAllocTableEntries = 19;
                static const int numTaskMapTableEntries = 7;
                
                static const machTableEntry machTable[4];
                static const schedTableEntry schedTable[6];
                static const FSTTableEntry FSTTable[3];
                static const allocTableEntry allocTable[19];
                static const taskMapTableEntry taskMapTable[7];

                SchedulerType schedulername(std::string inparam);
                MachineType machinename(std::string inparam);
                AllocatorType allocatorname(std::string inparam);
                TaskMapperType taskmappername(std::string inparam);
                FSTType FSTname(std::string inparam);
        };

    }
}
#endif
