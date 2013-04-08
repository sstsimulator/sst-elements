// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <pthread.h>
#include <stdlib.h>

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    abort();
}
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    abort();
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    abort();
}
int pthread_create(pthread_t * thread,
              const pthread_attr_t * attr,
              void *(*start_routine)(void*), void * arg)
{
    abort();
}


int pthread_attr_destroy(pthread_attr_t *attr)
{
    abort();
}

int pthread_attr_init(pthread_attr_t *attr)
{
    abort();
}

pthread_t pthread_self(void)
{
    abort();
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    abort();
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    abort();
}

int pthread_cond_timedwait(pthread_cond_t * cond,
              pthread_mutex_t * mutex,
              const struct timespec * abstime)
{
    abort();
}
int pthread_cond_wait(pthread_cond_t * cond,
              pthread_mutex_t * mutex)
{
    abort();
}

int pthread_getschedparam(pthread_t thread, int * policy,
              struct sched_param * param)
{
    abort();
}

int pthread_setschedparam(pthread_t thread, int policy,
              const struct sched_param *param)
{
    abort();
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr,
              int *detachstate)
{
    abort();
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    abort();
}

int pthread_attr_getstacksize(const pthread_attr_t * attr,
              size_t * stacksize)
{
    abort();
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    abort();
}
