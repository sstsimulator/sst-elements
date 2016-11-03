// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Machine based on a mesh structure
 */

#include "Mesh3DMachine.h"
#include "output.h"

#include <sstream>
#include <cmath>

using namespace SST::Scheduler;

Mesh3DMachine::Mesh3DMachine(std::vector<int> dims, int numCoresPerNode, double** D_matrix)
                         : StencilMachine(dims,
                                   3*dims[0]*dims[1]*dims[2] - dims[0]*dims[1] - dims[0]*dims[2] - dims[1]*dims[2],
                                   numCoresPerNode,
                                   D_matrix)
{
    schedout.init("", 8, 0, Output::STDOUT);
}

std::string Mesh3DMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) com="# ";
    else com="";
    std::stringstream ret;
    ret << com;
    for(unsigned int i = 0; i < (dims.size() - 1) ; i++){
       ret << dims[i] << "x";
    }
    ret << dims.back() << " Mesh";
    ret << ", " << coresPerNode << " cores per node";
    return ret.str();
}

int Mesh3DMachine::getNodeDistance(int node1, int node2) const
{
    MeshLocation loc1 = MeshLocation(node1, *this);
    MeshLocation loc2 = MeshLocation(node2, *this);
    return loc1.L1DistanceTo(loc2);
}

std::list<int>* Mesh3DMachine::getFreeAtDistance(int center, int dist) const
{
    int centerX = coordOf(center,0);
    int centerY = coordOf(center,1);
    int centerZ = coordOf(center,2);
    std::list<int>* nodeList = new std::list<int>();
    std::vector<int> curDims(3);
    //optimization:
    if(dist < 1 || dist > dims[0] + dims[1] + dims[2]){
        return nodeList;
    }

    for(curDims[0] = std::max(centerX - dist, 0); curDims[0] <= std::min(centerX + dist, dims[0] - 1); curDims[0]++){
        int yRange = dist - abs(centerX - curDims[0]);
        for(curDims[1] = std::max(centerY - yRange, 0); curDims[1] <= std::min(centerY + yRange, dims[1] - 1); curDims[1]++){
            int zRange = yRange - abs(centerY - curDims[1]);
            curDims[2] = centerZ - zRange;
            appendIfFree(curDims, nodeList);
            if(zRange != 0){
                curDims[2] = centerZ + zRange;
                appendIfFree(curDims, nodeList);
            }
        }
    }
    return nodeList;
}

std::list<int>* Mesh3DMachine::getFreeAtLInfDistance(int center, int dist) const
{
    const int centerX = coordOf(center,0);
    const int centerY = coordOf(center,1);
    const int centerZ = coordOf(center,2);
    std::vector<int> curDims(3);
    std::list<int>* nodeList = new std::list<int>();
    if(dist < 1 || dist > dims[0] + dims[1] + dims[2]){
        return nodeList;
    }

    //first scan the sides

    //go backward in x
    curDims[0] = centerX - dist;
    //scan all y except edges & corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            curDims[2] = centerZ + zDist;
            appendIfFree(curDims, nodeList);
        }
    }
    //go backward in y
    curDims[1] = centerY - dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            curDims[2] = centerZ + zDist;
            appendIfFree(curDims, nodeList);
        }
    }
    //go backward in z
    curDims[2] = centerZ - dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        //scan all y except edges & corners
        for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
            curDims[1] = centerY + yDist;
            appendIfFree(curDims, nodeList);
        }
    }
    //go forward in z
    curDims[2] = centerZ + dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        //scan all y except edges & corners
        for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
            curDims[1] = centerY + yDist;
            appendIfFree(curDims, nodeList);
        }
    }
    //go forward in y
    curDims[1] = centerY + dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            curDims[2] = centerZ + zDist;
            appendIfFree(curDims, nodeList);
        }
    }
    //go forward in x
    curDims[0] = centerX + dist;
    //scan all y except edges & corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            curDims[2] = centerZ + zDist;
            appendIfFree(curDims, nodeList);
        }
    }

    //now do edges

    //backward in x
    curDims[0] = centerX - dist;
    //backward in y
    curDims[1] = centerY - dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        curDims[2] = centerZ + zDist;
        appendIfFree(curDims, nodeList);
    }
    //backward in z
    curDims[2] = centerZ - dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in z
    curDims[2] = centerZ + dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in y
    curDims[1] = centerY + dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        curDims[2] = centerZ + zDist;
        appendIfFree(curDims, nodeList);
    }

    //backward in y
    curDims[1] = centerY - dist;
    //backward in z
    curDims[2] = centerZ - dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in z
    curDims[2] = centerZ + dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in y
    curDims[1] = centerY + dist;
    //backward in z
    curDims[2] = centerZ - dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in z
    curDims[2] = centerZ + dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        curDims[0] = centerX + xDist;
        appendIfFree(curDims, nodeList);
    }

    //forward in x
    curDims[0] = centerX + dist;
    //backward in y
    curDims[1] = centerY - dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        curDims[2] = centerZ + zDist;
        appendIfFree(curDims, nodeList);
    }
    //backward in z
    curDims[2] = centerZ - dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in z
    curDims[2] = centerZ + dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        curDims[1] = centerY + yDist;
        appendIfFree(curDims, nodeList);
    }
    //forward in y
    curDims[1] = centerY + dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        curDims[2] = centerZ + zDist;
        appendIfFree(curDims, nodeList);
    }

    //now do corners

    //backward in x
    curDims[0] = centerX - dist;
    curDims[1] = centerY - dist; //backward in y
    curDims[2] = centerZ - dist; //backward in z
    appendIfFree(curDims, nodeList);
    curDims[2] = centerZ + dist; //forward in z
    appendIfFree(curDims, nodeList);
    curDims[1] = centerY + dist; //forward in y
    curDims[2] = centerZ - dist; //backward in z
    appendIfFree(curDims, nodeList);
    curDims[2] = centerZ + dist; //forward in z
    appendIfFree(curDims, nodeList);

    //forward in x
    curDims[0] = centerX + dist;
    curDims[1] = centerY - dist; //backward in y
    curDims[2] = centerZ - dist; //backward in z
    appendIfFree(curDims, nodeList);
    curDims[2] = centerZ + dist; //forward in z
    appendIfFree(curDims, nodeList);
    curDims[1] = centerY + dist; //forward in y
    curDims[2] = centerZ - dist; //backward in z
    appendIfFree(curDims, nodeList);
    curDims[2] = centerZ + dist; //forward in z
    appendIfFree(curDims, nodeList);

    return nodeList;
}

int Mesh3DMachine::nodesAtDistance(int dist) const
{
    if(dist == 0)
        return 1;
    else
        return 4 * pow(dist, 2) + 2;
}

void Mesh3DMachine::appendIfFree(std::vector<int> curDims, std::list<int>* nodeList) const
{
    if(curDims[0] >= 0 && curDims[0] < dims[0] && curDims[1] >= 0 && curDims[1] < dims[1] && curDims[2] >= 0 && curDims[2] < dims[2]){
        int tempNode = indexOf(curDims);
        if(isFree(tempNode)){
            nodeList->push_back(tempNode);
        }
    }
}

int Mesh3DMachine::getLinkIndex(std::vector<int> nodeDims, int dimension) const
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

std::vector<int>* Mesh3DMachine::getRoute(int node0, int node1, double commWeight) const
{
    std::vector<int>* links = new std::vector<int>();
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
