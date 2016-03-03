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

/* Profiling hooks used by m5threads to measure synchronization usage */

//TODO: Profiling hooks for non-M5 mode

#if defined(M5_PROFILING)

  /* M5 profiling syscall asm */
  #if defined (__sparc)
    #define m5_prof_syscall(syscall_num, arg) __asm__ __volatile__ ( \
       "mov " #syscall_num ", %%g1\n\t" \
       "mov %0, %%o0\n\t" \
       "ta 0x6d\n\t" \
       :: "r"(arg) : "g1", "o0" \
    );
  #else
    #error "M5 profiling hooks not implemented for your architecture, write them"
  #endif

  #define PROFILE_LOCK_START(addr) m5_prof_syscall(1040, addr)
  #define PROFILE_LOCK_END(addr) m5_prof_syscall(1041, addr)

  #define PROFILE_UNLOCK_START(addr) m5_prof_syscall(1042, addr)
  #define PROFILE_UNLOCK_END(addr) m5_prof_syscall(1043, addr)

  #define PROFILE_BARRIER_WAIT_START(addr) m5_prof_syscall(1044, addr)
  #define PROFILE_BARRIER_WAIT_END(addr) m5_prof_syscall(1045, addr)

  #define PROFILE_COND_WAIT_START(addr) m5_prof_syscall(1046, addr)
  #define PROFILE_COND_WAIT_END(addr) m5_prof_syscall(1047, addr)

#else
  /* Empty hooks */
  #define PROFILE_LOCK_START(addr)
  #define PROFILE_LOCK_END(addr)

  #define PROFILE_UNLOCK_START(addr)
  #define PROFILE_UNLOCK_END(addr)

  #define PROFILE_BARRIER_WAIT_START(addr)
  #define PROFILE_BARRIER_WAIT_END(addr)

  #define PROFILE_COND_WAIT_START(addr)
  #define PROFILE_COND_WAIT_END(addr)
#endif

