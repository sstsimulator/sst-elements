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
                //debug:
                void printNode(Node* node, int shift) const;
                void print() const;

                //needs maximum size
                FibonacciHeap(unsigned int size);
                ~FibonacciHeap();

                bool isEmpty() const { return minRoot == NULL; }
                int findMin() const;
                //ID should be smaller than max size
                void insert(unsigned int nodeID, double key);
                int deleteMin();
                void decreaseKey(unsigned int nodeID, double newKey);
                double getKey(unsigned int nodeID);
        };
    }
}

#endif /* SST_SCHEDULER_FIBONACCI_HEAP_H_ */
