#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <errno.h>

#include <system.h>

#include <barrier.h>

#include <fcntl.h>
#include <syscall.h>
#include <debug.h>
#include <arch/isa_specific.hh>
#include <paramHelp.h>
#include <m5.h>

#if THE_ISA == SPARC_ISA
    #define ISA_OS SparcLinux
    #include <arch/sparc/linux/linux.hh>
#elif THE_ISA == ALPHA_ISA
    #define ISA_OS AlphaLinux
    #include <arch/alpha/linux/linux.hh>
#elif THE_ISA == X86_ISA
    #define ISA_OS X86Linux64
    #include <arch/x86/linux/linux.hh>
#else
    #error What ISA
#endif

#define __NR_open                                45 
#define __NR_close                               6 
#define __NR_read                                3 
#define __NR_write                               4 
#define __NR_exit                                1

#define __NR_barrier                             500

extern "C" {
SimObject* create_Syscall( SST::Component*, string name,
                                                SST::Params& sstParams );
}

SimObject* create_Syscall( SST::Component* comp, string name, 
                                                SST::Params& sstParams )
{
    Syscall::Params& memP   = *new Syscall::Params;

    memP.name = name;

    INIT_HEX( memP, sstParams, startAddr );

    memP.system = create_System( "syscall", NULL, Enums::timing ); 

    memP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );

    return new Syscall( &memP );
}

Syscall::Syscall( const Params* p ) :
    DmaDevice( p ),
    m_dmaEvent( this ),
    m_syscallEvent( this ),
    m_startAddr( p->startAddr ),
    m_barrierHandler( SST::Event::Handler<Syscall>(this,
                                        &Syscall::barrierReturn) ),
    m_comp( p->m5Comp )
{
    m_endAddr = m_startAddr + sizeof(m_mailbox);
    DBGX(2,"startAddr=%#lx endAddr=%#lx\n", m_startAddr, m_endAddr);
    memset( m_mailbox, 0, sizeof(m_mailbox) );

    m_comp->barrier().add( &m_barrierHandler );
}    

Syscall::~Syscall()
{
}

void Syscall::process(void)
{
    DBGX(2,"\n");
    finishSyscall();
}

void Syscall::addressRanges(AddrRangeList& resp)
{
    DBGX(2,"\n");
    resp.clear();
    resp.push_back( RangeSize( m_startAddr, m_endAddr ));
}

