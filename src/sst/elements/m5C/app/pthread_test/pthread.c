/*
    m5threads, a pthread library for the M5 simulator
    Copyright (C) 2009, Stanford University

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Author: Daniel Sanchez
*/

#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <sys/syscall.h>

//Spinlock assembly
#if defined(__x86) || defined(__x86_64)
  #include "spinlock_x86.h"
#elif defined(__alpha)
  #include "spinlock_alpha.h"
#elif defined(__sparc)
  #include "spinlock_sparc.h"
#else
  #error "spinlock routines not available for your arch!\n"
#endif

#include "pthread_defs.h"
#include "tls_defs.h"
#include "profiling_hooks.h"

#define restrict 

//64KB stack, change to your taste...
#define CHILD_STACK_BITS 16
#define CHILD_STACK_SIZE (1 << CHILD_STACK_BITS)

//#define __DEBUG
//Debug macro
#ifdef __DEBUG
  #define DEBUG(args...) printf(args)
#else
  #define DEBUG(args...) 
#endif

//Size and alignment requirements of "real" (NPTL/LinuxThreads) thread control block
#define TCB_SIZE 512
#define TCB_ALIGN sizeof(double)
//TODO: Figure out real (NPTL/LinuxThreads) TCB space. 512 bytes should be enough.

//Thread control structure
typedef struct {
  pthread_t tid;
  unsigned int is_detached; //0 if joinable, 1 if detached
  volatile int child_finished;
  void* result; //written by child on exit
  void *(*start_routine)(void*);
  void* arg;
  //thread block limits
  void* tls_start_addr;
  void* stack_start_addr;
} pthread_tcb_t;


//Information about the thread block (TLS, sizes)
static struct {
  size_t tls_memsz;
  size_t tls_filesz;
  void*  tls_initimage;
  size_t tls_align;
  size_t total_size;
  size_t stack_guard_size;
} thread_block_info;


/* Thread-local data */

//Pointer to our TCB (NULL for main thread)
__thread pthread_tcb_t* __tcb;

// Used for TSD (getspecific, setspecific, etc.)
__thread void** pthread_specifics = NULL; //dynamically allocated, since this is rarely used
__thread uint32_t pthread_specifics_size = 0;


/* Initialization, create/exit/join functions */

// Search ELF segments, pull out TLS block info, campute thread block sizes
static void populate_thread_block_info() {
  ElfW(Phdr) *phdr;

  //If there is no TLS segment...
  thread_block_info.tls_memsz = 0;
  thread_block_info.tls_filesz = 0;
  thread_block_info.tls_initimage = NULL;
  thread_block_info.tls_align = 0;

  /* Look through the TLS segment if there is any.  */
  if (_dl_phdr != NULL) {
    for (phdr = _dl_phdr; phdr < &_dl_phdr[_dl_phnum]; ++phdr) {
      if (phdr->p_type == PT_TLS) {
          /* Gather the values we need.  */
          thread_block_info.tls_memsz = phdr->p_memsz;
          thread_block_info.tls_filesz = phdr->p_filesz;
          thread_block_info.tls_initimage = (void *) phdr->p_vaddr;
          thread_block_info.tls_align = phdr->p_align;
          break;
      }
    }
  }

  //Set a stack guard size
  //In SPARC, this is actually needed to avoid out-of-range accesses on register saves...
  //Largest I have seen is 2048 (sparc64)
  //You could avoid this in theory by compiling with -mnostack-bias
  thread_block_info.stack_guard_size = 2048;

  //Total thread block size -- this is what we'll request to mmap
  size_t sz = sizeof(pthread_tcb_t) + thread_block_info.tls_memsz + TCB_SIZE + thread_block_info.stack_guard_size + CHILD_STACK_SIZE;
  //Note that TCB_SIZE is the "real" TCB size, not ours, which we leave zeroed (but some variables, notably errno, are somewhere inside there)

  //Align to multiple of CHILD_STACK_SIZE
  sz += CHILD_STACK_SIZE - 1;  
  thread_block_info.total_size = (sz>>CHILD_STACK_BITS)<<CHILD_STACK_BITS;

}


