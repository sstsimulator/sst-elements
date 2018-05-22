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

#include "Torus3DMachine.h"
#include "output.h"

#include <algorithm>
#include <sstream>
#include <cmath>

using namespace SST::Scheduler;

Torus3DMachine::Torus3DMachine(std::vector<int> dims, int numCoresPerNode, double** D_matrix)
                         : StencilMachine(dims,
                                   3*dims[0]*dims[1]*dims[2],
                                   numCoresPerNode,
                                   D_matrix)
{
    schedout.init("", 8, 0, Output::STDOUT);
}

std::string Torus3DMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) com="# ";
    else com="";
    std::stringstream ret;
    ret << com;
    for(unsigned int i = 0; i < (dims.size() - 1) ; i++){
       ret << dims[i] << "x";
    }
    ret << dims.back() << " Torus";
    ret << ", " << coresPerNode << " cores per node";
    return ret.str();
}

int Torus3DMachine::getNodeDistance(int node1, int node2) const
{
    int totalDist = 0;
    for(unsigned int i = 0; i < dims.size(); i++){
        totalDist += std::min(mod(coordOf(node2, i) - coordOf(node1, i), dims[i]),\
                              mod(coordOf(node1, i) - coordOf(node2, i), dims[i]));
    }
    return totalDist;
}

