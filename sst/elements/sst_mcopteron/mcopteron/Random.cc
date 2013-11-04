
#include "Random.h"

namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library
RandomFunc_t Random::rand_function=0;
SeedFunc_t Random::seed_function=0;
//Random::seed_function=0;
/*
class Random{
 public:
  Random():rand_function(NULL),seed_function(NULL){}
  Random(RandomFunc_t _R,SeedFunc_t _S):rand_function(_R),seed_function(_S){}
  double next(){return (*rand_function)();}
  void seed(uint32_t _s){(*seed_function)(_s);}

  RandomFunc_t rand_function(){return rand_function;}
  void rand_function(RandomFunc_t _R){rand_function=_R;}

  SeedFunc_t seed_function(){return seed_function;}
  void seed_function(SeedFunc_t _S){seed_function=_S;}

 private:
  RandomFunc_t rand_function;
  SeedFunc_t seed_function;
}
*/
/*
double genRandomProbability()
{
   double r = (double) genrand_real1(); // this function is defined in mersene.h
   return r;
}

void seedRandom(unsigned long seed)
{
   init_genrand(seed);
}
*/
}//end namespace McOpteron
