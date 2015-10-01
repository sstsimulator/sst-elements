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

int main(int argc, char **argv)
{
   char line[128];
   char mnemonic[24], op1[24], op2[24], op3[24];
   unsigned int iOpSize, i;
   unsigned long long occurs;
   unsigned long long totalInstructions;
   double iProbability, loadProb, storeProb;
   FILE *inf;

   // first get total instructions from last line of file
   inf = fopen(argv[1], "r");
   if (!inf)
      return -1;
   while (!feof(inf))
      fgets(line, sizeof(line), inf);
   fclose(inf);
   if (sscanf(line, "TOTAL %llu", &totalInstructions) != 1) {
      fprintf(stderr, "First line not right! (%s), aborting...\n", line);
   }
   printf("total instructions: %llu\n", totalInstructions);

   // now get instructions
   inf = fopen(argv[1], "r");
   if (!inf)
      return -1;
   fgets(line, sizeof(line), inf);  // skip first line
   while (!feof(inf)) {
      fgets(line, sizeof(line), inf);
      op1[0] = '\0';  op2[0] = '\0'; op3[0] = '\0';
      if (sscanf(line, "%s %llu %lf",mnemonic,&occurs,&iProbability) != 3) {
         if (sscanf(line, "%s %s %llu %lf",mnemonic,op1,&occurs,&iProbability) != 4) {
            if (sscanf(line, "%s %s %s %llu %lf",mnemonic,op1,op2,&occurs,&iProbability) != 5) {
               if (sscanf(line, "%s %s %s %s %llu %lf",mnemonic,op1,op2,op3,&occurs,&iProbability) != 6) {
                 fprintf(stderr,"BAD LINE: (%s)\n",line);
                 continue;
               }
            }
         }
      }
      if (!strcmp(mnemonic,"TOTAL"))
         break;

      iProbability = (double)occurs/totalInstructions;

      // for the very few 3-operand insns (all with imm as 3rd) move it to 2nd
      if (strstr(op3,"imm")) {
         strcpy(op2,op3);
      }

      // fixup single-operand instructions and other issues
      if (strstr(mnemonic,"POP")) {
         // source is memory, so set op2="m"
         op2[0] = 'm'; op2[1] = '\0';
      } else if (strstr(mnemonic,"PUSH")) {
         // dest is memory, so move op1 to op2, set op1="m"
         strcpy(op2,op1);
         op1[0] = 'm'; op1[1] = '\0';
      } else if (op2[0]=='\0' && strstr(op1,"imm")) {
         // assume dest a register, and one source is imm (no idea how big!??)
         strcpy(op2,op1);
         strcpy(op1,"r64"); // assume!
      } else if (op2[0]=='\0' && (op1[0]=='r' || op1[0]=='m')) {
         // make source same as dest (e.g., NOT, SHL, NEG, etc.)
         strcpy(op2,op1);
      } else if (strstr(op1,"near")) {
         // jump/call/etc
         op1[0] = '\0';
      }

      // find operand size (order is important here because 128 has an 8 in it
      // and some instructions have both 64 and 32 bit operands (we choose 64 first)
      if (strstr(op1,"128") || strstr(op2,"128")) {
         iOpSize = 128;
      } else if (strstr(op1,"64") || strstr(op2,"64")) {
         iOpSize = 64;
      } else if (strstr(op1,"32") || strstr(op2,"32")) {
         iOpSize = 32;
      } else if (strstr(op1,"16") || strstr(op2,"16")) {
         iOpSize = 16;
      } else if (strstr(op1,"8") || strstr(op2,"8")) {
         iOpSize = 8;
      } else {
         iOpSize = 64;
      }

      if (strstr(op1,"m")) {
         strcpy(op1,"mem");
         storeProb = 1.0;
      } else if (strstr(op1,"r")) {
         strcpy(op1,"reg");
         storeProb = 0.0;
      } else
         storeProb = 0.0;

      if (strstr(op2,"m")) {
         strcpy(op2,"mem");
         loadProb = 1.0;
      } else if (strstr(op2,"r")) {
         strcpy(op2,"reg");
         loadProb = 0.0;
      } else
         loadProb = 0.0;

      printf("%s:%d %s%g,%s%g %llu  %lg\n",mnemonic,iOpSize,
             op1,storeProb,op2,loadProb,occurs,iProbability);

   }
   fclose(inf);
}

