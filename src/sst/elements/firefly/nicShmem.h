// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
    Shmem( Nic& nic, Output& output ) : m_nic( nic ), m_dbg(output) 
    {}
    void regMem( NicShmemCmdEvent* event, int id );
    void put( NicShmemCmdEvent* event, int id );
    void get( NicShmemCmdEvent* event, int id );
    void fence( NicShmemCmdEvent* event, int id );

    std::pair<Hermes::MemAddr, size_t>& findRegion( uint64_t addr ) { 
        for ( int i = 0; i < m_regMem.size(); i++ ) {
            if ( addr >= m_regMem[i].first.getSimVAddr() &&
                addr < m_regMem[i].first.getSimVAddr() + m_regMem[i].second ) {
                return m_regMem[i]; 
            } 
        } 
        assert(0);
    }

  private:
    Nic& m_nic;
    Output& m_dbg;
    std::vector< std::pair<Hermes::MemAddr, size_t> > m_regMem;
};
