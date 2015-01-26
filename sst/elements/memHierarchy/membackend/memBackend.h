
#ifndef __SST_MEMH_BACKEND__
#define __SST_MEMH_BACKEND__

#include <sst/core/event.h>
#include <sst/core/output.h>

#include <iostream>
#include <map>

#include "memoryController.h"

#define NO_STRING_DEFINED "N/A"

namespace SST {
namespace MemHierarchy {

class MemController;

class MemBackend : public Module {
public:
    MemBackend();
    MemBackend(Component *comp, Params &params);
    virtual ~MemBackend();
    virtual bool issueRequest(MemController::DRAMReq *req) = 0;
    virtual void setup() {}
    virtual void finish() {}
    virtual void clock() {}
protected:
    MemController *ctrl;
    Output* output;

};



}}

#endif

