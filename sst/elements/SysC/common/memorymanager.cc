#include <sst_config.h>
#include "memorymanager.h"

#include <sst/core/output.h>
using namespace SST;
using namespace SST::SysC;

void MemoryManager::free(Payload_t* _trans){
  out->verbose(CALL_INFO,210,0,
              "Deleting %#llX------------\n",_trans->get_address());
  delete _trans;
}
