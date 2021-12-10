// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

int main( int argc, char* argv[] ) {
   int boo = 0;
   int mmio_addr = 0xD0000000;

//    __asm__ __volatile__ ( "addi s2, x0, 0x100" );
//    __asm__ __volatile__ ( "sw   x0, 0(s2)" );

   __asm__ __volatile__ (
      "li t1, 0xD0000000;"
//       "li t1, 0x100000000;"
      "sw x0, 0(t1);"
   );

//     asm ("movl %1, %%ebx;"
//          "movl %%ebx, %0;"
//          : "=r" ( val )        /* output */
//          : "r" ( no )         /* input */
//          : "%ebx"         /* clobbered register */
//      );

   return 0;
}
