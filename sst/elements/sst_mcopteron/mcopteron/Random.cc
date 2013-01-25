
#include "mersenne.h"

namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library
double genRandomProbability()
{
   double r = (double) genrand_real1(); // this function is defined in mersene.h
   return r;
}

void seedRandom(unsigned long seed)
{
   init_genrand(seed);
}
}//end namespace McOpteron
