
#ifndef _H_HERMES_MESSAGE_INTERFACE
#define _H_HERMES_MESSAGE_INTERFACE

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/module.h>

using namespace SST;

namespace SST {
namespace Hermes {

typedef uint32_t HermesCommunicator;
typedef uint32_t HermesRankID;

typedef struct MessageResponse {
};

class MessageInterface : public Module {
    public:
    enum PayloadDataType {
        CHAR,
	INT,
	LONG,
	DOUBLE,
	FLOAT,
	COMPLEX
    };

    enum ReductionOperation {
	SUM,
	MIN,
	MAX
    };

    MessageInterface() {}
    virtual ~MessageInterface() {}

    virtual void init() {}
    virtual void rank(HermesCommunicator group, int* rank) {}
    virtual void size(HermesCommunicator group) {}
    virtual void send(void* payload, uint32_t count, PayloadDataType dtype, HermesRankID dest, uint32_t tag, HermesCommunicator group) {}
    virtual void recv(void* target, uint32_t count, PayloadDataType dtype, HermesRankID source, uint32_t tag, HermesCommunicator group, MessageResponse& resp) {}

    virtual void allreduce(void* mydata, void* result, uint32_t count, PayloadDataType dtype, ReductionOperation op, HermesCommunicator group) {}
    virtual void reduce(void* mydata, void* result, uint32_t count, PayloadDataType dtype, ReductionOperation op, HermesRankID root, HermesCommunicator group) {}
    virtual void barrier(HermesCommunicator group) {}
};

}
}

#endif

