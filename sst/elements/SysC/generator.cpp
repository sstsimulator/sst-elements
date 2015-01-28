#include "generator.h"

void Generator::generateOutput(){
  sc_out_.write(count);
  native_out_.write(count);
  count++;
}
