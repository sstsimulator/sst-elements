#include "pmi.h"
#include <mercury/common/errors.h>
#include <mercury/common/thread_lock.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/process/thread.h>
#include <iris/sumi/message.h>
#include <iris/sumi/transport.h>
#include <cstdio>
#include <cstring>
#include <unordered_map>

static SST::Iris::sumi::Transport* sstmac_pmi()
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  return t->getLibrary<SST::Iris::sumi::Transport>("libfabric");
}

int PMI_Get_rank(int *ret)
{
  *ret = sstmac_pmi()->rank();
  return PMI_SUCCESS;
}

int PMI_Get_size(int* ret)
{
  *ret = sstmac_pmi()->nproc();
  return PMI_SUCCESS;
}

static SST::Hg::thread_lock kvs_lock;
static std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>> kvs;

// Per-thread (per-simulated-rank) flags; shared across ranks in one OS process.
static std::unordered_map<SST::Hg::Thread*, bool> pmi_initialized_map;
static std::unordered_map<SST::Hg::Thread*, bool> pmi_finalized_map;
static std::unordered_map<SST::Hg::Thread*, bool> ibarrier_pending_map;

static bool pmi_flag_get(std::unordered_map<SST::Hg::Thread*, bool>& m)
{
  auto* t = SST::Hg::OperatingSystem::currentThread();
  kvs_lock.lock();
  bool v = m[t];
  kvs_lock.unlock();
  return v;
}

static void pmi_flag_set(std::unordered_map<SST::Hg::Thread*, bool>& m, bool v)
{
  auto* t = SST::Hg::OperatingSystem::currentThread();
  kvs_lock.lock();
  m[t] = v;
  kvs_lock.unlock();
}

static std::string current_jobid_str()
{
  auto* api = sstmac_pmi();
  char buf[64];
  snprintf(buf, sizeof(buf), "app%d", api->sid().app_);
  return std::string(buf);
}

int
PMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
  kvs_lock.lock();
  kvs[kvsname][key] = value;
  kvs_lock.unlock();
  return PMI_SUCCESS;
}

int
PMI_KVS_Get( const char kvsname[], const char key[], char value[], int length)
{
  kvs_lock.lock();
  auto outer = kvs.find(kvsname);
  if (outer == kvs.end()){
    kvs_lock.unlock();
    return PMI_ERR_INVALID_KEY;
  }
  auto inner = outer->second.find(key);
  if (inner == outer->second.end()){
    kvs_lock.unlock();
    return PMI_ERR_INVALID_KEY;
  }
  const std::string& str = inner->second;
  if (length < (int)(str.length() + 1)){
    kvs_lock.unlock();
    return PMI_ERR_INVALID_LENGTH;
  }
  ::strcpy(value, str.c_str());
  kvs_lock.unlock();
  return PMI_SUCCESS;
}

int
PMI2_KVS_Put(const char key[], const char value[])
{
  std::string kvsname = current_jobid_str();
  kvs_lock.lock();
  kvs[kvsname][key] = value;
  kvs_lock.unlock();
  return PMI_SUCCESS;
}

int
PMI2_KVS_Get(const char *jobid, int /*src_pmi_id*/, const char key[],
             char value[], int maxvalue, int *vallen)
{
  std::string kvsname = (jobid && jobid[0]) ? std::string(jobid)
                                            : current_jobid_str();
  kvs_lock.lock();
  auto outer = kvs.find(kvsname);
  if (outer == kvs.end()){
    kvs_lock.unlock();
    if (vallen) *vallen = 0;
    return PMI_ERR_INVALID_KEY;
  }
  auto inner = outer->second.find(key);
  if (inner == outer->second.end()){
    kvs_lock.unlock();
    if (vallen) *vallen = 0;
    return PMI_ERR_INVALID_KEY;
  }
  const std::string& s = inner->second;
  if ((int)(s.size() + 1) > maxvalue){
    kvs_lock.unlock();
    return PMI_ERR_INVALID_LENGTH;
  }
  std::memcpy(value, s.c_str(), s.size() + 1);
  if (vallen) *vallen = (int)s.size();
  kvs_lock.unlock();
  return PMI_SUCCESS;
}

int
PMI2_KVS_Fence(void)
{
  return PMI_SUCCESS;
}

int
PMI2_Abort(void)
{
  SST::Hg::abort("unimplemented: PMI2_Abort");
  __builtin_unreachable();
}

int
PMI2_Job_GetId(char jobid[], int jobid_size)
{
  auto* api = sstmac_pmi();
  ::snprintf(jobid, jobid_size, "app%d", api->sid().app_);
  return PMI_SUCCESS;
}

int
PMI2_Init(int *spawned, int *size, int *rank, int *appnum)
{
  auto api = sstmac_pmi();
  api->init();
  pmi_flag_set(pmi_initialized_map, true);
  *size = api->nproc();
  *rank = api->rank();
  *appnum = 0;
  *spawned = 0;
  return PMI_SUCCESS;
}

int PMI_Abort(int rc, const char error_msg[])
{
  SST::Hg::abort(error_msg ? error_msg : "PMI_Abort called with null message");
  __builtin_unreachable();
}

