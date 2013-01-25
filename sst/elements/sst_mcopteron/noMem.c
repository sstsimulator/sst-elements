#include <stdio.h>

#define INNER_LOOP 1024*1024
#define OUTER_LOOP 100
int main() { 

  register int a,b;
  register double c, d=0.0;
  for(a=0; a<OUTER_LOOP; a++) 
    for(b=0; b<INNER_LOOP; b++) { 
      // do some simple computations
      c = (double)(b+2.25); 
      d = (double)(2.0+c)*(d+1.0);
    }

  return d>2.5; 

} 
