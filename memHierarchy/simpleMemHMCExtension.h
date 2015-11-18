#ifndef SIMPLE_MEM_HMC_EXTENSION_H
#define SIMPLE_MEM_HMC_EXTENSION_H

#include <sst/core/interfaces/simpleMem.h>

namespace SST { namespace MacSim {

class SimpleMemHMCExtension : public Interfaces::SimpleMem {
public:
    class HMCRequest : public Interfaces::SimpleMem::Request {
    public:
        uint8_t hmcInstType;
        uint64_t hmcTransId;
    public:
        /** Constructor */
        HMCRequest(Command cmd, Addr addr, size_t size, dataVec &data, flags_t flags = 0, uint8_t _hmcInstType = 0, uint64_t _hmcTransId=0) :
            Interfaces::SimpleMem::Request(cmd, addr, size, data, flags), hmcInstType(_hmcInstType), hmcTransId(_hmcTransId) { }

        /** Constructor */
        HMCRequest(Command cmd, Addr addr, size_t size, flags_t flags = 0, uint8_t _hmcInstType = 0, uint64_t _hmcTransId=0) :
            Interfaces::SimpleMem::Request(cmd, addr, size, flags), hmcInstType(_hmcInstType), hmcTransId(_hmcTransId) { }
    }; // class HMCRequest
}; // class SimpleMemHMCExtension

} // namespace MacSim
} // namespace SST

#endif // SIMPLE_MEM_HMC_EXTENSION_H