//Set up TLS block in current thread
static void setup_thread_tls(void* th_block_addr) {
  /* Compute the (real) TCB offset */
  size_t tcb_offset = roundup(thread_block_info.tls_memsz, TCB_ALIGN);
  /* Align the TLS block.  */
  void* tlsblock = (void *) (((uintptr_t) th_block_addr + thread_block_info.tls_align - 1)
                       & ~(thread_block_info.tls_align - 1));
  /* Initialize the TLS block.  */
  char* tls_start_ptr = ((char *) tlsblock + tcb_offset
                           - roundup (thread_block_info.tls_memsz, thread_block_info.tls_align ?: 1));

  //DEBUG("Init TLS: Copying %d bytes from 0x%llx to 0x%llx\n", filesz, (uint64_t) initimage, (uint64_t) tls_start_ptr);
  memcpy (tls_start_ptr, thread_block_info.tls_initimage, thread_block_info.tls_filesz);

  //Rest of tls vars are already cleared (mmap returns zeroed memory)

  //Note: We don't care about DTV pointers for x86/SPARC -- they're never used in static mode
  /* Initialize the thread pointer.  */
  TLS_INIT_TP ((char *) tlsblock + tcb_offset, 0);
}

//Some NPTL definitions
int __libc_multiple_threads; //set to one on initialization
int __nptl_nthreads = 32; //TODO: we don't really know...

