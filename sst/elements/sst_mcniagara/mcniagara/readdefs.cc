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
