#ifndef RANDOM_H
#define RANDOM_H
#include <stdint.h>
#include <functional>
namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library

//typedef double(*RandomFunc_t)();
typedef std::function<double ()> RandomFunc_t;
//typedef void(*SeedFunc_t)(uint32_t);
typedef std::function<void (uint32_t)> SeedFunc_t;
class Random{
 public:
  Random(){}
  Random(RandomFunc_t _R,SeedFunc_t _S){
    setrand_function(_R);
    setseed_function(_S);
  }
  double next(){return rand_function();}
  void seed(uint32_t _s){seed_function(_s);}

  RandomFunc_t getrand_function(){return rand_function;}
  void setrand_function(RandomFunc_t _R){rand_function=_R;}

  SeedFunc_t getseed_function(){return seed_function;}
  void setseed_function(SeedFunc_t _S){seed_function=_S;}

 private:
  static RandomFunc_t rand_function;
  static SeedFunc_t seed_function;
};

}//end namespace McOpteron
#endif