int PMI_Initialized( PMI_BOOL *initialized )
{
  *initialized = pmi_flag_get(pmi_initialized_map) ? 1 : 0;
  return PMI_SUCCESS;
}

int
PMI2_Finalize()
{
  pmi_flag_set(pmi_finalized_map, true);
  pmi_flag_set(pmi_initialized_map, false);
  auto api = sstmac_pmi();
  api->finish();
  return PMI_SUCCESS;
}

int
PMI_Get_nidlist_ptr(void** nidlist)
{
  *nidlist = sstmac_pmi()->nidlist();
  return PMI_SUCCESS;
}

int PMI_Init(int* spawned)
{
  auto* tport = sstmac_pmi();
  tport->init();
  pmi_flag_set(pmi_initialized_map, true);
  *spawned = 0;

  int nproc = tport->nproc();
  // TODO: multi-node topology. The mapping "(vector,(0,nproc,1))" assumes a
  // single node with all ranks contiguous starting at node 0. Revisit when
  // multi-node runs are exercised so that apps consuming PMI_process_mapping
  // see a PPN/node layout that matches the simulated platform.
  char mapping[64];
  snprintf(mapping, sizeof(mapping), "(vector,(%d,%d,%d))", 0, nproc, 1);
  char kvsname[256];
  snprintf(kvsname, sizeof(kvsname), "app%d", tport->sid().app_);
  kvs_lock.lock();
  kvs[kvsname]["PMI_process_mapping"] = mapping;
  kvs_lock.unlock();

  return PMI_SUCCESS;
}

int PMI_Finalize()
{
  sstmac_pmi()->finish();
  pmi_flag_set(pmi_initialized_map, false);
  pmi_flag_set(pmi_finalized_map, true);
  return PMI_SUCCESS;
}


int PMI_Allgather(void *in, void *out, int len)
{
  auto tport = sstmac_pmi();
  int init_tag = tport->engine()->allocateGlobalCollectiveTag();
  auto* msg = tport->engine()->allgather(out, in, len, 1, init_tag, SST::Iris::sumi::Message::default_cq, nullptr);
  if (msg) delete msg;
  return PMI_SUCCESS;
}

int PMI_Barrier()
{
  auto api = sstmac_pmi();
  int init_tag = api->engine()->allocateGlobalCollectiveTag();
  api->engine()->barrier(init_tag, SST::Iris::sumi::Message::default_cq, nullptr);
  auto* msg = api->engine()->blockUntilNext(SST::Iris::sumi::Message::default_cq);
  if (msg) delete msg;
  return PMI_SUCCESS;
}

int PMI_Ibarrier()
{
  auto api = sstmac_pmi();
  int init_tag = api->engine()->allocateGlobalCollectiveTag();
  api->engine()->barrier(init_tag, SST::Iris::sumi::Message::default_cq, nullptr);
  pmi_flag_set(ibarrier_pending_map, true);
  return PMI_SUCCESS;
}

int PMI_Wait()
{
  if (!pmi_flag_get(ibarrier_pending_map))
    return PMI_SUCCESS;
  auto api = sstmac_pmi();
  auto* msg = api->engine()->blockUntilNext(SST::Iris::sumi::Message::default_cq);
  if (msg) delete msg;
  pmi_flag_set(ibarrier_pending_map, false);
  return PMI_SUCCESS;
}

int
PMI_Get_numpes_on_smp(int* num)
{
  *num = 1;
  return PMI_SUCCESS;
}

int
PMI_Publish_name( const char service_name[], const char port[] )
{
  SST::Hg::abort("unimplemented error: PMI_Publish_name");
  __builtin_unreachable();
}

int
PMI_Unpublish_name( const char service_name[] )
{
  SST::Hg::abort("unimplemented error: PMI_Unpublish_name");
  __builtin_unreachable();
}

int
PMI_KVS_Get_name_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

int
PMI_KVS_Get_key_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

int
PMI_KVS_Get_value_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

int
PMI_KVS_Get_my_name( char kvsname[], int length )
{
  auto* api = sstmac_pmi();
  int actual_length = snprintf(kvsname, length, "app%d", api->sid().app_);
  if (actual_length >= length){
    return PMI_ERR_INVALID_LENGTH;
  } else {
    return PMI_SUCCESS;
  }
}

int
PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizesp[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
  SST::Hg::abort("unimplemented error: PMI_Spawn_multiple");
  __builtin_unreachable();
}

int
PMI_Lookup_name( const char service_name[], char port[] )
{
  SST::Hg::abort("unimplemented error: PMI_Lookup_name");
  __builtin_unreachable();
}

int
PMI_KVS_Commit( const char kvsname[] )
{
  return PMI_SUCCESS;
}

int
PMI_Get_universe_size( int *size )
{
  auto* api = sstmac_pmi();
  *size = api->nproc();
  return PMI_SUCCESS;
}

int
PMI_Get_appnum( int *appnum )
{
  auto* api = sstmac_pmi();
  *appnum = api->sid().app_;
  return PMI_SUCCESS;
}
