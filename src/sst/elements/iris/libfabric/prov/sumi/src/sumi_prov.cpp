#include <sumi_fabric.hpp>
#include <mercury/operating_system/libraries/api.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>

FabricTransport* sumi_fabric()
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  FabricTransport* tp = t->getApi<FabricTransport>("libfabric");
  if (!tp->inited())
    tp->init();
  return tp;
}

constexpr uint64_t FabricMessage::no_tag;
constexpr uint64_t FabricMessage::no_imm_data;
