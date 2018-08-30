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

#include "TaskMapInfo.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "TaskCommInfo.h"

using namespace SST::Scheduler;

TaskMapInfo::TaskMapInfo(AllocInfo* ai, const Machine & inMach) : machine(inMach)
{
    allocInfo = ai;
    job = ai->job;
    taskCommInfo = job->taskCommInfo;
    taskToNode = std::vector<long int>(job->getProcsNeeded(), -1);
    mappedCount = 0;
    avgHopDist = -1;
    numAvailCores = std::map<long int, int>();
    traffic = std::map<unsigned int, double>();
}

TaskMapInfo::~TaskMapInfo()
{
    delete allocInfo;
}

void TaskMapInfo::insert(int taskInd, long int nodeInd)
{
    if(taskToNode[taskInd] != -1){
        schedout.fatal(CALL_INFO, 1, "Attempted to map task %d of job %ld multiple times.\n", taskInd, job->getJobNum());
    }
    if(numAvailCores.count(nodeInd) == 0){
        numAvailCores[nodeInd] = machine.coresPerNode - 1;
    } else {
        numAvailCores[nodeInd]--;
        if(numAvailCores[nodeInd] < 0){
            schedout.fatal(CALL_INFO, 1, "Task %d of job %ld is mapped to node %ld, where there is no core available.\n", taskInd, job->getJobNum(), nodeInd);
        }
    }
    taskToNode[taskInd] = nodeInd;
    nodeToTasks[nodeInd].push_back(taskInd); //NetworkSim: Populate the map of nodes and the tasks mapped to them
    mappedCount++;
}

double TaskMapInfo::getAvgHopDist()
{
    //calculate average hop distance if not calculated yet
    if(avgHopDist == -1) {
        updateNetworkMetrics();
    }
    return avgHopDist;
}

std::map<unsigned int, double> TaskMapInfo::getTraffic()
{
    //calculate traffic if not calculated yet
    if(avgHopDist == -1){
        updateNetworkMetrics();
    }
    return traffic;
}


double TaskMapInfo::getMaxJobCongestion()
{
    //calculate traffic if not calculated yet
    if(avgHopDist == -1){
        updateNetworkMetrics();
    }
    return maxCongestion;
}


double TaskMapInfo::getHopBytes()
{
    //calculate traffic if not calculated yet
    if(avgHopDist == -1){
        updateNetworkMetrics();
    }
    return hopBytes;
}

void TaskMapInfo::updateNetworkMetrics()
{
    if(job->getProcsNeeded() > mappedCount){
        schedout.fatal(CALL_INFO, 1, "Network metrics of a job has been requested before all tasks are mapped.");
    }

    //hop-distance related:
    unsigned long totalHopDist = 0;
    int neighborCount = 0;

    //traffic: create a communication structure for the nodes
    std::map<unsigned int,std::map<unsigned int, double> > nodeCommInfo;
    for(int nodeIt = 0; nodeIt < allocInfo->getNodesNeeded(); nodeIt++){
        nodeCommInfo[allocInfo->nodeIndices[nodeIt]] = std::map<unsigned int, double>();
    }

    //iterate through tasks
    std::vector<std::map<int,int> >* commInfo = taskCommInfo->getCommInfo();

    for(int taskIter = 0; taskIter < job->getProcsNeeded(); taskIter++){
        //iterate through neighbors of taskIter
        for(std::map<int, int>::iterator it = commInfo->at(taskIter).begin(); it != commInfo->at(taskIter).end(); it++){
            //update hop related:
            totalHopDist += machine.getNodeDistance(taskToNode[taskIter], taskToNode[it->first]);
            neighborCount++;

            //update traffic related:
            if (nodeCommInfo[taskToNode[taskIter]].count(taskToNode[it->first]) == 0){ //no existing communication there
                nodeCommInfo[taskToNode[taskIter]][taskToNode[it->first]] = it->second;
                nodeCommInfo[taskToNode[it->first]][taskToNode[taskIter]] = it->second;
            } else { //add to existing communication
                nodeCommInfo[taskToNode[taskIter]][taskToNode[it->first]] += it->second;
                nodeCommInfo[taskToNode[it->first]][taskToNode[taskIter]] += it->second;
            }
        }
    }
    delete commInfo;

    //calculate average hop distance per neighbor
    //two-way distances and uncounted neighbors cancel each other
    avgHopDist = totalHopDist;
    if(neighborCount != 0){
        avgHopDist /= neighborCount;
    }

    //create traffic
    //iterate through all nodes
    for(unsigned int nodeIter = 0; nodeIter < nodeCommInfo.size(); nodeIter++){
        //iterate through its neighbors in the nodeCommInfo
        for(std::map<unsigned int, double>::iterator it = nodeCommInfo[nodeIter].begin();
          it != nodeCommInfo[nodeIter].end(); it++){
            //add communication to the traffic
            if(nodeIter < it->first){ //avoid duplicates
                //get used links
                std::list<int>* route = machine.getRoute(nodeIter, it->first, it->second);
                //iterate through the links to populate traffic
                for (std::list<int>::iterator linkIt = route->begin(); linkIt != route->end(); linkIt++){
                    if (traffic.count(*linkIt) == 0){ // no existing communication there
                        traffic[*linkIt] = it->second;
                    } else { //add to existing communication
                        traffic[*linkIt] += it->second;
                    }
                }
                delete route;
            }
        }
    }

    //calculate maxCongestion
    maxCongestion = 0;
    hopBytes = 0;
    for(std::map<unsigned int, double>::iterator it = traffic.begin(); 
      it != traffic.end(); 
      it++){
        hopBytes += it->second;
        if(it->second > maxCongestion){
            maxCongestion = it->second;
        }
    }
    hopBytes *= 256 * 1024 * 1024; //assumes 256MB/sec, need to change this
}
