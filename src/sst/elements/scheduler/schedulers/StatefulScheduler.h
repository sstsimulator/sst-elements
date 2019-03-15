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

#ifndef SST_SCHEDULER__STATEFULSCHEDULER_H__
#define SST_SCHEDULER__STATEFULSCHEDULER_H__

//#include <functional>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <map>
#include <set>
#include <string>
#pragma clang diagnostic pop

#include "Scheduler.h"

namespace SST {
    namespace Scheduler {
        class Job;
        class Machine;

        class SchedChange{
            protected:
                unsigned long time;
                const Machine & mach;
                int getNodesNeeded(const Job & j) const;

            public:
                bool isEnd;
                Job* j;
                SchedChange* partner;

                //need to implement some sort of comparison function for SchedChange

                const unsigned long getTime()
                {
                    return time;
                }

                SchedChange* getPartner()
                {
                    return partner;
                }

                int freeProcChange();
                char* toString();
                void print();
                SchedChange(unsigned long intime, Job* inj, bool end, const Machine & mach, SchedChange* inpartner = NULL);
                SchedChange(SchedChange* insc, const Machine & mach);
        };

        class SCComparator {
            public:
                bool operator()(SchedChange* const& first, SchedChange* const& second);
        };

        class StatefulScheduler : public Scheduler {
            private:
                static const int numCompTableEntries;
                enum ComparatorType {  //to represent type of JobComparator
                    FIFO = 0,
                    LARGEFIRST = 1,
                    SMALLFIRST = 2,
                    LONGFIRST = 3,
                    SHORTFIRST = 4,
                    BETTERFIT = 5
                };
                struct compTableEntry {
                    ComparatorType val;
                    std::string name;
                };
                static const compTableEntry compTable[6];

                std::string compSetupInfo;
                std::set<SchedChange*, SCComparator> *estSched;
                unsigned long findTime(std::set<SchedChange*, SCComparator> *sched, Job* Job, unsigned long time);

                int numProcs;
                int freeProcs;
                SchedChange* sc;
                const Machine & mach;

            public:
                void jobArrives(Job* j, unsigned long time, const Machine & mach);
                void jobFinishes(Job* j, unsigned long time, const Machine & mach);

                //Make????
                void reset();
                unsigned long scheduleJob(Job* job, unsigned long time);
                unsigned long zeroCase(std::set<SchedChange*, SCComparator> *sched, Job* filler, unsigned long time);
                Job* tryToStart(unsigned long time, const Machine & mach);
                void startNext(unsigned long time, const Machine & mach);
                std::string getSetupInfo(bool comment);
                void printPlan();
                void done()
                {
                    heart -> done(); 
                }
                void removeJob(Job* j, unsigned long time);
                StatefulScheduler* copy(std::vector<Job*>* running, std::vector<Job*>* toRun);


                class JobComparator : public std::binary_function<Job*,Job*,bool> {
                    public:
                        static JobComparator* Make(std::string typeName);  //return NULL if name is invalid
                        static void printComparatorList(std::ostream& out);  //print list of possible comparators
                        bool operator()(Job*& j1, Job*& j2) const;
                        bool operator()(Job* const& j1, Job* const& j2) const;
                        std::string toString();
                    private:
                        JobComparator(ComparatorType type);
                        ComparatorType type;
                };

                //MANAGERS:******************************************************
                class Manager {
                    protected:
                        const Machine & mach;
                    public:
                        Manager(const Machine & inmach) : mach(inmach) { }
                        StatefulScheduler* scheduler;
                        JobComparator* origcomp;
                        virtual void arrival(Job* j, unsigned long time) = 0;
                        virtual void start(Job* j, unsigned long time) = 0;
                        virtual void tryToStart(unsigned long time) = 0;
                        virtual void printPlan() = 0;
                        virtual void onTimeFinish(Job* j, unsigned long time) = 0;
                        virtual void reset() = 0;
                        virtual void done() = 0;
                        virtual void earlyFinish(Job* j, unsigned long time) = 0;
                        virtual void removeJob(Job* j, unsigned long time) = 0;
                        virtual std::string getString() = 0;
                        void compress(unsigned long time);
                        virtual Manager* copy(std::vector<Job*>* running, std::vector<Job*>* intoRun) = 0;
                };

                class ConservativeManager : public Manager{
                    public:
                        ConservativeManager(StatefulScheduler* inscheduler, const Machine & mach) : Manager(mach)
                        {
                            scheduler = inscheduler;
                        }
                        void earlyFinish(Job* j, unsigned long time)
                        {
                            compress(time);
                        }
                        void removeJob(Job* j, unsigned long time)
                        {
                            compress(time);
                        }
                        void arrival(Job* j, unsigned long time) { }
                        void start(Job* j, unsigned long time) { }
                        void tryToStart(unsigned long time){ }
                        void printPlan() { }
                        void onTimeFinish(Job* j, unsigned long time) { }
                        void reset() { }
                        void done() { }
                        std::string getString();
                        Manager* copy(std::vector<Job*>* running, std::vector<Job*>* intoRun) 
                        { 
                            return new ConservativeManager(scheduler, mach); 
                        }
                };

