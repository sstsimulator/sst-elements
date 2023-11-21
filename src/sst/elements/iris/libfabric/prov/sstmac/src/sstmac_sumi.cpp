#include <sstmac_sumi.hpp>
#include <sstmac/software/api/api.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>

FabricTransport* sstmac_fabric()
{
  sstmac::sw::Thread* t = sstmac::sw::OperatingSystem::currentThread();
  FabricTransport* tp = t->getApi<FabricTransport>("libfabric");
  if (!tp->inited())
    tp->init();
  return tp;
}

constexpr uint64_t FabricMessage::no_tag;
constexpr uint64_t FabricMessage::no_imm_data;
