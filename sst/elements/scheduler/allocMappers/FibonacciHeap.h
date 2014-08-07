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

#ifndef SST_SCHEDULER_FIBONACCI_HEAP_H_
#define SST_SCHEDULER_FIBONACCI_HEAP_H_

#include <string> // debug
#include <vector>

namespace SST {
    namespace Scheduler {

        //Custom Fibonacci heap implementation
        class FibonacciHeap {
            private:
                struct Node {
                    unsigned int ID;
                    double key;
                    unsigned int degree;
                    Node* left;
                    Node* right;
                    Node* parent;
                    std::vector<Node*> child;
                    unsigned int childNo;
                    bool marked;
                };

                Node* minRoot;
                std::vector<Node*> nodesByID;

                void makeRoot(Node *node);
                //extracts node0 and node1 from the root list
                //merges node0 with node1
                //makes merged node a root and returns it
                Node* mergeRoots(Node *node0, Node *node1);

            public:

                FibonacciHeap(unsigned int size);
                ~FibonacciHeap();

                bool isEmpty() { return minRoot == NULL; }
                int findMin();
                void insert(unsigned int nodeID, double key);
                int deleteMin();
                void decreaseKey(unsigned int nodeID, double newKey);
        };
    }
}

#endif /* SST_SCHEDULER_FIBONACCI_HEAP_H_ */
