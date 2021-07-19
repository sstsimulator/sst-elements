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


#ifndef _H_ZODIAC_OTF_READER
#define _H_ZODIAC_OTF_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <queue>

#include "sst/elements/hermes/msgapi.h"
#include "zevent.h"
#include "zsendevent.h"
#include "zrecvevent.h"
#include "otf.h"

using namespace std;
using namespace SST::Hermes;

extern "C" {
int handleOTFDefineProcess(void *userData, uint32_t stream, uint32_t process, const char *name, uint32_t parent);
int handleOTFEnter(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src);
int handleOTFExit(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src);
int handleOTFCollectiveOperation(void *userData, uint64_t time, uint32_t process, uint32_t collective, uint32_t procGroup, uint32_t rootProc, uint32_t sent,
       	uint32_t received, uint64_t duration, uint32_t source, OTF_KeyValueList *list);
int handleOTFRecvMsg(void *userData, uint64_t time, uint32_t recvProc, uint32_t sendProc, uint32_t group, uint32_t type, uint32_t length,
	uint32_t source, OTF_KeyValueList *list);
int handleOTFSendMsg(void *userData, uint64_t time, uint32_t sender, uint32_t receiver, uint32_t group, uint32_t type, uint32_t length,
	uint32_t source, OTF_KeyValueList *list);
int handleOTFBeginCollective(void *userData, uint64_t time, uint32_t process, uint32_t collOp, uint64_t matchingId, uint32_t procGroup,
        uint32_t rootProc, uint64_t sent, uint64_t received, uint32_t scltoken, OTF_KeyValueList *list);
int handleOTFEndCollective(void *userData, uint64_t time, uint32_t process, uint64_t matchingId, OTF_KeyValueList *list);

}

namespace SST {
namespace Zodiac {

class OTFReader {
    public:
	OTFReader(string file, uint32_t rank, uint32_t qLimit, std::queue<ZodiacEvent*>* eventQueue);
        void close();
	uint32_t generateNextEvents();
	uint32_t getQueueLimit();
	uint32_t getCurrentQueueSize();
	void enqueueEvent(ZodiacEvent* ev);

    private:
	OTF_Reader* reader;
	OTF_FileManager* fileMgr;
	OTF_HandlerArray* handlers;
	uint32_t rank;
	uint32_t qLimit;
	bool foundFinalize;
	std::queue<ZodiacEvent*>* eventQ;

};

}
}

#endif