Tick Syscall::write(Packet* pkt)
{
    DBGX(5,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->writeData( (uint8_t*) m_mailbox + pkt->getAddr() - m_startAddr  );
    pkt->makeTimingResponse();

    if ( pkt->getAddr() - m_startAddr == 0xf * sizeof( uint64_t ) ) {
        schedule( m_syscallEvent, curTick + 0 );
    }

    return 1;
}

Tick Syscall::read(Packet* pkt)
{
    DBGX(5,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->setData( ( uint8_t*) m_mailbox + pkt->getAddr() - m_startAddr );
    pkt->makeTimingResponse();
    return 1;
}

void Syscall::startOpen( Addr path )
{
    DBGX(2, "path=%#lx\n", path ); 

    m_dmaEvent.buf = (uint8_t*) malloc( FILENAME_MAX );
    assert( m_dmaEvent.buf );
    dmaRead( path, FILENAME_MAX, &m_dmaEvent, m_dmaEvent.buf, 0 );
}

int64_t Syscall::finishOpen( int oflag, mode_t mode )
{
    const char* path = (const char*) m_dmaEvent.buf;

    int hostFlags = 0;

    // translate open flags
    for (int i = 0; i < ISA_OS::NUM_OPEN_FLAGS; i++) {
        if (oflag & ISA_OS::openFlagTable[i].tgtFlag) {
            oflag &= ~ISA_OS::openFlagTable[i].tgtFlag;
            hostFlags |= ISA_OS::openFlagTable[i].hostFlag;
        }
    }

    int64_t  retval;
    if ( hostFlags & O_CREAT ) {
        DBGX(2, "path=`%s` flags=%#x mode=%#x\n", path, hostFlags, mode );
        retval = ::open( path, hostFlags, mode );
    } else {
        DBGX(2, "path=`%s` flags=%#x\n", path, hostFlags );
        retval = ::open( path, hostFlags );
    }

    free( m_dmaEvent.buf );

    if ( retval == -1 ) {
        retval = -errno;
    } 

    DBGX(2,"retval=%d\n",retval);

    return retval;
}

int64_t Syscall::close( int fd )
{
    DBGX(2, "fd=%d\n", fd );
    return ::close( fd );
}

int64_t Syscall::startRead( int fildes, Addr addr, size_t nbytes )
{
    DBGX(2, "fd=%d buf=%#lx nbytes=%lu\n", fildes, addr, nbytes );

    if ( nbytes == 0 ) return  0;

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );

    memset(m_dmaEvent.buf,0,nbytes);

    assert( m_dmaEvent.buf );

    m_dmaEvent.retval = ::read( fildes, m_dmaEvent.buf, nbytes );

    //printf("%s\n",m_dmaEvent.buf);

    DBGX(2, "retval=%lu\n", m_dmaEvent.retval );

    if ( m_dmaEvent.retval == -1 ) {
        free( m_dmaEvent.buf );
        return -errno;
    } else {
        if ( m_dmaEvent.retval > 0 ) {
            dmaWrite( addr, m_dmaEvent.retval, &m_dmaEvent, m_dmaEvent.buf, 0 );
        }
    }
    return m_dmaEvent.retval;
}

int64_t Syscall::finishRead( int fildes, size_t nbytes )
{
    DBGX(2, "fd=%d nbytes=%lu\n", fildes, nbytes );
    
    free( m_dmaEvent.buf );

    return m_dmaEvent.retval; 
}

int64_t Syscall::startWrite( int fildes, Addr addr, size_t nbytes )
{
    DBGX(2, "fd=%d buf=%#lx nbytes=%lu\n", fildes, addr, nbytes );

    if ( nbytes == 0 ) return 0;

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );

    assert( m_dmaEvent.buf );

    dmaRead( addr, nbytes, &m_dmaEvent, m_dmaEvent.buf, 0 );

    return nbytes;
}

int64_t Syscall::finishWrite( int fildes, size_t nbytes )
{
    const void* buf = (const void*) m_dmaEvent.buf;

    DBGX( 2, "fd=%d nbytes=%lu\n", fildes, nbytes );
    
    int64_t retval = ::write( fildes, buf, nbytes ); 

    DBGX( 2, "retval=%d\n", retval );

    if ( retval == -1 ) {
        retval = -errno; 
    }

    free( m_dmaEvent.buf );

    return retval; 
}

void Syscall::barrierReturn( SST::Event* )
{
    DBGX(2,"\n");
    foo( 0 );
}

void Syscall::startSyscall(void)
{
    DBGX(2,"%d\n", m_mailbox[0xf] - 1 );
    int64_t retval = 0;
    switch ( m_mailbox[0xf] - 1 ) 
    {
        case __NR_exit:
            m_comp->exit( m_mailbox[0] );
            break; 

        case __NR_write:
            retval = startWrite( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            if ( retval <= 0 ) foo( retval ); 
            break; 
    
        case __NR_read:
            retval = startRead( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            if ( retval <= 0 ) foo( retval ); 
            break; 
    
        case __NR_open:
            startOpen( m_mailbox[0] );
            break; 

        case __NR_close:
            foo( close( m_mailbox[0] ) );
            break; 

        case __NR_barrier:
            m_comp->barrier().enter();
            break; 

         default:
            for ( int i = 0; i < 0x10; i++ ) {
                fprintf(stderr,"Syscall::do_syscall arg%d %#lx\n", 
                            i, (unsigned long) m_mailbox[i]);
            }
            abort();
    }
}

void Syscall::finishSyscall(void)
{
    DBGX(2,"%d\n", m_mailbox[0xf] - 1 );
    int64_t retval;
    switch ( m_mailbox[0xf] - 1 ) 
    {
        case __NR_write:
            retval = finishWrite( m_mailbox[0], m_mailbox[2] );
            break; 

        case __NR_read:
            retval = finishRead( m_mailbox[0], m_mailbox[2] );
            break; 

        case __NR_open:
            retval = finishOpen( m_mailbox[1], m_mailbox[2] );
            break; 
    }
    foo ( retval );
}
