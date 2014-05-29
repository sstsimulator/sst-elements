//
//  instructionStream.h
//  SST
//
//  Created by Caesar De la Paz III on 5/29/14.
//  Copyright (c) 2014 De la Paz, Cesar. All rights reserved.
//

#ifndef __SST__instructionStream__
#define __SST__instructionStream__

#include <sst/core/serialization/element.h>
#include "sst/core/output.h"
#include <iostream>

#include "cacheController.h"
#include "util.h"

#endif /* defined(__SST__instructionStream__) */

namespace SST { namespace MemHierarchy {

using namespace std;


class CacheCPU{

public:
    typedef uint64_t time;
    typedef string   instruction;
    typedef queue<time, instruction> instructionStream;
    
    CacheCPU(const Cache* _cache, string _filename);
    startInstructionStream();

private:

    string filename_;
    instructionStream instStream_;

    void executeInstruction(string _instruction);
    void decodeInstruction(string _instruction);
    void parseOpcode(string _instruction);
    void modifyCacheLine(string _operands);
    void issueGetM(string _operands);
    void issueGetS(string _operands);
    void issueGetSEx(string _operands);
    void setReplacementTimestamps(string _operands);
    
    void parseInstructionStream();
    

};



}}