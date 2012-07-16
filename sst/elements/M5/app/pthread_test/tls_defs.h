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

#ifndef __TLS_DEFS_H__
#define __TLS_DEFS_H__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


//These are mostly taken verbatim from glibc 2.3.6

//32 for ELF32 binaries, 64 for ELF64
//TODO: Macro it
#define __ELF_NATIVE_CLASS 64

/* Standard ELF types.  */

#include <stdint.h>

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;


typedef struct
{
  Elf32_Word    p_type;                 /* Segment type */
  Elf32_Off     p_offset;               /* Segment file offset */
  Elf32_Addr    p_vaddr;                /* Segment virtual address */
  Elf32_Addr    p_paddr;                /* Segment physical address */
  Elf32_Word    p_filesz;               /* Segment size in file */
  Elf32_Word    p_memsz;                /* Segment size in memory */
  Elf32_Word    p_flags;                /* Segment flags */
  Elf32_Word    p_align;                /* Segment alignment */
} Elf32_Phdr;

typedef struct
{
  Elf64_Word    p_type;                 /* Segment type */
  Elf64_Word    p_flags;                /* Segment flags */
  Elf64_Off     p_offset;               /* Segment file offset */
  Elf64_Addr    p_vaddr;                /* Segment virtual address */
  Elf64_Addr    p_paddr;                /* Segment physical address */
  Elf64_Xword   p_filesz;               /* Segment size in file */
  Elf64_Xword   p_memsz;                /* Segment size in memory */
  Elf64_Xword   p_align;                /* Segment alignment */
} Elf64_Phdr;


#define ElfW(type) _ElfW (Elf, __ELF_NATIVE_CLASS, type)
#define _ElfW(e,w,t)       _ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)     e##w##t


#define PT_TLS              7               /* Thread-local storage segment */


# define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))

extern ElfW(Phdr) *_dl_phdr;
extern size_t _dl_phnum;

//Architecture-specific definitions

#if defined(__x86_64) || defined(__amd64)

/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  void *pointer;
} dtv_t;

typedef struct
{
  void *tcb;            /* Pointer to the TCB.  Not necessary the
                           thread descriptor used by libpthread.  */
  dtv_t *dtv;
  void *self;           /* Pointer to the thread descriptor.  */
  int multiple_threads;
} tcbhead_t;

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

/* Macros to load from and store into segment registers.  */
# define TLS_GET_FS() \
  { int __seg; __asm ("movl %%fs, %0" : "=q" (__seg)); __seg; }
# define TLS_SET_FS(val) \
  __asm ("movl %0, %%fs" :: "q" (val))

# define TLS_INIT_TP(thrdescr, secondcall) \
  { void *_thrdescr = (thrdescr);                                            \
     tcbhead_t *_head = (tcbhead_t *) _thrdescr;                              \
     int _result;                                                             \
                                                                              \
     _head->tcb = _thrdescr;                                                  \
     /* For now the thread descriptor is at the same address.  */             \
     _head->self = _thrdescr;                                                 \
                                                                              \
     /* It is a simple syscall to set the %fs value for the thread.  */       \
     asm volatile ("syscall"                                                  \
                   : "=a" (_result)                                           \
                   : "0" ((unsigned long int) __NR_arch_prctl),               \
                     "D" ((unsigned long int) ARCH_SET_FS),                   \
                     "S" (_thrdescr)                                          \
                   : "memory", "cc", "r11", "cx");                            \
                                                                              \
    _result ? "cannot set %fs base address for thread-local storage" : 0;     \
  }

#elif defined (__sparc)

register struct pthread *__thread_self __asm__("%g7");

/* Code to initially initialize the thread pointer.  */
# define TLS_INIT_TP(descr, secondcall) \
  (__thread_self = (__typeof (__thread_self)) (descr), NULL)

#else
  #error "No TLS defs for your architecture"
#endif

#endif /*__TLS_DEFS_H__*/