//Called at initialization. Sets up TLS for the main thread and populates thread_block_info, used in subsequent calls
//Works with LinuxThreads and NPTL
void __pthread_initialize_minimal() {
  __libc_multiple_threads = 1; //tell libc we're multithreaded (NPTL-specific)
  populate_thread_block_info();
  void* ptr = mmap(0, thread_block_info.total_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  setup_thread_tls(ptr);
}


//Used by pthread_create to spawn child
static int __pthread_trampoline(void* thr_ctrl) {
  //Set TLS up
  
  pthread_tcb_t* tcb = (pthread_tcb_t*) thr_ctrl; 
  setup_thread_tls(tcb->tls_start_addr);
  __tcb = tcb;
  DEBUG("Child in trampoline, TID=%llx\n", tcb->tid);

  void* result = tcb->start_routine(tcb->arg);
  pthread_exit(result);
  assert(0); //should never be reached
}

int pthread_create (pthread_t* thread,
                    const pthread_attr_t* attr,
                    void *(*start_routine)(void*), 
                    void* arg) {
  DEBUG("pthread_create: start\n");

  //Allocate the child thread block (TCB+TLS+stack area)
  //We use mmap so that the child can munmap it at exit without using a stack (it's a system call)
  void* thread_block;
  size_t thread_block_size = thread_block_info.total_size;
  thread_block = mmap(0, thread_block_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  DEBUG("pthread_create: mmapped child thread block 0x%llx -- 0x%llx\n", thread_block, ((char*)thread_block) + CHILD_STACK_SIZE) ;
 
  //Populate the thread control block
  pthread_tcb_t* tcb = (pthread_tcb_t*) thread_block;
  tcb->tid = (pthread_t) thread_block; //thread ID is tcb address itself
  tcb->is_detached = 0; //joinable
  tcb->child_finished = 0;
  tcb->start_routine = start_routine;
  tcb->arg = arg;
  tcb->tls_start_addr = (void*)(((char*)thread_block) + sizeof(pthread_tcb_t)); //right after tcb
  tcb->stack_start_addr = (void*) (((char*) thread_block) + thread_block_size - thread_block_info.stack_guard_size); //end of thread_block
  
  *thread=(pthread_t) thread_block;

  //Call clone()
  DEBUG("pthread_create: prior to clone()\n");
  clone(__pthread_trampoline, tcb->stack_start_addr, CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD, tcb);
  DEBUG("pthread_create: after clone()\n");
  return 0;
}

pthread_t pthread_self() {
    if (__tcb == NULL) return 0; //main thread
    return __tcb->tid;
}

int pthread_join (pthread_t thread, void** status) {
    DEBUG("pthread_join: started\n");
    pthread_tcb_t* child_tcb = (pthread_tcb_t*) thread;
    assert(child_tcb->tid == thread); // checks that this is really a tcb
    assert(!child_tcb->is_detached); // thread should be joinable
    volatile int child_done = 0;
    while (child_done == 0) { // spin until child done
        child_done = child_tcb->child_finished;
    }
    DEBUG("pthread_join: child joined\n");
    //Get result
    if (status) *status = child_tcb->result;

    //Deallocate child block
    //munmap(child_tcb, thread_block_info.total_size);   

    return 0;

}


void pthread_exit (void* status) {
    // TODO: The good way to solve this is to have the child, not its parent, free
    // its own stack (and TLS segment). This enables detached threads. But to do this
    // you need an extra stack. A way to do this is to have a global, lock-protected 
    // manager stack, or have the M5 exit system call do it... Anyhow, I'm deferring
    // this problem until we have TLS.

    //From point (XXX)  on, the thread **does not exist**,
    //as its parent may have already freed the stack. 
    //So we must call sys_exit without using the stack => asm

    // NOTE: You may be tempted to call exit(0) or _exit(0) here, but there call exit_group,
    // killing the whole process and not just the current thread

    //If the keys array was allocated, free it
    if (pthread_specifics != NULL) free(pthread_specifics);

    //Main thread
    if (__tcb == NULL) _exit(0);

    DEBUG("Child TID=0x%llx in pthread_exit...\n", pthread_self() );
    __tcb->result = status;
    //TODO mem barrier here...
    __tcb->child_finished = 1;
    //XXX
    syscall(__NR_exit,0);
    assert(0); //should never be reached

/*#if defined(__x86) or defined(__x86_64)
    __asm__ __volatile__  (
         "\nmov  $0x3c,%%eax\n\t" \
         "syscall\n\t" 
         ::: "eax");
#elif defined(__alpha)
    __asm__ __volatile__  (
         "\nldi  $0,1\n\t" \
         "callsys\n\t");
#elif defined(__sparc)
    // Since this part of the code is provisional, don't bother with asm for now
    syscall(__NR_exit,0);
#else
    #error "No pthread_exit asm for your arch, sorry!\n"
#endif

    assert(0);*/
}


// mutex functions

int pthread_mutex_init (pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    mutex->PTHREAD_MUTEX_T_COUNT = 0;
    return 0;
}

int pthread_mutex_lock (pthread_mutex_t* lock) {
    PROFILE_LOCK_START(lock); 
    spin_lock((int*)&lock->PTHREAD_MUTEX_T_COUNT);
    PROFILE_LOCK_END(lock);
    return 0;
}

int pthread_mutex_unlock (pthread_mutex_t* lock) {
    PROFILE_UNLOCK_START(lock);
    spin_unlock((int*)&lock->PTHREAD_MUTEX_T_COUNT);
    PROFILE_UNLOCK_END(lock);
    return 0;
}

int pthread_mutex_destroy (pthread_mutex_t* mutex) {
    return 0;
}

int pthread_mutex_trylock (pthread_mutex_t* mutex) {
    int acquired = trylock((int*)&mutex->PTHREAD_MUTEX_T_COUNT);
    if (acquired == 1) {
	//Profiling not really accurate here...
	PROFILE_LOCK_START(mutex);
	PROFILE_LOCK_END(mutex);
        return 0;
    }
    return EBUSY;
}

// rwlock functions

int pthread_rwlock_init (pthread_rwlock_t* lock, const pthread_rwlockattr_t* attr) {
    PTHREAD_RWLOCK_T_LOCK(lock) = 0; // used only with spin_lock, so we know to initilize to zero
    PTHREAD_RWLOCK_T_READERS(lock) = 0;
    PTHREAD_RWLOCK_T_WRITER(lock) = -1; // -1 means no one owns the write lock

    return 0;
}

int pthread_rwlock_destroy (pthread_rwlock_t* lock) {
    return 0;
}

int pthread_rwlock_rdlock (pthread_rwlock_t* lock) {
    PROFILE_LOCK_START(lock);
    do {
        // this is to reduce the contention and a possible live-lock to lock->access_lock
        while (1) {
            pthread_t writer = PTHREAD_RWLOCK_T_WRITER(lock);
            if (writer == -1) {
                break;
            }
        }

        spin_lock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
        if ((pthread_t)PTHREAD_RWLOCK_T_WRITER(lock) == -1) {
            PTHREAD_RWLOCK_T_READERS(lock)++;
            spin_unlock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
	    PROFILE_LOCK_END(lock);
            return 0;
        }
        spin_unlock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
    } while (1);
    PROFILE_LOCK_END(lock);
    return 0;
}

int pthread_rwlock_wrlock (pthread_rwlock_t* lock) {
    PROFILE_LOCK_START(lock);
    do {
        while (1) {
            pthread_t writer = PTHREAD_RWLOCK_T_WRITER(lock);
            if (writer == -1) {
                break;
            }
            int num_readers = PTHREAD_RWLOCK_T_READERS(lock);
            if (num_readers == 0) {
                break;
            }
        }

        spin_lock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
        if ((pthread_t)PTHREAD_RWLOCK_T_WRITER(lock) == -1 && PTHREAD_RWLOCK_T_READERS(lock) == 0) {
            PTHREAD_RWLOCK_T_WRITER(lock) = pthread_self();
            spin_unlock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
	    PROFILE_LOCK_END(lock);
            return 0;
        }
        spin_unlock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
    } while (1);
    PROFILE_LOCK_END(lock);
    return 0;
}

int pthread_rwlock_unlock (pthread_rwlock_t* lock) {
    PROFILE_UNLOCK_START(lock);
    spin_lock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
    if (pthread_self() == PTHREAD_RWLOCK_T_WRITER(lock)) {
        // the write lock will be released
        PTHREAD_RWLOCK_T_WRITER(lock) = -1;
    } else {
        // one of the read locks will be released
        PTHREAD_RWLOCK_T_READERS(lock) = PTHREAD_RWLOCK_T_READERS(lock) - 1;
    }
    spin_unlock((int*)&(PTHREAD_RWLOCK_T_LOCK(lock)));
    PROFILE_UNLOCK_END(lock);
    return 0;
}


// key functions
#ifndef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX 1024
#endif

typedef struct {
  int in_use;
  void (*destr)(void*);
} pthread_key_struct;

static pthread_key_struct pthread_keys[PTHREAD_KEYS_MAX];
static pthread_mutex_t pthread_keys_mutex = PTHREAD_MUTEX_INITIALIZER;

int pthread_key_create (pthread_key_t* key, void (*destructor)(void*)) {
  int i;

  pthread_mutex_lock(&pthread_keys_mutex);
  for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
    if (! pthread_keys[i].in_use) {
      /* Mark key in use */
      pthread_keys[i].in_use = 1;
      pthread_keys[i].destr = destructor;
      pthread_mutex_unlock(&pthread_keys_mutex);
      *key = i;
      return 0;
    }
  }
  pthread_mutex_unlock(&pthread_keys_mutex);
  return EAGAIN;
}

int pthread_key_delete (pthread_key_t key)
{
  pthread_mutex_lock(&pthread_keys_mutex);
  if (key >= PTHREAD_KEYS_MAX || !pthread_keys[key].in_use) {
    pthread_mutex_unlock(&pthread_keys_mutex);
    return EINVAL;
  }
  pthread_keys[key].in_use = 0;
  pthread_keys[key].destr = NULL;

  /* NOTE: The LinuxThreads implementation actually zeroes deleted keys on
     spawned threads. I don't care, the spec says that if you are  access a
     key after if has been deleted, you're on your own. */

  pthread_mutex_unlock(&pthread_keys_mutex);
  return 0;
}

int pthread_setspecific (pthread_key_t key, const void* value) {
  int m_size;
  if (key < 0 || key >= PTHREAD_KEYS_MAX) return EINVAL; 
  if (key >= pthread_specifics_size) {
    m_size = (key+1)*sizeof(void*);
    if (pthread_specifics_size == 0) {
       pthread_specifics = (void**) malloc(m_size);
       DEBUG("pthread_setspecific: malloc of size %d bytes, got 0x%llx\n", m_size, pthread_specifics);
    } else {
       pthread_specifics = (void**) realloc(pthread_specifics, m_size);
       DEBUG("pthread_setspecific: realloc of size %d bytes, got 0x%llx\n", m_size, pthread_specifics);
    }
    pthread_specifics_size = key+1;
  }
  pthread_specifics[key] = (void*) value;
  return 0;
}

void* pthread_getspecific (pthread_key_t key) {
  if (key < 0 || key >= pthread_specifics_size) return NULL;
  DEBUG("pthread_getspecific: key=%d pthread_specifics_size=%d\n", key, pthread_specifics_size);
  return pthread_specifics[key]; 
}

// condition variable functions

int pthread_cond_init (pthread_cond_t* cond, const pthread_condattr_t* attr) {
    PTHREAD_COND_T_FLAG(cond) = 0;
    PTHREAD_COND_T_THREAD_COUNT(cond) = 0;
    PTHREAD_COND_T_COUNT_LOCK(cond) = 0;
    return 0;    
}

int pthread_cond_destroy (pthread_cond_t* cond) {
    return 0;
}

int pthread_cond_broadcast (pthread_cond_t* cond) {
    PTHREAD_COND_T_FLAG(cond) = 1;
    return 0;
}

int pthread_cond_wait (pthread_cond_t* cond, pthread_mutex_t* lock) {
    PROFILE_COND_WAIT_START(cond);
    volatile int* thread_count  = &(PTHREAD_COND_T_THREAD_COUNT(cond));
    volatile int* flag = &(PTHREAD_COND_T_FLAG(cond));
    volatile int* count_lock    = &(PTHREAD_COND_T_COUNT_LOCK(cond));

    // dsm: ++/-- have higher precedence than *, so *thread_count++
    // increments *the pointer*, then dereferences it (!)
    (*thread_count)++;

    pthread_mutex_unlock(lock);
    while (1) {
        volatile int f = *flag;
        if (f == 1) {
            break;
        }
    }

    spin_lock(count_lock);

    (*thread_count)--;

    if (*thread_count == 0) {
        *flag = 0;
    }
    spin_unlock(count_lock);
    pthread_mutex_lock(lock);
    PROFILE_COND_WAIT_END(cond);
    return 0;
}

int pthread_cond_signal (pthread_cond_t* cond) {
    //Could also signal only one thread, but this is compliant too
    //TODO: Just wake one thread up
    return pthread_cond_broadcast(cond);
}


//barrier functions

//These funny tree barriers will only work with consecutive TIDs starting from 0, e.g. a barrier initialized for 8 thread will need to be taken by TIDs 0-7
//TODO: Adapt to work with arbitrary TIDs
/*int pthread_barrier_init (pthread_barrier_t *restrict barrier,
                          const pthread_barrierattr_t *restrict attr, unsigned count)
{
    assert(barrier != NULL);
    //assert(0 < count && count <= MAX_NUM_CPUS);

    PTHREAD_BARRIER_T_NUM_THREADS(barrier) = count;

    // add one to avoid false sharing
    tree_barrier_t* ptr
        = ((tree_barrier_t*)malloc((count + 1) * sizeof(tree_barrier_t))) + 1;
    for (unsigned i = 0; i < count; ++i) {
      ptr[i].value = 0;
    }

    PTHREAD_BARRIER_T_BARRIER_PTR(barrier) = ptr;

    return 0;
}

int pthread_barrier_destroy (pthread_barrier_t *barrier)
{
    free(PTHREAD_BARRIER_T_BARRIER_PTR(barrier) - 1);
    return 0;
}

int pthread_barrier_wait (pthread_barrier_t* barrier)
{
    int const num_threads = PTHREAD_BARRIER_T_NUM_THREADS(barrier);
    int const self = pthread_self(); 
    tree_barrier_t * const barrier_ptr = PTHREAD_BARRIER_T_BARRIER_PTR(barrier);

    int const goal = 1 - barrier_ptr[self].value;

    int round_mask = 3;
    while ((self & round_mask) == 0 && round_mask < (num_threads << 2)) {
      int const spacing = (round_mask + 1) >> 2;
      for (int i = 1; i <= 3 && self + i*spacing < num_threads; ++i) {
        while (barrier_ptr[self + i*spacing].value != goal) {
          // spin
        }
      }
      round_mask = (round_mask << 2) + 3;
    }

    barrier_ptr[self].value = goal;
    while (barrier_ptr[0].value != goal) {
      // spin
    }

    return 0;
}*/

int pthread_barrier_init (pthread_barrier_t *restrict barrier,
                          const pthread_barrierattr_t *restrict attr, unsigned count)
{
    assert(barrier != NULL);

    PTHREAD_BARRIER_T_NUM_THREADS(barrier) =  count;
    PTHREAD_BARRIER_T_SPINLOCK(barrier) = 0;
    PTHREAD_BARRIER_T_COUNTER(barrier) = 0;
    PTHREAD_BARRIER_T_DIRECTION(barrier) = 0; //up

    return 0;
}

int pthread_barrier_destroy (pthread_barrier_t *barrier)
{
    //Nothing to do
    return 0;
}

int pthread_barrier_wait (pthread_barrier_t* barrier)
{
    PROFILE_BARRIER_WAIT_START(barrier);
    int const initial_direction = PTHREAD_BARRIER_T_DIRECTION(barrier); //0 == up, 1 == down

    if (initial_direction == 0) {
       spin_lock(&(PTHREAD_BARRIER_T_SPINLOCK(barrier)));
       PTHREAD_BARRIER_T_COUNTER(barrier)++; 
       if (PTHREAD_BARRIER_T_COUNTER(barrier) == PTHREAD_BARRIER_T_NUM_THREADS(barrier)) {
           //reverse direction, now down
           PTHREAD_BARRIER_T_DIRECTION(barrier) = 1;
       }
       spin_unlock(&(PTHREAD_BARRIER_T_SPINLOCK(barrier)));
    } else {
       spin_lock(&(PTHREAD_BARRIER_T_SPINLOCK(barrier)));
       PTHREAD_BARRIER_T_COUNTER(barrier)--;
       if (PTHREAD_BARRIER_T_COUNTER(barrier) == 0) {
          //reverse direction, now up
          PTHREAD_BARRIER_T_DIRECTION(barrier) = 0;
       }
       spin_unlock(&(PTHREAD_BARRIER_T_SPINLOCK(barrier)));
   }

   volatile int direction = PTHREAD_BARRIER_T_DIRECTION(barrier);
   while (initial_direction == direction) {
      //spin
      direction = PTHREAD_BARRIER_T_DIRECTION(barrier);
   }
   PROFILE_BARRIER_WAIT_END(barrier);
   return 0;
}

//misc functions

static pthread_mutex_t __once_mutex = PTHREAD_MUTEX_INITIALIZER;
int pthread_once (pthread_once_t* once,
                  void (*init)(void))
{
  //fast path
  if (*once != PTHREAD_ONCE_INIT) return 0;
  pthread_mutex_lock(&__once_mutex);
  if (*once != PTHREAD_ONCE_INIT) {
    pthread_mutex_unlock(&__once_mutex);
    return 0;
  }
  *once = PTHREAD_ONCE_INIT+1;
  init();
  pthread_mutex_unlock(&__once_mutex);
  return 0;
}

#ifndef __USE_EXTERN_INLINES
int pthread_equal (pthread_t t1, pthread_t t2)
{
    return t1 == t2; //that was hard :-)
}
#endif

// Functions that we want defined, but we don't use them
// All other functions are not defined so that they will cause a compile time
// error and we can decide if we need to do something with them

// functions really don't need to do anything

int pthread_yield() {
    // nothing else to yield to
    return 0;
}

int pthread_attr_init (pthread_attr_t* attr) {
    return 0;
}

int pthread_attr_setscope (pthread_attr_t* attr, int scope) {
    return 0;
}

int pthread_rwlockattr_init (pthread_rwlockattr_t* attr) {
    return 0;
}

int pthread_attr_setstacksize (pthread_attr_t* attr, size_t stacksize) {
    return 0;
}

int pthread_attr_setschedpolicy (pthread_attr_t* attr, int policy) {
    return 0;
}

// some functions that we don't really support

int pthread_setconcurrency (int new_level) {
    return 0;
}

int pthread_setcancelstate (int p0, int* p1)
{
    //NPTL uses this
    return 0;
}

//and some affinity functions (used by libgomp, openmp)
int pthread_getaffinity_np(pthread_t thread, size_t size, cpu_set_t *set) {
  return 0;
}

int pthread_setaffinity_np(pthread_t thread, size_t size, cpu_set_t *set) {
  return 0;
}

int pthread_attr_setaffinity_np(pthread_attr_t attr, size_t cpusetsize, const cpu_set_t *cpuset) {
  return 0;
}

int pthread_attr_getaffinity_np(pthread_attr_t attr, size_t cpusetsize, cpu_set_t *cpuset) {
  return 0;
}


// ... including any dealing with thread-level signal handling
// (maybe we should throw an error message instead?)

int pthread_sigmask (int how, const sigset_t* set, sigset_t* oset) {
    return 0;
}

int pthread_kill (pthread_t thread, int sig)  {
    assert(0);
}

// unimplemented pthread functions

int pthread_atfork (void (*f0)(void),
                    void (*f1)(void),
                    void (*f2)(void))
{
    assert(0);
}

int pthread_attr_destroy (pthread_attr_t* attr)
{
    assert(0);
}

int pthread_attr_getdetachstate (const pthread_attr_t* attr,
                                 int* b)
{
    assert(0);
}

int pthread_attr_getguardsize (const pthread_attr_t* restrict a,
                               size_t *restrict b)
{
    assert(0);
}

int pthread_attr_getinheritsched (const pthread_attr_t *restrict a,
                                  int *restrict b)
{
    assert(0);
}

int pthread_attr_getschedparam (const pthread_attr_t *restrict a,
                                struct sched_param *restrict b)
{
    assert(0);
}

int pthread_attr_getschedpolicy (const pthread_attr_t *restrict a,
                                 int *restrict b)
{
    assert(0);
}

int pthread_attr_getscope (const pthread_attr_t *restrict a,
                           int *restrict b)
{
    assert(0);
}

int pthread_attr_getstack (const pthread_attr_t *restrict a,
                           void* *restrict b,
                           size_t *restrict c)
{
    assert(0);
}

int pthread_attr_getstackaddr (const pthread_attr_t *restrict a,
                               void* *restrict b)
{
    assert(0);
}

int pthread_attr_getstacksize (const pthread_attr_t *restrict a,
                               size_t *restrict b)
{
    assert(0);
}

int pthread_attr_setdetachstate (pthread_attr_t* a,
                                 int b)
{
   return 0; //FIXME
}
int pthread_attr_setguardsize (pthread_attr_t* a,
                               size_t b)
{
    assert(0);
}

int pthread_attr_setinheritsched (pthread_attr_t* a,
                                  int b)
{
    assert(0);
}

int pthread_attr_setschedparam (pthread_attr_t *restrict a,
                                const struct sched_param *restrict b)
{
    assert(0);
}

int pthread_attr_setstack (pthread_attr_t* a,
                           void* b,
                           size_t c)
{
    assert(0);
}

int pthread_attr_setstackaddr (pthread_attr_t* a,
                               void* b)
{
    assert(0);
}

int pthread_cancel (pthread_t a)
{
    assert(0);
}

void _pthread_cleanup_push (struct _pthread_cleanup_buffer *__buffer,
                            void (*__routine) (void *),
                            void *__arg) 
{
    assert(0);
}

void _pthread_cleanup_pop (struct _pthread_cleanup_buffer *__buffer,
                           int __execute) 
{
    assert(0);
}

int pthread_cond_timedwait (pthread_cond_t *restrict a,
                            pthread_mutex_t *restrict b,
                            const struct timespec *restrict c)
{
    assert(0);
}

int pthread_condattr_destroy (pthread_condattr_t* a)
{
    assert(0);
}

int pthread_condattr_getpshared (const pthread_condattr_t *restrict a,
                                 int *restrict b)
{
    assert(0);
}

int pthread_condattr_init (pthread_condattr_t* a)
{
    assert(0);
}

int pthread_condattr_setpshared (pthread_condattr_t* a,
                                 int b)
{
    assert(0);
}

int pthread_detach (pthread_t a)
{
    assert(0);
}


int pthread_getconcurrency ()
{
    assert(0);
}

int pthread_getschedparam(pthread_t a,
                          int *restrict b,
                          struct sched_param *restrict c)
{
    assert(0);
}

int pthread_mutex_getprioceiling (const pthread_mutex_t *restrict a,
                                  int *restrict b)
{
    assert(0);
}

int pthread_mutex_setprioceiling (pthread_mutex_t *restrict a,
                                  int b,
                                  int *restrict c)
{
    assert(0);
}

int pthread_mutex_timedlock (pthread_mutex_t* a,
                             const struct timespec* b)
{
    assert(0);
}

int pthread_mutexattr_destroy (pthread_mutexattr_t* a)
{
    //assert(0);
    //used by libc
    return 0;
}

int pthread_mutexattr_getprioceiling (const pthread_mutexattr_t *restrict a,
                                      int *restrict b)
{
    assert(0);
}

int pthread_mutexattr_getprotocol (const pthread_mutexattr_t *restrict a,
                                   int *restrict b)
{
    assert(0);
}

int pthread_mutexattr_getpshared (const pthread_mutexattr_t *restrict a,
                                  int *restrict b)
{
    assert(0);
}

int pthread_mutexattr_gettype (const pthread_mutexattr_t *restrict a,
                               int *restrict b)
{
    assert(0);
}

int pthread_mutexattr_init (pthread_mutexattr_t* a)
{
    //assert(0);
    //used by libc
    return 0;
}

int pthread_mutexattr_setprioceiling (pthread_mutexattr_t* a,
                                      int b)
{
    assert(0);
}

int pthread_mutexattr_setprotocol (pthread_mutexattr_t* a,
                                   int b)
{
    assert(0);
}

int pthread_mutexattr_setpshared (pthread_mutexattr_t* a,
                                  int b)
{
    assert(0);
}

int pthread_mutexattr_settype (pthread_mutexattr_t* a,
                               int b)
{
    //assert(0);
    //used by libc
    //yeah, and the freaking libc just needs a recursive lock.... screw it
    //if (b == PTHREAD_MUTEX_RECURSIVE_NP) assert(0);
    return 0;
}

int pthread_rwlock_timedrdlock (pthread_rwlock_t *restrict a,
                                const struct timespec *restrict b)
{
    assert(0);
}

int pthread_rwlock_timedwrlock (pthread_rwlock_t *restrict a,
                                const struct timespec *restrict b)
{
    assert(0);
}

int pthread_rwlock_tryrdlock (pthread_rwlock_t* a)
{
    assert(0);
}

int pthread_rwlock_trywrlock (pthread_rwlock_t* a)
{
    assert(0);
}

int pthread_rwlockattr_destroy (pthread_rwlockattr_t* a)
{
    assert(0);
}

int pthread_rwlockattr_getpshared (const pthread_rwlockattr_t *restrict a,
                                   int *restrict b)
{
    assert(0);
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t* a,
                                  int b)
{
    assert(0);
}

int pthread_setcanceltype (int a,
                           int* b)
{
    assert(0);
}

int pthread_setschedparam (pthread_t a,
                           int b,
                           const struct sched_param* c)
{
    assert(0);
}

int pthread_setschedprio (pthread_t a,
                          int b)
{
    assert(0);
}

void pthread_testcancel ()
{
    assert(0);
}


/* Stuff to properly glue with glibc */

// glibc keys

//For NPTL, or LinuxThreads with TLS defined and used
__thread void* __libc_tsd_MALLOC;
__thread void* __libc_tsd_DL_ERROR;
__thread void* __libc_tsd_RPC_VARS;
//__thread void* __libc_tsd_LOCALE; seems to be defined in my libc already, but your glibc might not dfine it...
//Defined in libgomp (OpenMP)
//__thread void* __libc_tsd_CTYPE_B;
//__thread void* __libc_tsd_CTYPE_TOLOWER;
//__thread void* __libc_tsd_CTYPE_TOUPPER;

//If glibc was not compiled with __thread, it uses __pthread_internal_tsd_get/set/address for its internal keys
//These are from linuxthreads-0.7.1/specific.c

//FIXME: When enabled, SPARC/M5 crashes (for some weird reason, libc calls a tsd_get on an uninitialized key at initialization, and uses its result). Are we supposed to initialize these values??
//libc can live without these, so it's not critical
#if 0
enum __libc_tsd_key_t { _LIBC_TSD_KEY_MALLOC = 0,
                        _LIBC_TSD_KEY_DL_ERROR,
                        _LIBC_TSD_KEY_RPC_VARS,
                        _LIBC_TSD_KEY_LOCALE,
                        _LIBC_TSD_KEY_CTYPE_B,
                        _LIBC_TSD_KEY_CTYPE_TOLOWER,
                        _LIBC_TSD_KEY_CTYPE_TOUPPER,
                        _LIBC_TSD_KEY_N };
__thread void* p_libc_specific[_LIBC_TSD_KEY_N]; /* thread-specific data for libc */

int
__pthread_internal_tsd_set (int key, const void * pointer)
{
  p_libc_specific[key] = (void*) pointer;
  return 0;
}

void *
__pthread_internal_tsd_get (int key)
{
  return  p_libc_specific[key];
}

void ** __attribute__ ((__const__))
__pthread_internal_tsd_address (int key)
{
  return &p_libc_specific[key];
}
#endif //0


//Aliases for glibc
int __pthread_mutex_init (pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)  __attribute__ ((weak, alias ("pthread_mutex_init")));
int __pthread_mutex_lock (pthread_mutex_t* lock) __attribute__ ((weak, alias ("pthread_mutex_lock")));
int __pthread_mutex_trylock (pthread_mutex_t* lock) __attribute__ ((weak, alias ("pthread_mutex_trylock")));
int __pthread_mutex_unlock (pthread_mutex_t* lock) __attribute__ ((weak, alias ("pthread_mutex_unlock")));

int __pthread_mutexattr_destroy (pthread_mutexattr_t* a) __attribute__ ((weak, alias ("pthread_mutexattr_destroy")));
int __pthread_mutexattr_init (pthread_mutexattr_t* a) __attribute__ ((weak, alias ("pthread_mutexattr_init")));
int __pthread_mutexattr_settype (pthread_mutexattr_t* a, int b) __attribute__ ((weak, alias ("pthread_mutexattr_settype")));

int __pthread_rwlock_init (pthread_rwlock_t* lock, const pthread_rwlockattr_t* attr) __attribute__ ((weak, alias ("pthread_rwlock_init")));  
int __pthread_rwlock_rdlock (pthread_rwlock_t* lock) __attribute__ ((weak, alias ("pthread_rwlock_rdlock")));
int __pthread_rwlock_wrlock (pthread_rwlock_t* lock) __attribute__ ((weak, alias ("pthread_rwlock_wrlock")));
int __pthread_rwlock_unlock (pthread_rwlock_t* lock) __attribute__ ((weak, alias ("pthread_rwlock_unlock")));
int __pthread_rwlock_destroy (pthread_rwlock_t* lock) __attribute__ ((weak, alias ("pthread_rwlock_destroy")));
/*
int   __pthread_key_create(pthread_key_t *, void (*)(void *)) __attribute__ ((weak, alias ("pthread_key_create")));
int   __pthread_key_delete(pthread_key_t) __attribute__ ((weak, alias ("pthread_key_delete")));
void* __pthread_getspecific(pthread_key_t) __attribute__ ((weak, alias ("pthread_getspecific")));
int   __pthread_setspecific(pthread_key_t, const void *) __attribute__ ((weak, alias ("pthread_setspecific")));
*/
int __pthread_once (pthread_once_t* once, void (*init)(void))  __attribute__ ((weak, alias ("pthread_once")));


//No effect, NPTL-specific, may cause leaks? (TODO: Check!)
void __nptl_deallocate_tsd() {}


