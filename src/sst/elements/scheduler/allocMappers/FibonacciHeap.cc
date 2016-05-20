// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "FibonacciHeap.h"

#include "output.h"

using namespace SST::Scheduler;

FibonacciHeap::FibonacciHeap(unsigned int size)
{
    nodesByID.resize(size, NULL);
    minRoot = NULL;
}

FibonacciHeap::~FibonacciHeap()
{

}

int FibonacciHeap::findMin()
{
    if(minRoot == NULL){
        schedout.fatal(CALL_INFO, 1, "Tried to find minimum in an empty Fibonacci heap");
    }
    return minRoot->ID;
}

void FibonacciHeap::insert(unsigned int nodeID, double key)
{
    Node* node = new Node();
    node->ID = nodeID;
    node->key = key;
    node->marked = false;
    node->degree = 0;
    nodesByID[nodeID] = node;
    makeRoot(node);
}

int FibonacciHeap::deleteMin()
{
    if(minRoot == NULL){
        schedout.fatal(CALL_INFO, 1, "Tried to delete minimum in an empty Fibonacci heap");
    }

    int retVal;

    //make children roots
    for(unsigned int i = 0; i < minRoot->child.size(); i++){
        if(minRoot->child[i] != NULL){
            makeRoot(minRoot->child[i]);
        }
    }
    minRoot->child.clear();

    //consolidate same-degree roots
    Node* newMin;
    Node* curRoot = minRoot->right;
    Node* nextRoot;
    //check if minRoot is the only node
    if(curRoot == minRoot){
        newMin = NULL;
    } else {
        //traverse linked list starting from minRoot
        Node* dummy = new Node;
        std::vector<Node*> rootListByDegree(0);
        rootListByDegree.push_back(dummy);
        newMin = curRoot;
        while(curRoot != minRoot){
            nextRoot = curRoot->right;
            //make sure that rootListByDegree is large enough
            while(rootListByDegree.back() != dummy || rootListByDegree.size() < curRoot->degree + 1){
                rootListByDegree.push_back(dummy);
            }
            //merge curRoot if necessary
            while(rootListByDegree[curRoot->degree] != dummy){
                Node* toMerge = rootListByDegree[curRoot->degree];
                rootListByDegree[curRoot->degree] = dummy;
                curRoot = mergeRoots(curRoot, toMerge);
            }
            rootListByDegree[curRoot->degree] = curRoot;
            //update new min
            if(curRoot->key <= newMin->key){
                newMin = curRoot;
            }
            //iterate
            curRoot = nextRoot;
        }
        //extract & delete minRoot
        minRoot->left->right = minRoot->right;
        minRoot->right->left = minRoot->left;
        delete dummy;
    }
    retVal = minRoot->ID;
    nodesByID[minRoot->ID] = NULL;
    delete minRoot;
    minRoot = newMin;

    return retVal;
}

void FibonacciHeap::makeRoot(Node *node)
{
    if(node->parent != NULL){
        node->parent->child[node->childNo] = NULL;
        node->parent = NULL;
    }
    node->marked = false;
    if(minRoot == NULL){
        minRoot = node;
        minRoot->left = node;
        minRoot->right = node;
    } else {
        node->left = minRoot;
        node->right = minRoot->right;
        minRoot->right->left = node;
        minRoot->right = node;
        if(node->key < minRoot->key){
            minRoot = node;
        }
    }
}

FibonacciHeap::Node* FibonacciHeap::mergeRoots(Node *node0, Node *node1)
{
    //find smaller
    Node* nodes[2]; //0: smaller, 1: larger
    if(node0->key <= node1->key){
        nodes[0] = node0;
        nodes[1] = node1;
    } else {
        nodes[1] = node0;
        nodes[0] = node1;
    }
    //extract node0 and node1 from root list
    node0->left->right = node0->right;
    node0->right->left = node0->left;
    node1->left->right = node1->right;
    node1->right->left = node1->left;
    //merge
    nodes[1]->parent = nodes[0];
    nodes[1]->childNo = nodes[0]->child.size();
    nodes[0]->child.push_back(nodes[1]);
    nodes[0]->degree++;
    //make root
    makeRoot(nodes[0]);

    return nodes[0];
}

void FibonacciHeap::decreaseKey(unsigned int nodeID, double newKey)
{
    Node* node = nodesByID[nodeID];
    node->key = newKey;
    Node* parent = node->parent;
    Node* child = node;
    if(parent != NULL && parent->key > node->key){
        makeRoot(child);
        while(parent != NULL && parent->marked){
            child = parent;
            parent = child->parent;
        }
        if(parent != NULL){
            parent->marked = true;
        }
    }
    if(newKey < minRoot->key){
        minRoot = node;
    }
}

double FibonacciHeap::getKey(unsigned int nodeID)
{
    if(nodesByID[nodeID] == NULL){
        schedout.fatal(CALL_INFO, 1, "Tried to get key of non-existing node.");
    }
    return nodesByID[nodeID]->key;
}
