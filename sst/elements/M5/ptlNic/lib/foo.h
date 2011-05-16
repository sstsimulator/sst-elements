#ifndef _foo_h
#define _foo_h

#include <unistd.h>
#include <stddef.h>
#include <cmdQueue.h>
#include <stdarg.h>
#include <portals4_types.h>

#define SYS_foo 443 

//static const char

class foo {
  public:
    foo( 
              unsigned int      options,
              ptl_pid_t         pid,
              ptl_ni_limits_t   *desired,
              ptl_ni_limits_t   *actual,
              ptl_size_t        map_size,
              ptl_process_t     *desired_mapping,
              ptl_process_t     *actual_mapping
    ) :
        m_tailShadow(0)
    {
        printf("%s()\n",__func__);
        char* tmp = getenv("PTLNIC_CMD_QUEUE_ADDR");
        printf("PTLNIC_QUEUE_ADDR `%s`\n",tmp);
        unsigned long addr = 0;
        if ( tmp )
            addr = strtoul(tmp,NULL,16);

        m_cmdQueue = (cmdQueue_t*)syscall( SYS_foo, addr, sizeof( cmdQueue_t) );
        //printf("ptr=%p\n",m_cmdQueue);

        push_cmd( PtlNIInit, 7, options, pid, desired,
                        actual, map_size, desired_mapping, actual_mapping ); 
        printf("%s():%d %p\n",__func__,__LINE__,m_cmdQueue);
    }

    void check()
    {
        printf("%s():%d %p\n",__func__,__LINE__,m_cmdQueue);
    }
    ~foo() {
        printf("%s()\n",__func__);
        push_cmd( PtlNIFini , 0 );
    } 

    int ptAlloc( unsigned int    options,
                 ptl_handle_eq_t eq_handle,
                 ptl_pt_index_t  pt_index_req,
                 ptl_pt_index_t *pt_index) 
    {                                     
        printf("%s()\n",__func__);
        printf("%s():%d %p\n",__func__,__LINE__,m_cmdQueue);
        return push_cmd( PtlPTAlloc, 4, options, 
                        eq_handle, pt_index_req, pt_index ); 
    }

    int ptFree( ptl_pt_index_t pt_index ) 
    {
        printf("%s()\n",__func__);
        return push_cmd( PtlPTFree, 1, pt_index ); 
    }

  private:

    int push_cmd( int cmd, int numArgs, ... ) 
    {
        va_list ap;

        printf("%s() %s\n", __func__, m_cmdNames[cmd]);
        int next = ( m_tailShadow + 1 ) % CMD_QUEUE_SIZE;
        while ( next == m_cmdQueue->head );

        cmdQueueEntry_t* entry = &m_cmdQueue->queue[ m_tailShadow ]; 

        va_start( ap, numArgs );
        for ( int i = 0; i < numArgs; i++ ) {
            unsigned long tmp = va_arg( ap, unsigned long );  
            //printf("arg%d %#lx\n", i, tmp );
            entry->arg[i] = tmp; 
        }
        va_end( ap );

        entry->cmd = cmd;

        asm volatile("wmb");

        // moving the tail ptr tells the nic there is a new entry
        m_cmdQueue->tail = m_tailShadow = next;

        // the nic clears this field to inform us that the retval is valid 
        while ( entry->cmd == cmd ); 

        return entry->retval;
    } 
    
    cmdQueue_t* m_cmdQueue;    
    int         m_tailShadow;
    static const char* m_cmdNames[];
};

const char* foo::m_cmdNames[] = CMD_NAMES;

#endif
