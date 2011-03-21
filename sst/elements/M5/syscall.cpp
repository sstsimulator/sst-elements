#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <errno.h>

using namespace SST;
class Component;

#include <fcntl.h>
#include <syscall.h>
#include <debug.h>

#define __NR_read                                0
//#define __NR_xxxxx                               5
#define __NR_open                                2
#define __NR_close                               3
#define __NR_write                               4 
#if 0
#endif

//static const char* syscallNames[] = { "read", "write", "open", "close" };
static const char* syscallNames[] = { "read", "write", "open", "close", "write" };

#define LINUX_O_CREAT 0x40
#define LINUX_O_RDWR 0x2
#define LINUX_O_RDONLY 0x0
#define LINUX_O_WRONLY 0x1

#define LINUX_O_ALL (LINUX_O_CREAT |\
                     LINUX_O_RDWR |\
                     LINUX_O_RDONLY |\
                     LINUX_O_WRONLY)

extern "C" {
SimObject* create_Syscall( Component*, string name, Params& sstParams );
}

SimObject* create_Syscall( Component*, string name, Params& sstParams )
{
    Syscall::Params* memP   = new Syscall::Params;

    memP->name = name;
#if 0
    memP->range.start = sstParams.find_integer( "physicalMemory.start" );;
    memP->range.end = sstParams.find_integer( "physicalMemory.end", 0 ); 

    DBGC( 1, "%s.physicalMemory.start %#lx\n",name.c_str(), memP->range.start);
    DBGC( 1, "%s.physicalMemory.end %#lx\n",name.c_str(),memP->range.end);
#endif

    return new Syscall( memP );
}

Syscall::Syscall( const Params* p ) :
    DmaDevice( p ),
    m_offset( 0 ),
    m_dmaEvent( this ),
    m_syscallEvent( this )
{
    DBGX(2,"\n");
    memset( m_mailbox, 0, sizeof(m_mailbox) );
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
    int start = 0x0;
    int end = 0x1000 - start;
    resp.push_back( RangeSize(start, end));
}

Tick Syscall::write(Packet* pkt)
{
    DBGX(5,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    pkt->writeData( (uint8_t*) m_mailbox + pkt->getAddr() - m_offset  );
    pkt->makeAtomicResponse();

    if ( pkt->getAddr() - m_offset == 0xf * sizeof( uint64_t ) ) {
        schedule( m_syscallEvent, curTick + 1 );
    }

    return 5000;
}

Tick Syscall::read(Packet* pkt)
{
    DBGX(5,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    pkt->setData( ( uint8_t*) m_mailbox + pkt->getAddr() - m_offset );
    pkt->makeAtomicResponse();
    return 5000;
}

int64_t Syscall::startOpen( Addr path, int oflag, mode_t mode )
{
    DBGX(2, "path=%#lx flags=%#x mode=%x\n", path, oflag, mode ); 

    if ( oflag & ~LINUX_O_ALL ) {
        return -EINVAL;
    } else {
        m_dmaEvent.buf = (uint8_t*) malloc( FILENAME_MAX );
        DBGX(2,"buf=%p\n", m_dmaEvent.buf );
        assert( m_dmaEvent.buf );
        dmaRead( path, FILENAME_MAX, &m_dmaEvent, m_dmaEvent.buf, 5000 );
    }
    return 0; 
}

int64_t Syscall::finishOpen( int linux_oflag, mode_t mode )
{
    const char* path = (const char*) m_dmaEvent.buf;
    int oflag = 0;

    if ( linux_oflag & LINUX_O_CREAT )  oflag |= O_CREAT;
    if ( linux_oflag & LINUX_O_RDWR )   oflag |= O_RDWR;
    if ( linux_oflag & LINUX_O_RDONLY ) oflag |= O_RDONLY;
    if ( linux_oflag & LINUX_O_WRONLY ) oflag |= O_WRONLY;

    int64_t  retval;
    if ( oflag & O_CREAT ) {
        DBGX(2, "path=`%s` flags=%#x mode=%#x\n", path, oflag, mode );
        retval = ::open( path, oflag, mode );
    } else {
        DBGX(2, "path=`%s` flags=%#x\n", path, oflag );
        retval = ::open( path, oflag );
    }

        DBGX(2,"buf=%p\n", m_dmaEvent.buf );
    //free( m_dmaEvent.buf );

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

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );
        DBGX(2,"buf=%p\n", m_dmaEvent.buf );

    assert( m_dmaEvent.buf );

    int64_t retval = ::read( fildes, m_dmaEvent.buf, nbytes );

    DBGX(2, "retval=%lu\n", retval );

    if ( retval == -1 ) {
        DBGX(2,"buf=%p\n", m_dmaEvent.buf );
        //free( m_dmaEvent.buf );
        return -errno;
    } else {
        dmaWrite( addr, retval, &m_dmaEvent, m_dmaEvent.buf, 5000 );
        m_dmaEvent.retval = retval;
    }
    return 0;
}

int64_t Syscall::finishRead( int fildes, size_t nbytes )
{
    DBGX(2, "fd=%d nbytes=%lu\n", fildes, nbytes );
    
        DBGX(2,"buf=%p\n", m_dmaEvent.buf );
    //free( m_dmaEvent.buf );

    return m_dmaEvent.retval; 
}

int64_t Syscall::startWrite( int fildes, Addr addr, size_t nbytes )
{
    DBGX(2, "fd=%d buf=%#lx nbytes=%lu\n", fildes, addr, nbytes );

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );
        DBGX(2,"buf=%p\n", m_dmaEvent.buf );

    assert( m_dmaEvent.buf );

    dmaRead( addr, nbytes, &m_dmaEvent, m_dmaEvent.buf, 5000 );

    return 0;
}

int64_t Syscall::finishWrite( int fildes, size_t nbytes )
{
    const void* buf = (const void*) m_dmaEvent.buf;

    DBGX(2, "fd=%d nbytes=%lu\n", fildes, nbytes );
    
    //int64_t retval = ::write( fildes, buf, nbytes ); 
    int64_t retval = ::write( 2, buf, nbytes ); 

    if ( retval == -1 ) {
        retval = -errno; 
    }

        DBGX(2,"buf=%p\n", m_dmaEvent.buf );
    //free( m_dmaEvent.buf );

    DBGX(2,"retval=%d\n",retval);

    return retval; 
}

void Syscall::startSyscall(void)
{
    DBGX(2,"%s\n", syscallNames[ m_mailbox[0xf] - 1 ] );
    int64_t retval = 0;
    switch ( m_mailbox[0xf] - 1 ) 
    {
        case __NR_write:
            retval = startWrite( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            break; 
    
        case __NR_read:
            //retval = startRead( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            foo( 0xf );
            break; 
    
        case __NR_open:
            retval = startOpen( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            break; 

        case __NR_close:
            foo( close( m_mailbox[0] ) );
            break; 
    
         default:
            for ( int i = 0; i < 0x10; i++ ) {
                fprintf(stderr,"Syscall::do_syscall arg%d %#lx\n", 
                            i, (unsigned long) m_mailbox[i]);
            }
            abort();
    }
    if ( retval < 0 ) {
        foo( retval );
    }
}

void Syscall::finishSyscall(void)
{
    DBGX(2,"%s\n", syscallNames[ m_mailbox[0xf] - 1 ] );
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
