
#ifndef _H_ZODIAC_OTF_READER
#define _H_ZODIAC_OTF_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>

#include "otf.h"

using namespace std;

extern "C" {
int handleOTFEnter(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src);
int handleOTFExit(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src);
}

namespace SST {
namespace Zodiac {

class OTFReader {
    public:
	OTFReader(string file, uint32_t rank);
        void close();
	void generateNextEvent();

    private:
	OTF_Reader* reader;
	OTF_FileManager* fileMgr;
	OTF_HandlerArray* handlers;
	uint32_t rank;

};

}
}

#endif
