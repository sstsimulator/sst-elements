
#ifndef _H_SST_MEM_HIERARCHY_DRAMREQ
#define _H_SST_MEM_HIERARCHY_DRAMREQ

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/core/serialization/serializable.h"

namespace SST {
namespace MemHierarchy {

    /**
	Creates a request to DRAM memory backends
    */
    struct DRAMReq : public SST::Core::Serialization::serializable {
        enum Status_t {NEW, PROCESSING, RETURNED, DONE};

        MemEvent*   reqEvent_;
        MemEvent*   respEvent_;
        int         respSize_;
        Command     cmd_;
        uint64_t      size_;
        uint64_t      amtInProcess_;
        uint64_t      amtProcessed_;
        Status_t    status_;
        Addr        baseAddr_;
        Addr        addr_;
        bool        isWrite_;

        DRAMReq(MemEvent *ev, const uint64_t busWidth, const uint64_t cacheLineSize) :
            reqEvent_(new MemEvent(*ev)), respEvent_(NULL), amtInProcess_(0),
            amtProcessed_(0), status_(NEW){

            cmd_      = ev->getCmd();
            isWrite_  = (cmd_ == PutM) ? true : false;
            baseAddr_ = ev->getBaseAddr();
            addr_     = ev->getBaseAddr();
            setSize(cacheLineSize);
        }

        DRAMReq(const uint64_t baseAddress, Command reqCmd, const uint64_t busWidth, const uint64_t cacheLineSize) :
	    reqEvent_(NULL), respEvent_(NULL), amtInProcess_(0),
            amtProcessed_(0), status_(NEW) {

	    addr_ = baseAddress;
            baseAddr_ = baseAddress;
	    cmd_ = reqCmd;
            isWrite_  = (cmd_ == PutM) ? true : false;
            setSize(cacheLineSize);
        }

        ~DRAMReq() { delete reqEvent_; }

        void setSize(uint64_t _size){ size_ = _size; }
        void setAddr(Addr _baseAddr){ baseAddr_ = _baseAddr; }

    
        void serialize_order(SST::Core::Serialization::serializer &ser) {
            ser & reqEvent_;
            ser & respEvent_;
            ser & respSize_;
            ser & cmd_;
            ser & size_;
            ser & amtInProcess_;
            ser & amtProcessed_;
            ser & status_;
            ser & baseAddr_;
            ser & addr_;
            ser & isWrite_;
            
        }
    private:
        DRAMReq() {}
        
        ImplementSerializable(SST::MemHierarchy::DRAMReq)

    };

}
}

#endif
