// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain
// rights in this software.
//
// Copyright (c) 2009-2025, NTESS. All rights reserved.
//
// Simulated pthread implementation for SST mercury (hg).
// Ported from sst-macro pthread.

#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/common/errors.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/operating_system/libraries/pthread/hg_pthread_runner.h>
#include <mercury/operating_system/libraries/pthread/hg_pthread_impl.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/operating_system/process/mutex.h>
#include <mercury/common/timestamp.h>
#include <mercury/operating_system/process/thread_id.h>

#include <errno.h>
#include <cstddef>
#include <map>
#include <vector>
#include <mutex>
#include <utility>

extern "C" int sst_hg_global_stacksize;

namespace {

SST::Hg::Thread* currentThread()
{
  return SST::Hg::Thread::current();
}

/* Per-thread cleanup handler stack for pthread_cleanup_push/pop */
using cleanup_entry_t = std::pair<void (*)(void*), void*>;
static std::mutex cleanup_mutex;
static std::map<SST::Hg::Thread*, std::vector<cleanup_entry_t>> cleanup_stacks;

/* Barrier state for pthread_barrier_* */
struct BarrierState {
  SST::Hg::App* app;
  int mutex_id;
  int cond_id;
  int count;
  unsigned int init_count;
};
static std::mutex barrier_map_mutex;
static std::map<int, BarrierState> barrier_map;
static int next_barrier_id = 1;

int check_mutex(hg_pthread_mutex_t* mutex)
{
  if (*mutex == HG_PTHREAD_MUTEX_INITIALIZER) {
    return HG_pthread_mutex_init(mutex, nullptr);
  }
  return 0;
}

int check_cond(hg_pthread_cond_t* cond)
{
  if (*cond == HG_PTHREAD_COND_INITIALIZER) {
    return HG_pthread_cond_init(cond, nullptr);
  }
  return 0;
}

} // namespace

