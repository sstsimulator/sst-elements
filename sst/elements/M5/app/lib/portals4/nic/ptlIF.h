#ifndef _ptlIF_h
#define _ptlIF_h

#include <unistd.h>
#include <stddef.h>
#include <cmdQueue.h>
#include <stdarg.h>
#include <portals4_types.h>

#define SYS_foo 443 

namespace PtlNic {

class PtlIF {
  public:
    PtlIF() :
        m_tailShadow(0)
    {
//        PTL_DBG2("\n");
        char* tmp = getenv("PTLNIC_CMD_QUEUE_ADDR");
        unsigned long addr = 0;
        if ( tmp )
            addr = strtoul(tmp,NULL,16);

        m_cmdQueue = (cmdQueue_t*)syscall( SYS_foo, addr, sizeof( cmdQueue_t) );

        assert( m_cmdQueue );
        ptl_jid_t jid = 1;
        m_context = push_cmd( ContextInit, 2, getuid(), jid );
        if ( m_context == -1 ) {
            abort();
        }
    }

    ~PtlIF() {
//        PTL_DBG2("\n");
        push_cmd( ContextFini , 0 );
    } 

  private:

    friend class PtlAPI;
    int push_cmd( int cmd, int numArgs, ... ) 
    {
        va_list ap;

        int next = ( m_tailShadow + 1 ) % CMD_QUEUE_SIZE;
        while ( next == m_cmdQueue->head );

        cmdQueueEntry_t* entry = &m_cmdQueue->queue[ m_tailShadow ]; 

        va_start( ap, numArgs );
        for ( int i = 0; i < numArgs; i++ ) {
            unsigned long tmp = va_arg( ap, unsigned long );  
            entry->args[i] = tmp; 
        }
        va_end( ap );

        entry->cmd = cmd;
        entry->context = m_context;

        asm volatile("wmb");

        // moving the tail ptr tells the nic there is a new entry
        m_cmdQueue->tail = m_tailShadow = next;

        // the nic clears this field to inform us that the retval is valid 
        while ( entry->cmd == cmd ); 

        return entry->retval;
    } 
    
    cmdQueue_t* m_cmdQueue;    
    int         m_tailShadow;
    int         m_context;
    static const char* m_cmdNames[];
};

const char* PtlIF::m_cmdNames[] = CMD_NAMES;

}

#endif
