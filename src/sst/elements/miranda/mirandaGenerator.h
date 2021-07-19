// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEM_H_REQUEST_GEN
#define _H_SST_MEM_H_REQUEST_GEN

#include <stdint.h>
#include <sst/core/subcomponent.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

typedef enum {
	READ,
	WRITE,
	REQ_FENCE,
        CUSTOM,
        OPCOUNT
} ReqOperation;


class GeneratorRequest {
public:
	GeneratorRequest() {
		reqID = nextGeneratorRequestID++;
	}

	virtual ~GeneratorRequest() {}
	virtual ReqOperation getOperation() const = 0;
	uint64_t getRequestID() const { return reqID; }

	void addDependency(uint64_t depReq) {
		dependsOn.push_back(depReq);
	}

	void satisfyDependency(const GeneratorRequest* req) {
		satisfyDependency(req->getRequestID());
	}

	void satisfyDependency(const uint64_t req) {
		std::vector<uint64_t>::iterator searchDeps;

		for(searchDeps = dependsOn.begin(); searchDeps != dependsOn.end(); searchDeps++) {
			if( req == (*searchDeps) ) {
				dependsOn.erase(searchDeps);
				break;
			}
		}
	}

	bool canIssue() {
		return dependsOn.empty();
	}

	uint64_t getIssueTime() const {
		return issueTime;
	}

	void setIssueTime(const uint64_t now) {
		issueTime = now;
	}
protected:
	uint64_t reqID;
	uint64_t issueTime;
	std::vector<uint64_t> dependsOn;
private:
	static std::atomic<uint64_t> nextGeneratorRequestID;
};

template<typename QueueType>
class MirandaRequestQueue {
public:
       	MirandaRequestQueue() {
                        theQ = (QueueType*) malloc(sizeof(QueueType) * 16);
                        maxCapacity = 16;
                        curSize = 0;
                }
        ~MirandaRequestQueue() {
               	free(theQ);
        }

        bool empty() const {
               	return 0 == curSize;
        }

        void resize(const uint32_t newSize) {
//		printf("Resizing MirandaQueue from: %" PRIu32 " to %" PRIu32 "\n",
//			curSize, newSize);

               	QueueType * newQ = (QueueType *) malloc(sizeof(QueueType) * newSize);
               	for(uint32_t i = 0; i < curSize; ++i) {
                       	newQ[i] = theQ[i];
                }

                free(theQ);
               	theQ = newQ;
               	maxCapacity = newSize;
               	curSize = std::min(curSize, newSize);
        }

	uint32_t size() const {
		return curSize;
	}

	uint32_t capacity() const {
		return maxCapacity;
	}

       	QueueType at(const uint32_t index) {
               	return theQ[index];
       	}

       	void erase(const std::vector<uint32_t> eraseList) {
		if(0 == eraseList.size()) {
			return;
		}

               	QueueType* newQ = (QueueType*) malloc(sizeof(QueueType) * maxCapacity);

               	uint32_t nextSkipIndex = 0;
               	uint32_t nextSkip = eraseList.at(nextSkipIndex);
                uint32_t nextNewQIndex = 0;

               	for(uint32_t i = 0; i < curSize; ++i) {
                       	if(nextSkip == i) {
                                nextSkipIndex++;

                                if(nextSkipIndex >= eraseList.size()) {
                                       	nextSkip = curSize;
                               	} else {
                                       	nextSkip = eraseList.at(nextSkipIndex);
                                }
                       	} else {
                               	newQ[nextNewQIndex] = theQ[i];
                                nextNewQIndex++;
                       	}
               	}

                free(theQ);

               	theQ = newQ;
		curSize = nextNewQIndex;
        }

	void push_back(QueueType t) {
                if(curSize == maxCapacity) {
                        resize(maxCapacity + 16);
                }

                theQ[curSize] = t;
                curSize++;
        }
private:
        QueueType* theQ;
        uint32_t maxCapacity;
        uint32_t curSize;
};

class MemoryOpRequest : public GeneratorRequest {
public:
        MemoryOpRequest(const uint64_t cAddr,
		const uint64_t cLength,
		const ReqOperation cOpType) :
		GeneratorRequest(),
		addr(cAddr), length(cLength), op(cOpType) {}
	~MemoryOpRequest() {}
	ReqOperation getOperation() const { return op; }
	bool isRead() const { return op == READ; }
	bool isWrite() const { return op == WRITE; }
        bool isCustom() const { return op == CUSTOM; }
	uint64_t getAddress() const { return addr; }
	uint64_t getLength() const { return length; }

protected:
	uint64_t addr;
	uint64_t length;
	ReqOperation op;
};

class CustomOpRequest : public MemoryOpRequest {
public:
    CustomOpRequest(const uint64_t cAddr,
            const uint64_t cLength,
            const uint32_t cOpcode) :
        MemoryOpRequest(cAddr, cLength, CUSTOM) {
            opcode = cOpcode;
        }
    ~CustomOpRequest() {}
    uint32_t getOpcode() const { return opcode; }

protected:
    uint32_t opcode;
};

class FenceOpRequest : public GeneratorRequest {
public:
	FenceOpRequest() : GeneratorRequest() {}
	~FenceOpRequest() {}
	ReqOperation getOperation() const { return REQ_FENCE; }
};

class RequestGenerator : public SubComponent {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Miranda::RequestGenerator)

	RequestGenerator( ComponentId_t id, Params& params) : SubComponent(id) {}
	~RequestGenerator() {}
	virtual void generate(MirandaRequestQueue<GeneratorRequest*>* q) { }
	virtual bool isFinished() { return true; }
	virtual void completed() { }

};

}
}

#endif
