// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Simulated pthread types and constants for SST mercury (hg).
// Ported from sst-macro pthread for use when building MPI apps for simulator.

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>

/* Opaque handles (ints) to avoid conflicts with system pthread types */
typedef int hg_pthread_t;
typedef int hg_pthread_cond_t;
typedef int hg_pthread_mutex_t;
typedef int hg_pthread_spinlock_t;
typedef int hg_pthread_once_t;
typedef int hg_pthread_condattr_t;
typedef int hg_pthread_mutexattr_t;
typedef int hg_pthread_rwlock_t;
typedef int hg_pthread_rwlockattr_t;
typedef int hg_pthread_barrier_t;
typedef int hg_pthread_barrierattr_t;

typedef int hg_pthread_key_t;
typedef void (*hg_pthread_key_destructor_fxn)(void*);

#define HG_PTHREAD_ONCE_INIT 0
#define HG_PTHREAD_MUTEX_INITIALIZER -1
#define HG_PTHREAD_COND_INITIALIZER -1
#define HG_PTHREAD_RWLOCK_INITIALIZER -1

#define HG_PTHREAD_THREADS_MAX 1000
#define HG_PTHREAD_KEYS_MAX 10000
#define HG_PTHREAD_STACK_MIN 16384
#define HG_PTHREAD_CREATE_DETACHED 0
#define HG_PTHREAD_CREATE_JOINABLE 1

#define HG_PTHREAD_MUTEX_NORMAL     0
#define HG_PTHREAD_MUTEX_ERRORCHECK 1
#define HG_PTHREAD_MUTEX_RECURSIVE  2
#define HG_PTHREAD_MUTEX_DEFAULT 3
#define HG_PTHREAD_MUTEX_ERRORCHECK_NP 4

#define HG_PTHREAD_PROCESS_SHARED 0
#define HG_PTHREAD_PROCESS_PRIVATE 1
#define HG_PTHREAD_BARRIER_SERIAL_THREAD (-1)

enum {
  HG_PTHREAD_SCOPE_PROCESS,
  HG_PTHREAD_SCOPE_SYSTEM
};

typedef struct {
  uint64_t cpumask;
  int detach_state;
} hg_pthread_attr_t;

