
#ifndef _H_ZODIAC_DUMPI_READER
#define _H_ZODIAC_DUMPI_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>

#include "dumpifunc.h"

using namespace std;

namespace SST {
namespace Zodiac {


class DUMPIReader {
    public:
	DUMPIReader(string file);
        void close();
	DUMPIFunction readNextFunction();

    private:
        FILE* trace;
        void readTrace(uint16_t*);
	void readTrace(uint32_t*);
	void readTrace(uint64_t*);

	void readArrayTrace(uint32_t*, int);
	void readArrayTrace(uint16_t*, int);
	void readArrayTrace(char**, uint32_t*);
};

}
}

#endif
