#ifndef _ptlIF_h
#define _ptlIF_h

#include <unistd.h>
#include <stddef.h>
#include <cmdQueue.h>
#include <stdarg.h>
#include <portals4_types.h>

#include <m5_syscall.h>

namespace PtlNic {

class PtlIF {
  public:
    PtlIF( int jid, int uid ) :
        m_tailShadow(0),
        m_ctx_id( 101 ),
        m_nid(-1),
        m_meUnlinkedPos( 0 )
    {
        PTL_DBG2("\n");
        char* tmp = getenv("PTLNIC_CMD_QUEUE_ADDR");
        unsigned long addr = 0;
        if ( tmp )
            addr = strtoul(tmp,NULL,16);

        m_cmdQueue = (cmdQueue_t*)syscall( SYS_mmap_dev, addr, 
                                                sizeof( cmdQueue_t) );

        for ( int i = 0; i < ME_UNLINKED_SIZE; i++ ) {
            m_meUnlinked[i] = -1;
        }

        assert( m_cmdQueue );
        cmdUnion_t& cmd = getCmdSlot(ContextInit);
        cmd.ctxInit.uid = uid;
        cmd.ctxInit.jid = jid;
        cmd.ctxInit.nidPtr = (cmdAddr_t) &m_nid;
        cmd.ctxInit.limitsPtr = (cmdAddr_t) &m_limits;
        cmd.ctxInit.meUnlinkedPtr = (cmdAddr_t) &m_meUnlinked;
        commitCmd(); 

        while ( m_nid == -1 );

        PTL_DBG2("nid=%d\n",m_nid);
    }

    ~PtlIF() {
        PTL_DBG2("\n");

        cmdUnion_t& cmd = getCmdSlot(ContextFini);
        commitCmd();
    } 

    ptl_nid_t nid() { return m_nid; }

    ptl_ni_limits_t& limits() { return m_limits; }

    int popMeUnlinked() {
        int retval = m_meUnlinked[m_meUnlinkedPos]; 
        if ( retval != -1 ) {
            m_meUnlinked[m_meUnlinkedPos] = -1; 
            m_meUnlinkedPos = ( m_meUnlinkedPos + 1 ) % ME_UNLINKED_SIZE; 
        }
        return retval;
    }

  private:

    friend class PtlAPI;
    cmdUnion_t& getCmdSlot( int type )
    {
    //    PTL_DBG2("%s\n", m_cmdNames[type] );
        m_next = ( m_tailShadow + 1 ) % CMD_QUEUE_SIZE;
        while ( m_next == m_cmdQueue->head );

        cmdQueueEntry_t&  cmd = m_cmdQueue->queue[ m_tailShadow ]; 
        cmd.type = type;
        cmd.ctx_id = m_ctx_id; 
        
        return cmd.u; 
    }

    void commitCmd() {

        asm volatile( "":::"memory");
        //asm volatile("wmb");

        // moving the tail ptr tells the nic there is a new entry
        m_cmdQueue->tail = m_tailShadow = m_next;
    }
    
    cmdQueue_t* m_cmdQueue;    
    int         m_tailShadow;
    int         m_next;

    int                 m_ctx_id;
    ptl_pid_t           m_pid;
    ptl_ni_limits_t     m_limits;
    volatile ptl_nid_t  m_nid;
    int                 m_meUnlinked[ME_UNLINKED_SIZE]; 
    int                 m_meUnlinkedPos;

    static const char* m_cmdNames[];
};

const char* PtlIF::m_cmdNames[] = CMD_NAMES;

}

#endif
