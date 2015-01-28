#ifndef COMMON_MEMORYMANAGER_H
#define COMMON_MEMORYMANAGER_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "tlm.h"

#include <sst/core/sst_types.h>

namespace SST{
class Output;
namespace SysC{
class MemoryManager : public tlm::tlm_mm_interface{
 private:
  typedef SST::SysC::MemoryManager      ThisType;
  typedef tlm::tlm_mm_interface         BaseType;
 protected:
  typedef tlm::tlm_generic_payload     Payload_t;
 public:
  MemoryManager(Output *_out):out(_out){}
  void free(Payload_t* _trans);
 private:
  Output *out;
};
}//namespace SysC
}//namespace SST

#endif//COMMON_MEMORYMANAGER_H
