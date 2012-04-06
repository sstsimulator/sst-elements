#ifndef _ptlIF_h
#define _ptlIF_h

#include <limits.h>
#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <cmdQueue.h>
#include <portals4.h>


namespace PtlNic {

class PtlIF {
  public:
    PtlIF( int jid, int uid ) :
        m_ctx_id( getpid() ),
        m_nid((unsigned)-1),
        m_meUnlinkedPos( 0 )
    {
        int fd;
#if 0
        int ret = mlockall( MCL_CURRENT | MCL_FUTURE );
        assert ( ret == 0 );
#endif

        char* filename = getenv( "P4_DEVICE" );
        assert( filename );
        PTL_DBG2("P4 device file `%s`\n", filename );
        if ( ( fd=open( filename, O_RDWR ) ) < 0 )
        {
            fprintf(stderr,"open %s failed %s", filename, strerror(errno) );
            exit(-1);
        }
        assert( fd > -1 );

        assert( sizeof(cmdQueue_t) <= 4096 );
        m_cmdQueue = (cmdQueue_t*) mmap(0, 4096, 
                    PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        assert( m_cmdQueue != (void*) -1 );

        m_tailShadow = m_cmdQueue->tail;
        PTL_DBG2("m_cmdQueue=%p\n",m_cmdQueue);
        PTL_DBG2("&m_nid=%p size=%lu\n", &m_nid, sizeof(m_nid));
        PTL_DBG2("&m_limits=%p size=%lu\n", &m_limits, sizeof(m_limits));
        for ( int i = 0; i < ME_UNLINKED_SIZE; i++ ) {
            m_meUnlinked[i] = -1;
        }

        assert( m_cmdQueue );
        cmdUnion_t& cmd = getCmdSlot(ContextInitCmd);
        cmd.ctxInit.uid = uid;
        cmd.ctxInit.nidPtr = (cmdAddr_t) &m_nid;
        cmd.ctxInit.limitsPtr = (cmdAddr_t) &m_limits;
        cmd.ctxInit.meUnlinkedPtr = (cmdAddr_t) &m_meUnlinked;
        commitCmd(); 

        while ( m_nid == (unsigned) -1 );

        PTL_DBG2("nid=%d\n",m_nid);
    }

    ~PtlIF() {
        PTL_DBG2("\n");

        getCmdSlot(ContextFiniCmd);
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

  private:
    
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
