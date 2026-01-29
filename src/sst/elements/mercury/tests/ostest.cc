// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#define ssthg_app_name ostest
#include <iostream>
#include <mercury/common/skeleton.h>
using namespace SST::Hg;

int main(int argc, char** argv) {
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i] << "\n";
  }

  std::cout << "Hello from Mercury!\n";
  std::cout << "Now I will sleep\n";
  ssthg_sleep(5);
  std::cout << "I'm back!\n";
  std::cout << "Bye!\n";
  return 0;
}
