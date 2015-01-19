
#ifndef _H_SST_MEMH_SIMPLE_MEM_BACKEND
#define _H_SST_MEMH_SIMPLE_MEM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemory : public MemBackend {
public:
    SimpleMemory();
    SimpleMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
private:
    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(MemController::DRAMReq* req) : SST::Event(), req(req)
        { }

        MemController::DRAMReq *req;
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
            ar & BOOST_SERIALIZATION_NVP(req);
        }
    };

    void handleSelfEvent(SST::Event *event);

    Link *self_link;
};

}
}

#endif
