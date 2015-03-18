// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <string.h>

int main()
{
   char line[256];
   unsigned long long lv;
   double dv;
   char name[40], value[40];
   while (fgets(line,sizeof(line),stdin)) {
      if (sscanf(line,"#define %s %s", &name, &value) == 2) {
         printf("matched (%s) (%s)\n",name,value);
         if (strstr(value,"L")) {
            sscanf(value, "%llu", &lv);
            printf("ull val = %llu\n",lv);
         } else if (strstr(value,".")) {
            sscanf(value, "%lg", &dv);
            printf("dbl val = %lg\n",dv);
         }
      }
   }
   return 0;
}