#ifdef __cplusplus
extern "C" {
#endif

int HG_pthread_create(hg_pthread_t* thread,
                      const hg_pthread_attr_t* attr,
                      void* (*start_routine)(void*), void* arg);
void HG_pthread_exit(void* retval);
int HG_pthread_join(hg_pthread_t thread, void** status);
hg_pthread_t HG_pthread_self(void);
int HG_pthread_equal(hg_pthread_t t1, hg_pthread_t t2);

int HG_pthread_mutex_init(hg_pthread_mutex_t* mutex,
                           const hg_pthread_mutexattr_t* attr);
int HG_pthread_mutex_destroy(hg_pthread_mutex_t* mutex);
int HG_pthread_mutex_lock(hg_pthread_mutex_t* mutex);
int HG_pthread_mutex_trylock(hg_pthread_mutex_t* mutex);
int HG_pthread_mutex_unlock(hg_pthread_mutex_t* mutex);

int HG_pthread_mutexattr_init(hg_pthread_mutexattr_t* attr);
int HG_pthread_mutexattr_destroy(hg_pthread_mutexattr_t* attr);
int HG_pthread_mutexattr_gettype(const hg_pthread_mutexattr_t* attr, int* type);
int HG_pthread_mutexattr_settype(hg_pthread_mutexattr_t* attr, int type);
int HG_pthread_mutexattr_getpshared(const hg_pthread_mutexattr_t* attr, int* pshared);
int HG_pthread_mutexattr_setpshared(hg_pthread_mutexattr_t* attr, int pshared);

int HG_pthread_spin_init(hg_pthread_spinlock_t* lock, int pshared);
int HG_pthread_spin_destroy(hg_pthread_spinlock_t* lock);
int HG_pthread_spin_lock(hg_pthread_spinlock_t* lock);
int HG_pthread_spin_trylock(hg_pthread_spinlock_t* lock);
int HG_pthread_spin_unlock(hg_pthread_spinlock_t* lock);

int HG_pthread_cond_init(hg_pthread_cond_t* cond,
                          const hg_pthread_condattr_t* attr);
int HG_pthread_cond_destroy(hg_pthread_cond_t* cond);
int HG_pthread_cond_wait(hg_pthread_cond_t* cond, hg_pthread_mutex_t* mutex);
int HG_pthread_cond_timedwait(hg_pthread_cond_t* cond, hg_pthread_mutex_t* mutex,
                              const struct timespec* abstime);
int HG_pthread_cond_signal(hg_pthread_cond_t* cond);
int HG_pthread_cond_broadcast(hg_pthread_cond_t* cond);

int HG_pthread_condattr_init(hg_pthread_condattr_t* attr);
int HG_pthread_condattr_destroy(hg_pthread_condattr_t* attr);
int HG_pthread_condattr_getpshared(const hg_pthread_condattr_t* attr, int* pshared);
int HG_pthread_condattr_setpshared(hg_pthread_condattr_t* attr, int pshared);

int HG_pthread_once(hg_pthread_once_t* once_init, void (*init_routine)(void));

int HG_pthread_key_create(hg_pthread_key_t* key, void (*dest_routine)(void*));
int HG_pthread_key_delete(hg_pthread_key_t key);
int HG_pthread_setspecific(hg_pthread_key_t key, const void* pointer);
void* HG_pthread_getspecific(hg_pthread_key_t key);

int HG_pthread_attr_init(hg_pthread_attr_t* attr);
int HG_pthread_attr_destroy(hg_pthread_attr_t* attr);
int HG_pthread_attr_getdetachstate(const hg_pthread_attr_t* attr, int* state);
int HG_pthread_attr_setdetachstate(hg_pthread_attr_t* attr, int state);
int HG_pthread_attr_setscope(hg_pthread_attr_t* attr, int scope);
int HG_pthread_attr_getscope(hg_pthread_attr_t* attr, int* scope);
int HG_pthread_attr_setaffinity_np(hg_pthread_attr_t* attr, size_t cpusetsize,
                                    const void* cpuset);
int HG_pthread_attr_getaffinity_np(hg_pthread_attr_t attr, size_t cpusetsize,
                                   void* cpuset);
int HG_pthread_attr_getstack(hg_pthread_attr_t* attr, void** stack, size_t* stacksize);

int HG_pthread_detach(hg_pthread_t thread);
int HG_pthread_yield(void);
int HG_pthread_testcancel(void);
int HG_pthread_kill(hg_pthread_t thread, int sig);

int HG_pthread_rwlock_init(hg_pthread_rwlock_t* rwlock,
                           const hg_pthread_rwlockattr_t* attr);
int HG_pthread_rwlock_destroy(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlock_rdlock(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlock_tryrdlock(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlock_wrlock(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlock_trywrlock(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlock_unlock(hg_pthread_rwlock_t* rwlock);
int HG_pthread_rwlockattr_init(hg_pthread_rwlockattr_t* attr);
int HG_pthread_rwlockattr_destroy(hg_pthread_rwlockattr_t* attr);

int HG_pthread_barrier_init(hg_pthread_barrier_t* barrier,
                             const hg_pthread_barrierattr_t* attr,
                             unsigned int count);
int HG_pthread_barrier_destroy(hg_pthread_barrier_t* barrier);
int HG_pthread_barrier_wait(hg_pthread_barrier_t* barrier);
int HG_pthread_barrierattr_init(hg_pthread_barrierattr_t* attr);
int HG_pthread_barrierattr_destroy(hg_pthread_barrierattr_t* attr);
int HG_pthread_barrierattr_getpshared(const hg_pthread_barrierattr_t* attr,
                                       int* pshared);
int HG_pthread_barrierattr_setpshared(hg_pthread_barrierattr_t* attr,
                                       int pshared);

int HG_pthread_setconcurrency(int level);
int HG_pthread_getconcurrency(void);
int HG_pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
int HG_pthread_setaffinity_np(hg_pthread_t thread, size_t cpusetsize, const void* cpuset);
int HG_pthread_getaffinity_np(hg_pthread_t thread, size_t cpusetsize, void* cpuset);

void HG_pthread_cleanup_push(void (*routine)(void*), void* arg);
void HG_pthread_cleanup_pop(int execute);

#ifdef __cplusplus
}
#endif