extern "C" {

int
HG_pthread_create(hg_pthread_t* pthread,
                  const hg_pthread_attr_t* attr,
                  void* (*start_routine)(void*), void* arg)
{
  SST::Hg::Thread* thr = currentThread();
  SST::Hg::OperatingSystemAPI* os = thr->os();
  SST::Hg::App* parent_app = thr->parentApp();

  SST::Hg::ThreadId unknown_thrid(-1);
  SST::Hg::SoftwareId newid(parent_app->aid(), parent_app->tid(), unknown_thrid);
  SST::Hg::HgPthreadRunner* tr = new SST::Hg::HgPthreadRunner(
      newid, parent_app, start_routine, arg, os,
      attr ? attr->detach_state : HG_PTHREAD_CREATE_JOINABLE);

  parent_app->addSubthread(tr);
  *pthread = static_cast<hg_pthread_t>(tr->threadId());

  if (attr) {
    tr->setCpumask(attr->cpumask);
  }

  os->startThread(tr);
  return 0;
}

int
HG_pthread_yield(void)
{
  return 0;
}

void
HG_pthread_exit(void* /*retval*/)
{
  SST::Hg::Thread* current = currentThread();
  /* Run cleanup handlers in LIFO order before exit */
  std::vector<cleanup_entry_t> to_run;
  {
    std::lock_guard<std::mutex> guard(cleanup_mutex);
    auto it = cleanup_stacks.find(current);
    if (it != cleanup_stacks.end()) {
      to_run = std::move(it->second);
      cleanup_stacks.erase(it);
    }
  }
  for (auto it = to_run.rbegin(); it != to_run.rend(); ++it) {
    if (it->first) it->first(it->second);
  }
  current->kill();
}

int
HG_pthread_kill(hg_pthread_t /*thread*/, int /*sig*/)
{
  sst_hg_abort_printf("pthread_kill: not implemented");
  return -1;
}

int
HG_pthread_join(hg_pthread_t pthread, void** /*status*/)
{
  SST::Hg::Thread* current_thr = currentThread();
  SST::Hg::OperatingSystemAPI* os = current_thr->os();
  SST::Hg::App* parent_app = current_thr->parentApp();
  SST::Hg::Thread* joiner = parent_app->getSubthread(static_cast<uint32_t>(pthread));

  if (joiner) {
    os->joinThread(joiner);
  }

  parent_app->removeSubthread(static_cast<uint32_t>(pthread));
  return 0;
}

int
HG_pthread_testcancel(void)
{
  sst_hg_abort_printf("pthread_testcancel: not implemented");
  return 0;
}

hg_pthread_t
HG_pthread_self(void)
{
  SST::Hg::Thread* t = currentThread();
  return static_cast<hg_pthread_t>(t->threadId());
}

int
HG_pthread_equal(hg_pthread_t t1, hg_pthread_t t2)
{
  return (t1 == t2) ? 1 : 0;
}

int
HG_pthread_mutexattr_gettype(const hg_pthread_mutexattr_t* /*attr*/, int* type)
{
  *type = HG_PTHREAD_MUTEX_NORMAL;
  return 0;
}

int
HG_pthread_mutexattr_settype(hg_pthread_mutexattr_t* /*attr*/, int type)
{
  if (type == HG_PTHREAD_MUTEX_NORMAL) {
    return 0;
  }
  return EINVAL;
}

int
HG_pthread_spin_init(hg_pthread_spinlock_t* lock, int /*pshared*/)
{
  return HG_pthread_mutex_init(lock, nullptr);
}

int
HG_pthread_mutex_init(hg_pthread_mutex_t* mutex,
                      const hg_pthread_mutexattr_t* /*attr*/)
{
  SST::Hg::Thread* thr = currentThread();
  SST::Hg::App* parent_app = thr->parentApp();
  *mutex = parent_app->allocateMutex();
  return 0;
}

int
HG_pthread_spin_destroy(hg_pthread_spinlock_t* lock)
{
  return HG_pthread_mutex_destroy(lock);
}

int
HG_pthread_mutex_destroy(hg_pthread_mutex_t* mutex)
{
  if (*mutex == HG_PTHREAD_MUTEX_INITIALIZER) {
    return 0;
  }
  SST::Hg::Thread* thr = currentThread();
  SST::Hg::App* a = thr->parentApp();
  bool found = a->eraseMutex(*mutex);
  return found ? 0 : EINVAL;
}

int
HG_pthread_spin_lock(hg_pthread_spinlock_t* lock)
{
  return HG_pthread_mutex_lock(lock);
}

int
HG_pthread_mutex_lock(hg_pthread_mutex_t* mutex)
{
  int rc;
  if ((rc = check_mutex(mutex)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::mutex_t* mut = thr->parentApp()->getMutex(*mutex);
  if (mut == nullptr) {
    return EINVAL;
  }
  if (mut->locked) {
    mut->waiters.push_back(thr);
    thr->os()->block();
  } else {
    mut->locked = true;
  }
  return 0;
}

int
HG_pthread_spin_trylock(hg_pthread_spinlock_t* lock)
{
  return HG_pthread_mutex_trylock(lock);
}

int
HG_pthread_mutex_trylock(hg_pthread_mutex_t* mutex)
{
  int rc;
  if ((rc = check_mutex(mutex)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::mutex_t* mut = thr->parentApp()->getMutex(*mutex);
  if (mut == nullptr) {
    return EINVAL;
  }
  if (mut->locked) {
    return 1; /* EBUSY-style */
  }
  mut->locked = true;
  return 0;
}

int
HG_pthread_spin_unlock(hg_pthread_spinlock_t* lock)
{
  return HG_pthread_mutex_unlock(lock);
}

int
HG_pthread_mutex_unlock(hg_pthread_mutex_t* mutex)
{
  int rc;
  if ((rc = check_mutex(mutex)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::mutex_t* mut = thr->parentApp()->getMutex(*mutex);
  if (mut == nullptr || !mut->locked) {
    return EINVAL;
  }
  if (!mut->waiters.empty()) {
    SST::Hg::Thread* blocker = mut->waiters.front();
    mut->waiters.pop_front();
    thr->os()->sendExecutionEventNow(
        new SST::Hg::UnblockEvent(thr->os(), blocker));
  } else {
    mut->locked = false;
  }
  return 0;
}

int
HG_pthread_mutexattr_init(hg_pthread_mutexattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_mutexattr_destroy(hg_pthread_mutexattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_cond_init(hg_pthread_cond_t* cond,
                     const hg_pthread_condattr_t* /*attr*/)
{
  SST::Hg::Thread* thr = currentThread();
  *cond = thr->parentApp()->allocateCondition();
  return 0;
}

int
HG_pthread_cond_destroy(hg_pthread_cond_t* cond)
{
  if (*cond == HG_PTHREAD_COND_INITIALIZER) {
    return 0;
  }
  SST::Hg::Thread* thr = currentThread();
  thr->parentApp()->eraseCondition(*cond);
  return 0;
}

int
HG_pthread_cond_wait(hg_pthread_cond_t* cond, hg_pthread_mutex_t* mutex)
{
  return HG_pthread_cond_timedwait(cond, mutex, nullptr);
}

int
HG_pthread_cond_timedwait(hg_pthread_cond_t* cond,
                          hg_pthread_mutex_t* mutex,
                          const struct timespec* abstime)
{
  int rc;
  if ((rc = check_cond(cond)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::condition_t* pending = thr->parentApp()->getCondition(*cond);
  if (pending == nullptr) {
    return EINVAL;
  }

  SST::Hg::mutex_t* mut = thr->parentApp()->getMutex(*mutex);
  if (mut == nullptr) {
    return EINVAL;
  }

  SST::Hg::OperatingSystemAPI* myos = thr->os();
  (*pending)[*mutex] = mut;
  if (!mut->waiters.empty()) {
    SST::Hg::Thread* blocker = mut->waiters.front();
    mut->waiters.pop_front();
    myos->sendExecutionEventNow(new SST::Hg::UnblockEvent(myos, blocker));
  } else {
    mut->locked = false;
  }
  mut->conditionals.push_back(thr);

  if (abstime) {
    double secs = static_cast<double>(abstime->tv_sec) +
                  1e-9 * static_cast<double>(abstime->tv_nsec);
    SST::Hg::TimeDelta delay(secs);
    myos->blockTimeout(delay);
  } else {
    myos->block();
  }

  if (abstime && thr->timedOut()) {
    auto end = mut->conditionals.end();
    for (auto it = mut->conditionals.begin(); it != end; ++it) {
      if (*it == thr) {
        mut->conditionals.erase(it);
        break;
      }
    }
  }
  return 0;
}

int
HG_pthread_cond_signal(hg_pthread_cond_t* cond)
{
  int rc;
  if ((rc = check_cond(cond)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::condition_t* pending = thr->parentApp()->getCondition(*cond);
  if (pending == nullptr) {
    return EINVAL;
  }
  SST::Hg::OperatingSystemAPI* myos = thr->os();
  for (auto& it : *pending) {
    SST::Hg::mutex_t* mut = it.second;
    if (!mut->conditionals.empty()) {
      SST::Hg::Thread* w = mut->conditionals.front();
      mut->conditionals.pop_front();
      mut->locked = true;
      myos->sendExecutionEventNow(new SST::Hg::UnblockEvent(myos, w));
      break; /* signal one */
    }
  }
  return 0;
}

int
HG_pthread_cond_broadcast(hg_pthread_cond_t* cond)
{
  int rc;
  if ((rc = check_cond(cond)) != 0) {
    return rc;
  }

  SST::Hg::Thread* thr = currentThread();
  SST::Hg::condition_t* pending = thr->parentApp()->getCondition(*cond);
  if (pending == nullptr) {
    return EINVAL;
  }
  SST::Hg::OperatingSystemAPI* myos = thr->os();
  for (auto& it : *pending) {
    SST::Hg::mutex_t* mut = it.second;
    while (!mut->conditionals.empty()) {
      SST::Hg::Thread* w = mut->conditionals.front();
      mut->conditionals.pop_front();
      mut->locked = true;
      myos->sendExecutionEventNow(new SST::Hg::UnblockEvent(myos, w));
    }
  }
  return 0;
}

int
HG_pthread_condattr_getpshared(const hg_pthread_condattr_t* attr, int* pshared)
{
  *pshared = *attr;
  return 0;
}

int
HG_pthread_condattr_setpshared(hg_pthread_condattr_t* attr, int pshared)
{
  *attr = pshared;
  return 0;
}

int
HG_pthread_once(hg_pthread_once_t* once_init, void (*init_routine)(void))
{
  if (*once_init == 0) {
    *once_init = 1;
    (*init_routine)();
  }
  return 0;
}

int
HG_pthread_key_create(hg_pthread_key_t* key, void (*dest_routine)(void*))
{
  SST::Hg::App* current = currentThread()->parentApp();
  *key = current->allocateTlsKey(dest_routine);
  return 0;
}

int
HG_pthread_key_delete(hg_pthread_key_t /*key*/)
{
  return 0;
}

int
HG_pthread_setspecific(hg_pthread_key_t key, const void* pointer)
{
  SST::Hg::Thread* current = currentThread();
  current->setTlsValue(key, const_cast<void*>(pointer));
  return 0;
}

void*
HG_pthread_getspecific(hg_pthread_key_t key)
{
  SST::Hg::Thread* current = currentThread();
  if (current == nullptr) {
    return nullptr;
  }
  return current->getTlsValue(key);
}

void
HG_pthread_cleanup_pop(int execute)
{
  SST::Hg::Thread* thr = currentThread();
  if (thr == nullptr) return;
  cleanup_entry_t entry(nullptr, nullptr);
  {
    std::lock_guard<std::mutex> guard(cleanup_mutex);
    auto it = cleanup_stacks.find(thr);
    if (it == cleanup_stacks.end() || it->second.empty()) {
      return;
    }
    entry = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) {
      cleanup_stacks.erase(it);
    }
  }
  if (execute && entry.first) {
    entry.first(entry.second);
  }
}

void
HG_pthread_cleanup_push(void (*routine)(void*), void* arg)
{
  SST::Hg::Thread* thr = currentThread();
  if (thr == nullptr) return;
  std::lock_guard<std::mutex> guard(cleanup_mutex);
  cleanup_stacks[thr].emplace_back(routine, arg);
}

int
HG_pthread_condattr_init(hg_pthread_condattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_condattr_destroy(hg_pthread_condattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_attr_init(hg_pthread_attr_t* attr)
{
  attr->cpumask = 0;
  attr->cpumask = ~(attr->cpumask);
  attr->detach_state = HG_PTHREAD_CREATE_JOINABLE;
  return 0;
}

int
HG_pthread_attr_destroy(hg_pthread_attr_t* /*attr*/)
{
  return 0;
}

/* cpu_set type for affinity - same layout as sstmac for compatibility */
typedef struct { uint64_t cpubits; } hg_cpu_set_t;

int
HG_pthread_attr_setaffinity_np(hg_pthread_attr_t* attr, size_t /*cpusetsize*/,
                              const void* cpuset)
{
  if (cpuset) {
    attr->cpumask = static_cast<const hg_cpu_set_t*>(cpuset)->cpubits;
  }
  return 0;
}

int
HG_pthread_attr_getaffinity_np(hg_pthread_attr_t attr, size_t /*cpusetsize*/,
                               void* cpuset)
{
  if (cpuset) {
    static_cast<hg_cpu_set_t*>(cpuset)->cpubits = attr.cpumask;
  }
  return 0;
}

int
HG_pthread_attr_getstack(hg_pthread_attr_t* /*attr*/, void** stack, size_t* stacksize)
{
  *stack = nullptr;
  *stacksize = (sst_hg_global_stacksize > 0) ?
               static_cast<size_t>(sst_hg_global_stacksize) : 16384;
  return 0;
}

int
HG_pthread_attr_getdetachstate(const hg_pthread_attr_t* attr, int* state)
{
  *state = attr->detach_state;
  return 0;
}

int
HG_pthread_attr_setdetachstate(hg_pthread_attr_t* attr, int state)
{
  attr->detach_state = state;
  return 0;
}

int
HG_pthread_attr_setscope(hg_pthread_attr_t* /*attr*/, int scope)
{
  if (scope == HG_PTHREAD_SCOPE_PROCESS) {
    return ENOTSUP;
  }
  return 0;
}

int
HG_pthread_attr_getscope(hg_pthread_attr_t* /*attr*/, int* scope)
{
  *scope = HG_PTHREAD_SCOPE_SYSTEM;
  return 0;
}

int
HG_pthread_detach(hg_pthread_t thr)
{
  SST::Hg::App* parent_app = currentThread()->parentApp();
  SST::Hg::Thread* joiner = parent_app->getSubthread(static_cast<uint32_t>(thr));
  if (joiner) {
    joiner->setDetachState(SST::Hg::Thread::DETACHED);
  }
  return 0;
}

int
HG_pthread_rwlock_rdlock(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_tryrdlock(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_wrlock(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_trywrlock(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_unlock(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_destroy(hg_pthread_rwlock_t* /*rwlock*/) { return 0; }
int
HG_pthread_rwlock_init(hg_pthread_rwlock_t* /*rwlock*/,
                       const hg_pthread_rwlockattr_t* /*attr*/) { return 0; }
int
HG_pthread_rwlockattr_init(hg_pthread_rwlockattr_t* /*attr*/) { return 0; }
int
HG_pthread_rwlockattr_destroy(hg_pthread_rwlockattr_t* /*attr*/) { return 0; }

int
HG_pthread_barrier_init(hg_pthread_barrier_t* barrier,
                        const hg_pthread_barrierattr_t* /*attr*/,
                        unsigned int count)
{
  if (barrier == nullptr || count == 0) {
    return EINVAL;
  }
  SST::Hg::App* app = currentThread()->parentApp();
  int mutex_id = app->allocateMutex();
  int cond_id = app->allocateCondition();
  std::lock_guard<std::mutex> guard(barrier_map_mutex);
  int id = next_barrier_id++;
  barrier_map[id] = BarrierState{app, mutex_id, cond_id, 0, count};
  *barrier = id;
  return 0;
}

int
HG_pthread_barrier_destroy(hg_pthread_barrier_t* barrier)
{
  if (barrier == nullptr) {
    return EINVAL;
  }
  BarrierState st;
  {
    std::lock_guard<std::mutex> guard(barrier_map_mutex);
    auto it = barrier_map.find(*barrier);
    if (it == barrier_map.end()) {
      return EINVAL;
    }
    st = it->second;
    barrier_map.erase(it);
  }
  /* Wait for any thread in barrier_wait to finish by acquiring the mutex */
  hg_pthread_mutex_t mid = st.mutex_id;
  HG_pthread_mutex_lock(&mid);
  HG_pthread_mutex_unlock(&mid);
  st.app->eraseMutex(st.mutex_id);
  st.app->eraseCondition(st.cond_id);
  return 0;
}

int
HG_pthread_barrier_wait(hg_pthread_barrier_t* barrier)
{
  if (barrier == nullptr) {
    return EINVAL;
  }
  int mutex_id;
  int cond_id;
  int c;
  unsigned int init;
  {
    std::lock_guard<std::mutex> guard(barrier_map_mutex);
    auto it = barrier_map.find(*barrier);
    if (it == barrier_map.end()) {
      return EINVAL;
    }
    BarrierState& st = it->second;
    mutex_id = st.mutex_id;
    cond_id = st.cond_id;
    st.count++;
    c = st.count;
    init = st.init_count;
  }
  hg_pthread_mutex_t mid = mutex_id;
  HG_pthread_mutex_lock(&mid);
  if (c < (int)init) {
    HG_pthread_cond_wait(&cond_id, &mid);
    HG_pthread_mutex_unlock(&mid);
    return 0;
  }
  /* Last thread: reset count and broadcast */
  {
    std::lock_guard<std::mutex> guard(barrier_map_mutex);
    auto it = barrier_map.find(*barrier);
    if (it != barrier_map.end()) {
      it->second.count = 0;
    }
  }
  HG_pthread_cond_broadcast(&cond_id);
  HG_pthread_mutex_unlock(&mid);
  return HG_PTHREAD_BARRIER_SERIAL_THREAD;
}

int
HG_pthread_barrierattr_init(hg_pthread_barrierattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_barrierattr_destroy(hg_pthread_barrierattr_t* /*attr*/)
{
  return 0;
}

int
HG_pthread_barrierattr_getpshared(const hg_pthread_barrierattr_t* attr,
                                   int* pshared)
{
  *pshared = *attr;
  return 0;
}

int
HG_pthread_barrierattr_setpshared(hg_pthread_barrierattr_t* attr, int pshared)
{
  *attr = pshared;
  return 0;
}

int
HG_pthread_setconcurrency(int level)
{
  currentThread()->setPthreadConcurrency(level);
  return 0;
}

int
HG_pthread_getconcurrency(void)
{
  return currentThread()->pthreadConcurrency();
}

int
HG_pthread_atfork(void (*/*prepare*/)(void), void (*/*parent*/)(void),
                 void (*/*child*/)(void))
{
  sst_hg_abort_printf("pthread_atfork: not implemented");
  return 0;
}

int
HG_pthread_mutexattr_getpshared(const hg_pthread_mutexattr_t* attr, int* pshared)
{
  *pshared = *attr;
  return 0;
}

int
HG_pthread_mutexattr_setpshared(hg_pthread_mutexattr_t* attr, int pshared)
{
  *attr = pshared;
  return 0;
}

int
HG_pthread_setaffinity_np(hg_pthread_t thread, size_t /*cpusetsize*/,
                          const void* cpuset)
{
  SST::Hg::Thread* t = currentThread()->parentApp()->getSubthread(
      static_cast<uint32_t>(thread));
  if (t && cpuset) {
    t->setCpumask(static_cast<const hg_cpu_set_t*>(cpuset)->cpubits);
    return 0;
  }
  return EINVAL;
}

int
HG_pthread_getaffinity_np(hg_pthread_t thread, size_t /*cpusetsize*/,
                           void* cpuset)
{
  SST::Hg::Thread* t = currentThread()->parentApp()->getSubthread(
      static_cast<uint32_t>(thread));
  if (t && cpuset) {
    static_cast<hg_cpu_set_t*>(cpuset)->cpubits = t->cpumask();
    return 0;
  }
  return EINVAL;
}

} // extern "C"
