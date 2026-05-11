/* Re-enable Mercury pthread macros after a system header inclusion.
   Undef the guard so hg_pthread_macro.h can be re-entered (no-op if clear
   already undef'd it); then re-include to restore macros. */
#undef SST_HG_LIBRARIES_PTHREAD_HG_PTHREAD_MACRO_H
#undef PTHREAD_ONCE_INIT
#undef PTHREAD_MUTEX_INITIALIZER
#undef PTHREAD_COND_INITIALIZER
#undef PTHREAD_RWLOCK_INITIALIZER
#undef PTHREAD_CREATE_DETACHED
#undef PTHREAD_CREATE_JOINABLE
#undef PTHREAD_MUTEX_NORMAL
#undef PTHREAD_MUTEX_RECURSIVE
#undef PTHREAD_MUTEX_ERRORCHECK
#undef PTHREAD_MUTEX_DEFAULT
#undef PTHREAD_PROCESS_SHARED
#undef PTHREAD_PROCESS_PRIVATE
#undef PTHREAD_SCOPE_PROCESS
#undef PTHREAD_SCOPE_SYSTEM
#undef PTHREAD_BARRIER_SERIAL_THREAD
#include <mercury/operating_system/libraries/pthread/hg_pthread_macro.h>