std::list<int>* Torus3DMachine::getFreeAtDistance(int center, int dist) const
{
    std::list<int>* nodeList = new std::list<int>();
    //optimization:
    if(dist < 1 || dist > dims[0] + dims[1] + dims[2]){
        return nodeList;
    }

    std::vector<int> centerDims(3);
    centerDims[0] = coordOf(center,0);
    centerDims[1] = coordOf(center,1);
    centerDims[2] = coordOf(center,2);

    int xMinDelta = (dims[0] - 1) / 2;
    int xMaxDelta = ceil((float) (dims[0] - 1) / 2);
    int yMinDelta = (dims[1] - 1) / 2;
    int yMaxDelta = ceil((float) (dims[1] - 1) / 2);
    int zMinDelta = (dims[2] - 1) / 2;
    int zMaxDelta = ceil((float) (dims[2] - 1) / 2);

    std::vector<int> curDims(3);

    for(int xDist = -std::min(dist, xMinDelta);
            xDist <= std::min(dist, xMaxDelta);
            xDist++){
        curDims[0] = mod(centerDims[0] + xDist, dims[0]);
        int yRange = dist - abs(xDist);
        for(int yDist = -std::min(yRange, yMinDelta);
                yDist <= std::min(yRange, yMaxDelta);
                yDist++){
            curDims[1] = mod(centerDims[1] + yDist, dims[1]);
            int zDist = yRange - abs(yDist);
            if(zDist <= zMinDelta){
                curDims[2] = mod(centerDims[2] - zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
            if(zDist != 0 && zDist <= zMaxDelta){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    return nodeList;
}

std::list<int>* Torus3DMachine::getFreeAtLInfDistance(int center, int dist) const
{
    std::list<int>* nodeList = new std::list<int>();
    if(dist < 1 || dist > dims[0] + dims[1] + dims[2]){
        return nodeList;
    }

    std::vector<int> centerDims(3);
    centerDims[0] = coordOf(center,0);
    centerDims[1] = coordOf(center,1);
    centerDims[2] = coordOf(center,2);

    int xMinDelta = (dims[0] - 1) / 2;
    int xMaxDelta = ceil((float) (dims[0] - 1) / 2);
    int yMinDelta = (dims[1] - 1) / 2;
    int yMaxDelta = ceil((float) (dims[1] - 1) / 2);
    int zMinDelta = (dims[2] - 1) / 2;
    int zMaxDelta = ceil((float) (dims[2] - 1) / 2);

    std::vector<int> curDims(3);

    //first scan the sides

    //go backward in x
    if(dist <= xMinDelta){
        curDims[0] = mod(centerDims[0] - dist, dims[0]);
        //scan all y except edges & corners
        for(int yDist = -std::min((dist - 1), yMinDelta);
                yDist <= std::min((dist - 1), yMaxDelta);
                yDist++){
            curDims[1] = mod(centerDims[1] + yDist, dims[1]);
            //scan all z except edges & corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    //go backward in y
    if(dist <= yMinDelta){
        curDims[1] = mod(centerDims[1] - dist, dims[1]);
        //scan all x except edges & corners
        for(int xDist = -std::min((dist - 1), xMinDelta);
                xDist <= std::min((dist - 1), xMaxDelta);
                xDist++){
            curDims[0] = centerDims[0] + xDist;
            //scan all z except edges & corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    //go backward in z
    if(dist <= zMinDelta){
        curDims[2] = mod(centerDims[2] - dist, dims[2]);
        //scan all x except edges & corners
        for(int xDist = -std::min((dist - 1), xMinDelta);
                xDist <= std::min((dist - 1), xMaxDelta);
                xDist++){
            curDims[0] = centerDims[0] + xDist;
            //scan all y except edges & corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    //go forward in z
    if(dist <= zMaxDelta){
        curDims[2] = mod(centerDims[2] + dist, dims[2]);
        //scan all x except edges & corners
        for(int xDist = -std::min((dist - 1), xMinDelta);
                xDist <= std::min((dist - 1), xMaxDelta);
                xDist++){
            curDims[0] = centerDims[0] + xDist;
            //scan all y except edges & corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    //go forward in y
    if(dist <= yMaxDelta){
        curDims[1] = mod(centerDims[1] + dist, dims[1]);
        //scan all x except edges & corners
        for(int xDist = -std::min((dist - 1), xMinDelta);
                xDist <= std::min((dist - 1), xMaxDelta);
                xDist++){
            curDims[0] = centerDims[0] + xDist;
            //scan all z except edges & corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }
    //go forward in x
    if(dist <= xMaxDelta){
        curDims[0] = mod(centerDims[0] + dist, dims[0]);
        //scan all y except edges & corners
        for(int yDist = -std::min((dist - 1), yMinDelta);
                yDist <= std::min((dist - 1), yMaxDelta);
                yDist++){
            curDims[1] = mod(centerDims[1] + yDist, dims[1]);
            //scan all z except edges & corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //now do edges

    //backward in x
    if(dist <= xMinDelta){
        curDims[0] = mod(centerDims[0] - dist, dims[0]);
        //backward in y
        if(dist <= yMinDelta){
            curDims[1] = mod(centerDims[1] - dist, dims[1]);
            //scan all z except corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
        //backward in z
        if(dist <= zMinDelta){
            curDims[2] = mod(centerDims[2] - dist, dims[2]);
            //scan all y except corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in z
        if(dist <= zMaxDelta){
            curDims[2] = mod(centerDims[2] + dist, dims[2]);
            //scan all y except corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in y
        if(dist <= yMaxDelta){
            curDims[1] = mod(centerDims[1] + dist, dims[1]);
            //scan all z except corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //backward in y
    if(dist <= yMinDelta){
        curDims[1] = mod(centerDims[1] - dist, dims[1]);
        //backward in z
        if(dist <= zMinDelta){
            curDims[2] = mod(centerDims[2] - dist, dims[2]);
            //scan all x except corners
            for(int xDist = -std::min((dist - 1), xMinDelta);
                    xDist <= std::min((dist - 1), xMaxDelta);
                    xDist++){
                curDims[0] = centerDims[0] + xDist;
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in z
        if(dist <= zMaxDelta){
            curDims[2] = mod(centerDims[2] + dist, dims[2]);
            //scan all x except corners
            for(int xDist = -std::min((dist - 1), xMinDelta);
                    xDist <= std::min((dist - 1), xMaxDelta);
                    xDist++){
                curDims[0] = centerDims[0] + xDist;
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //forward in y
    if(dist <= yMaxDelta){
        curDims[1] = mod(centerDims[1] + dist, dims[1]);
        //backward in z
        if(dist <= zMinDelta){
            curDims[2] = mod(centerDims[2] - dist, dims[2]);
            //scan all x except corners
            for(int xDist = -std::min((dist - 1), xMinDelta);
                    xDist <= std::min((dist - 1), xMaxDelta);
                    xDist++){
                curDims[0] = centerDims[0] + xDist;
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in z
        if(dist <= zMaxDelta){
            curDims[2] = mod(centerDims[2] + dist, dims[2]);
            //scan all x except corners
            for(int xDist = -std::min((dist - 1), xMinDelta);
                    xDist <= std::min((dist - 1), xMaxDelta);
                    xDist++){
                curDims[0] = centerDims[0] + xDist;
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //forward in x
    if(dist <= xMaxDelta){
        curDims[0] = mod(centerDims[0] + dist, dims[0]);
        //backward in y
        if(dist <= yMinDelta){
            curDims[1] = mod(centerDims[1] - dist, dims[1]);
            //scan all z except corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
        //backward in z
        if(dist <= zMinDelta){
            curDims[2] = mod(centerDims[2] - dist, dims[2]);
            //scan all y except corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in z
        if(dist <= zMaxDelta){
            curDims[2] = mod(centerDims[2] + dist, dims[2]);
            //scan all y except corners
            for(int yDist = -std::min((dist - 1), yMinDelta);
                    yDist <= std::min((dist - 1), yMaxDelta);
                    yDist++){
                curDims[1] = mod(centerDims[1] + yDist, dims[1]);
                appendIfFree(curDims, nodeList);
            }
        }
        //forward in y
        if(dist <= yMaxDelta){
            curDims[1] = mod(centerDims[1] + dist, dims[1]);
            //scan all z except corners
            for(int zDist = -std::min((dist - 1), zMinDelta);
                    zDist <= std::min((dist - 1), zMaxDelta);
                    zDist++){
                curDims[2] = mod(centerDims[2] + zDist, dims[2]);
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //now do corners

    //backward in x
    if(dist <= xMinDelta){
        curDims[0] = mod(centerDims[0] - dist, dims[0]);
        if(dist <= yMinDelta){
            curDims[1] = mod(centerDims[1] - dist, dims[1]); //backward in y
            if(dist <= zMinDelta){
                curDims[2] = mod(centerDims[2] - dist, dims[2]); //backward in z
                appendIfFree(curDims, nodeList);
            }
            if(dist <= zMaxDelta){
                curDims[2] = mod(centerDims[2] + dist, dims[2]); //forward in z
                appendIfFree(curDims, nodeList);
            }
        }
        if(dist <= yMaxDelta){
            curDims[1] = mod(centerDims[1] + dist, dims[1]); //forward in y
            if(dist <= zMinDelta){
                curDims[2] = mod(centerDims[2] - dist, dims[2]); //backward in z
                appendIfFree(curDims, nodeList);
            }
            if(dist <= zMaxDelta){
                curDims[2] = mod(centerDims[2] + dist, dims[2]); //forward in z
                appendIfFree(curDims, nodeList);
            }
        }
    }

    //forward in x
    if(dist <= xMaxDelta){
        curDims[0] = mod(centerDims[0] + dist, dims[0]);
        if(dist <= yMinDelta){
            curDims[1] = mod(centerDims[1] - dist, dims[1]); //backward in y
            if(dist <= zMinDelta){
                curDims[2] = mod(centerDims[2] - dist, dims[2]); //backward in z
                appendIfFree(curDims, nodeList);
            }
            if(dist <= zMaxDelta){
                curDims[2] = mod(centerDims[2] + dist, dims[2]); //forward in z
                appendIfFree(curDims, nodeList);
            }
        }
        if(dist <= yMaxDelta){
            curDims[1] = mod(centerDims[1] + dist, dims[1]); //forward in y
            if(dist <= zMinDelta){
                curDims[2] = mod(centerDims[2] - dist, dims[2]); //backward in z
                appendIfFree(curDims, nodeList);
            }
            if(dist <= zMaxDelta){
                curDims[2] = mod(centerDims[2] + dist, dims[2]); //forward in z
                appendIfFree(curDims, nodeList);
            }
        }
    }

    return nodeList;
}

int Torus3DMachine::nodesAtDistance(int dist) const
{
    if(dist == 0)
        return 1;
    else
        return 4 * pow(dist, 2) + 2;
}

void Torus3DMachine::appendIfFree(std::vector<int> curDims, std::list<int>* nodeList) const
{
    if(curDims[0] >= 0 && curDims[0] < dims[0] &&
       curDims[1] >= 0 && curDims[1] < dims[1] &&
       curDims[2] >= 0 && curDims[2] < dims[2]){
        int tempNode = indexOf(curDims);
        if(isFree(tempNode)){
            nodeList->push_back(tempNode);
        }
    }
}

int Torus3DMachine::getLinkIndex(std::vector<int> nodeDims, int dimension) const
{
    int x = nodeDims[0];
    int y = nodeDims[1];
    int z = nodeDims[2];
    int linkNo;
    //link order: first all links in x dimension (same ordering with nodes), then y, then z
    switch(dimension){
        case 0:
            if(x == (dims[0] - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            linkNo = x + y * (dims[0] - 1) + z * dims[1] * (dims[0] - 1);
            break;
        case 1:
            if(y == (dims[1] - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            //add all x links
            linkNo = dims[2] * dims[1] * (dims[0] - 1);
            linkNo += x + y * dims[0] + z * dims[0] * (dims[1] - 1);
            break;
        case 2:
            if(z == (dims[2] - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            //add all x and y links
            linkNo = dims[2] * (2 * dims[0] * dims[1] - dims[1] - dims[0]);
            linkNo += x + y * dims[0] + z * dims[0] * dims[1];
            break;
        default:
            schedout.fatal(CALL_INFO, 1, "Link index requested for non-existing dimension.\n");
            linkNo = 0; //avoids compilation warning
    }
    return linkNo;
}

std::list<int>* Torus3DMachine::getRoute(int node0, int node1, double commWeight) const
{
    std::list<int>* links = new std::list<int>();
    int x0 = coordOf(node0,0);
    int x1 = coordOf(node1,0);
    int y0 = coordOf(node0,1);
    int y1 = coordOf(node1,1);
    int z0 = coordOf(node0,2);
    int z1 = coordOf(node1,2);
    //add X route
    std::vector<int> loc(3);
    for(int x = std::min(x0, x1); x < std::max(x0, x1); x++){
        loc[0] = x;
        loc[1] = y0;
        loc[2] = z0;
        links->push_back(getLinkIndex(loc, 0));
    }
    //add Y route
    for(int y = std::min(y0, y1); y < std::max(y0, y1); y++){
        loc[0] = x1;
        loc[1] = y;
        loc[2] = z0;
        links->push_back(getLinkIndex(loc, 1));
    }
    //add Z route
    for(int z = std::min(z0, z1); z < std::max(z0, z1); z++){
        loc[0] = x1;
        loc[1] = y1;
        loc[2] = z;
        links->push_back(getLinkIndex(loc, 2));
    }
    return links;
}
