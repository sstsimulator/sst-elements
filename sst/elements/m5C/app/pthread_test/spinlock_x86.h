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


#ifndef __SPINLOCK_X86_H__
#define __SPINLOCK_X86_H__

// routines from /usr/src/linux/include/asm-x86/spinlock.h

static __inline__ void spin_lock (volatile int* lock) {
    char oldval;
    __asm__ __volatile__
        (
         "\n1:\t" \
         "cmpb $0,%1\n\t" \
         "ja 1b\n\t" \
         "xchgb %b0, %1\n\t" \
         "cmpb $0,%0\n" \
         "ja 1b\n\t"
         :"=q"(oldval), "=m"(*lock)
         : "0"(1)
         : "memory");
}

static __inline__ void spin_unlock (volatile int* lock) {
	__asm__ __volatile__
        ("movb $0,%0" \
         :"=m" (*lock) : : "memory");
}

static __inline__ int trylock (volatile int* lock) {
    char oldval;
    __asm__ __volatile__
        (
         "xchgb %b0,%1"
         :"=q" (oldval),
          "=m" (*lock)
         :"0" (1) 
         : "memory");
    return oldval == 0;
}

#endif  // __SPINLOCK_X86_H__
