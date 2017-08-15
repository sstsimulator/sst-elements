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


    class Op {
      public:
        typedef std::function<void()> Callback;
        enum Type { Wait } m_type;  
        Op( Type type, NicShmemOpCmdEvent* cmd, Callback callback ) : m_type(type), m_cmd(cmd), m_callback(callback) {}
        virtual ~Op() { 
            m_callback();
            delete m_cmd;
        }
        virtual bool checkOp( ) = 0;
        bool inRange( Hermes::Vaddr addr, size_t length ) {
            //printf("%s() addr=%lu length=%lu\n",__func__,addr, length);
            return ( m_cmd->addr >= addr && m_cmd->addr + m_cmd->value.getLength() <= addr + length );
        }

      protected:
        NicShmemOpCmdEvent* m_cmd;
        Callback            m_callback;
        Hermes::Value       m_value;   
    };

    class WaitOp : public Op {
      public:
        WaitOp( NicShmemOpCmdEvent* cmd, void* backing, Callback callback ) : 
            Op( Wait, cmd, callback ), 
            m_value( cmd->value.getType(), backing ) 
        {} 

        bool checkOp() {
#if 0
            std::stringstream tmp;
            tmp << m_value << " " << m_cmd->op << " " << m_cmd->value;
            printf("%s %s\n",__func__,tmp.str().c_str());
#endif
            switch ( m_cmd->op ) {
              case Hermes::Shmem::NE:
                return m_value != m_cmd->value; 
                break;
              case Hermes::Shmem::GTE:
                return m_value >= m_cmd->value; 
                break;
              case Hermes::Shmem::GT:
                return m_value > m_cmd->value; 
                break;
              case Hermes::Shmem::EQ:
                return m_value == m_cmd->value; 
                break;
              case Hermes::Shmem::LT:
                return m_value < m_cmd->value;
                break;
              case Hermes::Shmem::LTE:
                return m_value <= m_cmd->value;
                break;
              default:
                assert(0);
            } 
            return false;
        } 
      private:
        Hermes::Value m_value;
    }; 

  public:
    Shmem( Nic& nic, Output& output ) : m_nic( nic ), m_dbg(output) 
    {}
    ~Shmem() {
        m_regMem.clear();
    }

    void regMem( NicShmemRegMemCmdEvent*, int id );
    void wait( NicShmemOpCmdEvent*, int id );
    void put( NicShmemPutCmdEvent*, int id );
    void putv( NicShmemPutvCmdEvent*, int id );
    void get( NicShmemGetCmdEvent*, int id );
    void getv( NicShmemGetvCmdEvent*, int id );
    void fadd( NicShmemFaddCmdEvent*, int id );
    void cswap( NicShmemCswapCmdEvent*, int id );
    void swap( NicShmemSwapCmdEvent*, int id );
    void fence( NicShmemCmdEvent*, int id );

    void* getBacking( Hermes::Vaddr addr, size_t length ) {
        return  m_nic.findShmem( addr, length ).getBacking();
    }
    void checkWaitOps( Hermes::Vaddr addr, size_t length );

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
    std::list<Op*> m_pendingOps;
    std::vector< std::pair<Hermes::MemAddr, size_t> > m_regMem;
};