                class PrioritizeCompressionManager : public Manager{
                    protected:
                        std::set<Job*, JobComparator>* backfill;
                        int fillTimes;
                        int* numSBF;
                    public:
                        PrioritizeCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp, int infillTimes, const Machine & mach);
                        PrioritizeCompressionManager(PrioritizeCompressionManager* inmanager, std::set<Job*, JobComparator>* inbackfill, const Machine & mach);
                        void reset();
                        void arrival(Job* j, unsigned long time) 
                        {
                            backfill -> insert(j);
                        }
                        void start(Job *j, unsigned long time) 
                        {
                            backfill -> erase(j);
                        }
                        void printPlan();
                        void done();
                        void earlyFinish(Job* j, unsigned long time);
                        void tryToStart(unsigned long time) { }
                        void removeJob(Job* j, unsigned long time) { }
                        void onTimeFinish(Job* j, unsigned long time) { }
                        std::string getString();
                        PrioritizeCompressionManager* copy(std::vector<Job*>* running, std::vector<Job*>* intoRun); 
                };

                class DelayedCompressionManager : public Manager {
                    protected:
                        std::set<Job*, JobComparator>* backfill;
                    public:
                        DelayedCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp, const Machine & mach);
                        DelayedCompressionManager(DelayedCompressionManager* inmanager, std::set<Job*, JobComparator>* inbackfill, const Machine & mach);
                        void reset();
                        void arrival(Job* j, unsigned long time);
                        void start(Job* j, unsigned long time)
                        {
                            backfill -> erase(j);
                        }
                        void tryToStart(unsigned long time);
                        void printPlan();
                        void done();
                        void earlyFinish(Job *j, unsigned long time);
                        void fill(unsigned long time);
                        void removeJob(Job* j, unsigned long time) { }
                        void onTimeFinish(Job* j, unsigned long time) { }
                        std::string getString();
                        DelayedCompressionManager* copy(std::vector<Job*>* running, std::vector<Job*>* intoRun); 
                    private:
                        int results;
                };

                class EvenLessManager : public Manager{
                    protected:
                        std::set<Job*, JobComparator>* backfill;
                        std::set<SchedChange*, SCComparator> *guarantee;
                        std::map<Job*, SchedChange*, JobComparator> *guarJobToEvents;
                        int bftimes;
                    public:
                        EvenLessManager(StatefulScheduler* inscheduler, JobComparator* comp, int infillTimes, const Machine & mach);
                        EvenLessManager(EvenLessManager* inmanager, std::set<Job*, JobComparator>* inbackfill, const Machine & mach);
                        void deepCopy(std::set<SchedChange*, SCComparator> *from, std::set<SchedChange*, SCComparator> *to, std::map<Job*, SchedChange*, JobComparator> *toJ);
                        void backfillfunc(unsigned long time);
                        void arrival(Job* j, unsigned long time);
                        void start(Job* j, unsigned long time);
                        void tryToStart(unsigned long time){};
                        void printPlan();
                        void done(){};
                        void earlyFinish(Job* j, unsigned long time);
                        void onTimeFinish(Job* j, unsigned long time);
                        void fill(unsigned long time) { }
                        void removeJob(Job* j, unsigned long time) { }
                        void reset();
                        std::string getString();
                        EvenLessManager* copy(std::vector<Job*>* running, std::vector<Job*>* intoRun); 
                    private:
//                        int results;
                };
                //MANAGERS OVER***************************************************************

                std::map<Job*, SchedChange*, JobComparator>* jobToEvents;
                JobComparator* origcomp;

                StatefulScheduler(int numprocs, JobComparator* comp, bool dummy, const Machine & mach);
                StatefulScheduler(int numprocs, JobComparator* comp, int fillTimes, const Machine & mach);
                StatefulScheduler(int numprocs, JobComparator* comp, const Machine & mach);
                StatefulScheduler(int numprocs, JobComparator* comp, int fillTimes, bool dummy, const Machine & mach);
                StatefulScheduler(StatefulScheduler* insched, std::set<SchedChange*, SCComparator>* inestSched, Manager* inheart, std::map<Job*, SchedChange*, StatefulScheduler::JobComparator>* inJobToEvents, const Machine & mach);

            protected:
                Manager *heart;
                int getNodesNeeded(const Job & j) const;
        };


    }
}
#endif
