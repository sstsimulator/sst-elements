#ifndef _KGDBG_H
#define _KGDBG_H

#include <iostream>

namespace kgdbg {

static inline void spinner( const char* id, bool cond = true ) {
  if( !std::getenv( id ) )
    return;
  if( !cond )
    return;
  unsigned long spinner = 1;
  if( spinner )
    std::cout << id << " spinning" << std::endl;
  while( spinner > 0 ) {
    spinner++;
    if( spinner % 100000000 == 0 )  // breakpoint here
      std::cout << ".";
  }
  std::cout << std::endl;
}
}  //namespace kgdbg

#endif  //_KGDBG_H
