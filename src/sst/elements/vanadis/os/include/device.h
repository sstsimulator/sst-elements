// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_DEVICE
#define _H_VANADIS_NODE_OS_INCLUDE_DEVICE

namespace SST {
namespace Vanadis {

namespace OS {

class Device {
  public:
    Device( std::string name, uint64_t addr, size_t length) : name(name), physAddr(addr), length(length)  
    { }
    ~Device() { }

    std::string getName() { return name; } 
    uint64_t getPhysAddr() { return physAddr; }
    size_t getLength() { return length; }
  private:
    std::string name;
    uint64_t physAddr;
    size_t length;
};

}
}
}

#endif
