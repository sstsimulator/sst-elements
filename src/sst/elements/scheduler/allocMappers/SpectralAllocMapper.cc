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

#include "SpectralAllocMapper.h"

#include "AllocInfo.h"
#include "output.h"

#include <cfloat>

using namespace SST::Scheduler;
using namespace std;

SpectralAllocMapper::SpectralAllocMapper(const Machine & mach, bool alloacateAndMap, int rngSeed) : AllocMapper(mach, alloacateAndMap)
{
 /*   if(rngSeed > 0){
        randNG = SST::RNG::MersenneRNG(rngSeed);
    } else {
        randNG = SST::RNG::MersenneRNG();
    }*/
}

SpectralAllocMapper::~SpectralAllocMapper()
{

}

std::string SpectralAllocMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    return com + "Nearest AllocMapper";
}

void SpectralAllocMapper::allocMap(const AllocInfo & ai,
                                  vector<long int> & usedNodes,
                                  vector<int> & taskToNode)
{
    if(mach.coresPerNode != 1){
        schedout.fatal(CALL_INFO, 1, "Multi-core node support is not yet implemented for SpectralAllocMapper\n");
    }

    //initialize
    std::vector<int>* freeNodes = mach.getFreeNodes();
    commMatrix = ai.job->taskCommInfo->getCommInfo();
    unsigned int numNodes = ai.getNodesNeeded();
    unsigned long mappedcount = numNodes;

    //pairs representing potential correspondence
    possPairs = vector<pair<unsigned int, unsigned long int> >(numNodes * freeNodes->size());
    pairIndices = list<unsigned long int>();
    for(unsigned int task = 0; task < numNodes; task++){
        for(unsigned long int node = 0; node < freeNodes->size(); node++){
            possPairs[task*numNodes + node] = pair<int,long int>(task, node);
            pairIndices.push_back(task*numNodes + node);
        }
    }
    pair<unsigned int, unsigned long int> toMap;
    usedNodes.clear();

    //main loop
    while(mappedcount > 0){
        //get principal eigenvector
        vector<double>* principal = principalEigenVector();

        //find max of it and select the corresponding pair
        double max = 0;
        int index = 0;
        unsigned long maxIndex = -1;
        for(list<unsigned long>::const_iterator it = pairIndices.begin(); it != pairIndices.end(); it++){
            if(principal->at(index) > max && taskToNode[possPairs[*it].first] == -1){
                max = principal->at(index);
                maxIndex = *it;
                toMap = possPairs[*it];
            }
            index++;
        }
        delete principal;

        //map
        taskToNode[toMap.first] = toMap.second;
        usedNodes.push_back(toMap.first);

        //remove conflicting pairs
        list<unsigned long>::iterator tempIt;
        for(list<unsigned long>::iterator it = pairIndices.begin(); it != pairIndices.end(); it++){
            if(possPairs[*it].first == toMap.first || possPairs[*it].second == toMap.second){
                tempIt = it;
                pairIndices.erase(it);
                it = tempIt;
                it--;
            }
        }

        //re-add mapped pair
        pairIndices.push_back(maxIndex);

        mappedcount--;
    }

    delete commMatrix;
    possPairs.clear();
    pairIndices.clear();
}

vector<double>* SpectralAllocMapper::principalEigenVector(const unsigned int maxIteration,
                                                          const double epsilon) const
{
    //initialize
    unsigned int size = pairIndices.size();
    vector<double>* bPrev = new vector<double>();
    //generate random vector with norm 1
    vector<double>* bNext = new vector<double>(size);
    for(unsigned int i = 0; i < size; i++){
        bNext->at(i) = (double) (rand() % 1000 + 1) / 1000;//randNG.nextUniform();
    }

    normalize(*bNext);

    //converge
    double error = DBL_MAX;
    for(unsigned int iter = 0; iter < maxIteration && error > epsilon; iter++){
        //iterate
        delete bPrev;
        bPrev = bNext;
        bNext = multWithM(*bPrev);
        normalize(*bNext);
        //find error
        error = 0;
        for(unsigned i = 0; i < size; i++){
            error += abs(bPrev->at(i) - bNext->at(i)) / size;
        }
    }
    delete bPrev;

    return bNext;
}

vector<double>* SpectralAllocMapper::multWithM(const vector<double> & inVector) const
{
    //vector is ordered such that t0<->n0, t0<->n1, ..., t1<->n0, t1<->n1, ...
    vector<double>* retVec = new vector<double>(pairIndices.size(), 0);
    long int resInd = 0; //index for retVec
    for(list<unsigned long>::const_iterator i = pairIndices.begin(); i != pairIndices.end(); i++){
        int task0 = possPairs[*i].first;
        unsigned long node0 = possPairs[*i].second;
        for(list<unsigned long>::const_iterator j = pairIndices.begin(); j != pairIndices.end(); j++){
            int task1 = possPairs[*j].first;
            unsigned long node1 = possPairs[*j].second;
            long int mulInd = 0; //index for inVector
            if(task0 != task1
                    && node0 != node1
                    && commMatrix->at(task0).count(task1) != 0){
                double commWeight = commMatrix->at(task0)[task1];
                long int distance = mach.getNodeDistance(node0, node1);
                retVec->at(resInd) += inVector[mulInd] * commWeight / distance;
            }
            mulInd++;
        }
        resInd++;
    }
    return retVec;
}

void SpectralAllocMapper::normalize(vector<double> & inVector) const
{
    double temp = 0;
    for(unsigned int i = 0; i < inVector.size(); i++){
        temp += pow(inVector[i],2);
    }
    temp = sqrt(temp);
    for(unsigned int i = 0; i < inVector.size(); i++){
        inVector[i] /= temp;
    }
}

