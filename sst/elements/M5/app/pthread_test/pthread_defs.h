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
*/



#ifndef __PTHREAD_DEFS_H__
#define __PTHREAD_DEFS_H__


/*typedef struct {
    volatile int value;
    long _padding[15]; // to prevent false sharing
} tree_barrier_t;*/

// old LinuxThreads needs different magic than newer NPTL implementation
// definitions for LinuxThreads
#ifdef __linux__

//XOPEN2K and UNIX98 defines to avoid for rwlocks/barriers when compiling with gcc...
//see <bits/pthreadtypes.h>
#if !defined(__USE_UNIX98) && !defined(__USE_XOPEN2K) && !defined(__SIZEOF_PTHREAD_MUTEX_T)
/* Read-write locks.  */
typedef struct _pthread_rwlock_t
{
  struct _pthread_fastlock __rw_lock; /* Lock to guarantee mutual exclusion */
  int __rw_readers;                   /* Number of readers */
  _pthread_descr __rw_writer;         /* Identity of writer, or NULL if none */
  _pthread_descr __rw_read_waiting;   /* Threads waiting for reading */
  _pthread_descr __rw_write_waiting;  /* Threads waiting for writing */
  int __rw_kind;                      /* Reader/Writer preference selection */
  int __rw_pshared;                   /* Shared between processes or not */
} pthread_rwlock_t;


/* Attribute for read-write locks.  */
typedef struct
{
  int __lockkind;
  int __pshared;
} pthread_rwlockattr_t;
#endif
#if !defined(__USE_XOPEN2K) && !defined(__SIZEOF_PTHREAD_MUTEX_T)
/* POSIX spinlock data type.  */
typedef volatile int pthread_spinlock_t;

/* POSIX barrier. */
typedef struct {
  struct _pthread_fastlock __ba_lock; /* Lock to guarantee mutual exclusion */
  int __ba_required;                  /* Threads needed for completion */
  int __ba_present;                   /* Threads waiting */
  _pthread_descr __ba_waiting;        /* Queue of waiting threads */
} pthread_barrier_t;

/* barrier attribute */
typedef struct {
  int __pshared;
} pthread_barrierattr_t;

#endif


#ifndef  __SIZEOF_PTHREAD_MUTEX_T
#define PTHREAD_MUTEX_T_COUNT __m_count

#define PTHREAD_COND_T_FLAG(cond) (*(volatile int*)(&(cond->__c_lock.__status)))
#define PTHREAD_COND_T_THREAD_COUNT(cond) (*(volatile int*)(&(cond-> __c_waiting)))
#define PTHREAD_COND_T_COUNT_LOCK(cond) (*(volatile int*)(&(cond->__c_lock.__spinlock)))

#define PTHREAD_RWLOCK_T_LOCK(rwlock)  (*(volatile int*)(&rwlock->__rw_lock))
#define PTHREAD_RWLOCK_T_READERS(rwlock)  (*(volatile int*)(&rwlock->__rw_readers))
#define PTHREAD_RWLOCK_T_WRITER(rwlock)  (*(volatile pthread_t*)(&rwlock->__rw_kind))

//For tree barriers
//#define PTHREAD_BARRIER_T_NUM_THREADS(barrier)  (*(int*)(&barrier->__ba_lock.__spinlock))
//#define PTHREAD_BARRIER_T_BARRIER_PTR(barrier) (*(tree_barrier_t**)(&barrier->__ba_required))

#define PTHREAD_BARRIER_T_SPINLOCK(barrier)  (*(volatile int*)(&barrier->__ba_lock.__spinlock))
#define PTHREAD_BARRIER_T_NUM_THREADS(barrier) (*((volatile int*)(&barrier->__ba_required)))
#define PTHREAD_BARRIER_T_COUNTER(barrier) (*((volatile int*)(&barrier->__ba_present)))
#define PTHREAD_BARRIER_T_DIRECTION(barrier) (*((volatile int*)(&barrier->__ba_waiting)))

// definitions for NPTL implementation
#else /* __SIZEOF_PTHREAD_MUTEX_T defined */
#define PTHREAD_MUTEX_T_COUNT __data.__count

#define PTHREAD_RWLOCK_T_LOCK(rwlock)  (*(volatile int*)(&rwlock->__data.__lock))
#define PTHREAD_RWLOCK_T_READERS(rwlock)  (*(volatile int*)(&rwlock->__data.__nr_readers))
#define PTHREAD_RWLOCK_T_WRITER(rwlock)  (*(volatile int*)(&rwlock->__data.__writer))

#if defined(__GNUC__) && __GNUC__ >= 4
#define PTHREAD_COND_T_FLAG(cond) (*(volatile int*)(&(cond->__data.__lock)))
#define PTHREAD_COND_T_THREAD_COUNT(cond) (*(volatile int*)(&(cond-> __data.__futex)))
#define PTHREAD_COND_T_COUNT_LOCK(cond) (*(volatile int*)(&(cond->__data.__nwaiters)))

//For tree barriers
//#define PTHREAD_BARRIER_T_NUM_THREADS(barrier)  (*((int*)(barrier->__size+(0*sizeof(int)))))
//#define PTHREAD_BARRIER_T_BARRIER_PTR(barrier) (*(tree_barrier_t**)(barrier->__size+(1*sizeof(int))))

#define PTHREAD_BARRIER_T_SPINLOCK(barrier) (*((volatile int*)(barrier->__size+(0*sizeof(int)))))
#define PTHREAD_BARRIER_T_NUM_THREADS(barrier) (*((volatile int*)(barrier->__size+(1*sizeof(int)))))
#define PTHREAD_BARRIER_T_COUNTER(barrier) (*((volatile int*)(barrier->__size+(2*sizeof(int)))))
#define PTHREAD_BARRIER_T_DIRECTION(barrier) (*((volatile int*)(barrier->__size+(3*sizeof(int)))))

//Tree barrier-related
#if 0
#ifndef __SIZEOF_PTHREAD_BARRIER_T
#error __SIZEOF_PTHREAD_BARRIER_T not defined
#endif
#if ((4/*fields*/*4/*sizeof(int32)*/) > __SIZEOF_PTHREAD_BARRIER_T)
#error barrier size __SIZEOF_PTHREAD_BARRIER_T not large enough for our implementation
#endif
#endif

#else // gnuc >= 4
//gnuc < 4
#error "This library requires gcc 4.0+ (3.x should work, but you'll need to change pthread_defs.h)"
#endif // gnuc >= 4

#endif // LinuxThreads / NPTL

// non-linux definitions... fill this in?
#else // !__linux__
  #error "Non-Linux pthread definitions not available"
#endif //!__linux__

#endif //  __PTHREAD_DEFS_H__
