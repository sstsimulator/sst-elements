// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class Shmem {
  public:
    Shmem( Output& output ) : m_dbg( output) {}
    void init( std::function<std::pair<Hermes::MemAddr,size_t>(int,uint64_t)> func ) {
        m_findRegMem = func;
    }
  private:
    Output& m_dbg;
    std::function<std::pair<Hermes::MemAddr,size_t>(int,uint64_t)> m_findRegMem;
};
