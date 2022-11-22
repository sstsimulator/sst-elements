
#include <sst/core/interfaces/stdMem.h>

#include "inst/vinst.h"
#include "inst/vload.h"
#include "inst/vstore.h"
#include "inst/vfence.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

enum class VanadisBasicLoadStoreEntryOp {
    LOAD,
    STORE,
    FENCE
};

class VanadisBasicLoadStoreEntry {
public:
    VanadisBasicLoadStoreEntry(VanadisInstruction* the_ins) : ins(the_ins) {}
    virtual ~VanadisBasicLoadStoreEntry() {}
    virtual VanadisBasicLoadStoreEntryOp getEntryOp() = 0;
    virtual VanadisInstruction* getInstruction() { return ins; }

    virtual bool isInstructionIssued() const { return ins->completedIssue(); }
    virtual bool isinstructionExecuted() const { return ins->completedExecution(); }

    uint32_t getHWThread() const { return ins->getHWThread(); }
    uint64_t getInstructionAddress() const { return ins->getInstructionAddress(); }
protected:
    VanadisInstruction* ins;
};

class VanadisBasicFenceEntry : public VanadisBasicLoadStoreEntry {
public:
    VanadisBasicFenceEntry(VanadisFenceInstruction* fence_ins) : VanadisBasicLoadStoreEntry(fence_ins) {}

    VanadisBasicLoadStoreEntryOp getEntryOp() override {
        return VanadisBasicLoadStoreEntryOp::FENCE;
    }

    VanadisFenceInstruction* getStoreInstruction() {
        return (VanadisFenceInstruction*) ins;
    }
};

class VanadisBasicStoreEntry : public VanadisBasicLoadStoreEntry {
public:
    VanadisBasicStoreEntry(VanadisStoreInstruction* store_ins) : VanadisBasicLoadStoreEntry(store_ins) {}

    VanadisBasicLoadStoreEntryOp getEntryOp() override {
        return VanadisBasicLoadStoreEntryOp::STORE;
    }

    VanadisStoreInstruction* getStoreInstruction() {
        return (VanadisStoreInstruction*) ins;
    }
};

class VanadisBasicStorePendingEntry : public VanadisBasicStoreEntry {
public:
    VanadisBasicStorePendingEntry(VanadisStoreInstruction* store_ins, uint64_t addr, uint64_t width, 
        VanadisStoreRegisterType valRegType, uint16_t valReg) : 
        VanadisBasicStoreEntry(store_ins), storeAddress(addr), storeWidth(width),
        valueRegister(valReg), valueRegisterType(valRegType), dispatched(false) {}

    ~VanadisBasicStorePendingEntry() {
        requests.clear();
    }

    bool     isDispatched() const { return dispatched; }
    void     markDispatched() { dispatched = true; }

    uint64_t getStoreAddress() const { return storeAddress; }
    uint64_t getStoreWidth() const { return storeWidth; }
    uint16_t getValueRegister() const { return valueRegister; }

    size_t   countRequests() const { return requests.size(); }
    void     addRequest(StandardMem::Request::id_t req) { requests.push_back(req); }
    void     removeRequest(StandardMem::Request::id_t req) {
        for(auto req_itr = requests.begin(); req_itr != requests.end(); ) {
            if( (*req_itr) == req ) {
                requests.erase(req_itr);
                break;
            } else {
                req_itr++;
            }
        }
    }
    bool     containsRequest(StandardMem::Request::id_t req) {
        bool found = false;

        for(auto req_itr = requests.begin(); req_itr != requests.end(); req_itr++) {
            if((*req_itr) == req) {
                found = true; 
                break;
            }
        }

        return found;
    }

    bool    storeAddressOverlaps(const uint64_t loadAddress, const uint64_t loadWidth) const {
        bool overlaps = false;

        // Address Overlaps
        // An address overlaps with the store IF:
        // 1. the address being checked is within the range of the store being performed (overlaps fully/right side)
        // 2. the address + width being checked is within the range of the store being performed (overlaps left side)

        const auto store_end = storeAddress + storeWidth;
        const auto load_end  = loadAddress + loadWidth;

        /*
            There are five main cases to capture:
                (S = start of store)
                (L = start of load)

            Case 1: is overlap of load and store
              S-------|
              L-------|

            Case 2: load starts below a store and overlaps a small part at the beginning
                  S-------|
              L-------|

            Case 3: load starts within the region of the store and goes outside the range
               S-------|
                   L--------|

            Case 4: load is larger and covers the store
                  S--|
               L-------|

            Case 5: load is smaller and is covered by the store
               S--------|
                   L---|    
        */

        // Load address is between the start and end of the store
        // captures cases 1, 3 and 5
        overlaps = ((loadAddress >= storeAddress) & (loadAddress < (store_end)));

        // Load address is less than the start of the store but its end is within the store range
        // captures cases 1, 2 and 4
        overlaps = overlaps | ((loadAddress <= storeAddress) & (load_end >= storeAddress));

        return overlaps;
    }

    VanadisStoreRegisterType getValueRegisterType() const {
        return valueRegisterType;
    }

protected:
    std::vector<StandardMem::Request::id_t> requests;
    const uint64_t storeAddress;
    const uint64_t storeWidth;
    const uint16_t valueRegister;

    bool dispatched;
    const VanadisStoreRegisterType valueRegisterType;
};

class VanadisBasicLoadEntry : public VanadisBasicLoadStoreEntry {
public:
    VanadisBasicLoadEntry(VanadisLoadInstruction* load_ins) : VanadisBasicLoadStoreEntry(load_ins) {}

    VanadisBasicLoadStoreEntryOp getEntryOp() override {
        return VanadisBasicLoadStoreEntryOp::LOAD;
    }
  
    VanadisLoadInstruction* getLoadInstruction() { 
        return (VanadisLoadInstruction*) ins;
    }
};

class VanadisBasicLoadPendingEntry : public VanadisBasicLoadEntry {
public:
    VanadisBasicLoadPendingEntry(VanadisLoadInstruction* load_ins, uint64_t address, uint64_t width) : 
        VanadisBasicLoadEntry(load_ins), load_address(address), load_width(width) {}
    
    ~VanadisBasicLoadPendingEntry() {
        requests.clear();
    }

    void addRequest(StandardMem::Request::id_t req) {
        requests.push_back(req);
    }

    bool containsRequest(StandardMem::Request::id_t req) {
        bool found = false;

        for(auto req_itr = requests.begin(); req_itr != requests.end(); req_itr++) {
            if( req == (*req_itr) ) {
                found = true; 
                break;
            }
        }

        return found;
    }

    void removeRequest(StandardMem::Request::id_t req) {
        for(auto req_itr = requests.begin(); req_itr != requests.end(); req_itr++) {
            if((*req_itr) == req) {
                requests.erase(req_itr);
                break;
            }
        }
    }

    uint64_t getLoadAddress() const {
        return load_address;
    }

    uint64_t getLoadWidth() const {
        return load_width;
    }

    size_t countRequests() const {
        return requests.size();
    }

    // identify what the req order is for this entry
    // in split-cache line loads we need to restore data into the register
    // in the correct order
    int identifySequence(StandardMem::Request::id_t req) const {
        int index = -1;

        for(int i = 0; i < requests.size(); ++i) {
            if(requests[i] == req) {
                index = i;
                break;
            }
        }

        return index;
    }
protected:
    std::vector<StandardMem::Request::id_t> requests;
    const uint64_t load_address;
    const uint64_t load_width;
};

}
}