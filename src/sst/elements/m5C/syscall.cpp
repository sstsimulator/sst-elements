// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <errno.h>

#include <system.h>

#include <termios.h>

#include <sim/syscall_emul.hh>
#include <barrier.h>
#include <trace.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <syscall.h>
#include <debug.h>
#include <paramHelp.h>
#include <m5.h>

#if THE_ISA == SPARC_ISA
    #define ISA_OS SparcLinux

    #include <byteswap.h>

    #include <arch/sparc/linux/linux.hh>

    #define __NR_exit                                1
    #define __NR_read                                3 
    #define __NR_write                               4 
    #define __NR_open                                5 

	#define isaSwap( x ) bswap_64(x)

#elif THE_ISA == ALPHA_ISA
    #define ISA_OS AlphaLinux
    #include <arch/alpha/linux/linux.hh>

    #define __NR_open                                45 
    #define __NR_read                                3 
    #define __NR_write                               4 
    #define __NR_exit                                1
    #define __NR_fstat                               91 
    #define __NR_fstat64                             427
    #define __NR_ioctl                               54

	#define isaSwap(x) x

#elif THE_ISA == X86_ISA
    #define ISA_OS X86Linux64
    #include <arch/x86/linux/linux.hh>

    #define __NR_open                                2 
    #define __NR_read                                0 
    #define __NR_write                               1 
    #define __NR_exit                                60 
    #define __NR_fstat                               108
    #define __NR_fstat64                             197 
    #define __NR_ioctl                               54

	#define isaSwap(x) x

#else
    #error What ISA
#endif

using namespace SST::M5;

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
    m_barrierHandler( BarrierAction::Handler<Syscall>(this,
                                        &Syscall::barrierReturn) ),
    m_dmaEvent( this ),
    m_syscallEvent( this ),
    m_startAddr( p->startAddr ),
    m_comp( p->m5Comp )
{
    m_endAddr = m_startAddr + sizeof(m_mailbox);
    DBGX(3,"startAddr=%#lx endAddr=%#lx\n", m_startAddr, m_endAddr);
    memset( m_mailbox, 0, sizeof(m_mailbox) );

#if 0
    m_comp->barrier().add( &m_barrierHandler );
#endif
}    

Syscall::~Syscall()
{
}

void Syscall::process(void)
{
    DBGX(3,"\n");
    finishSyscall();
}

void Syscall::addressRanges(AddrRangeList& resp)
{
    DBGX(3," m_startAddr=%#lx m_endAddr=%#lx\n",m_startAddr,m_endAddr);
    resp.clear();
    resp.push_back( RangeSize( m_startAddr, sizeof(m_mailbox) ));
}

Tick Syscall::write(Packet* pkt)
{
    DBGX(4,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->writeData( (uint8_t*) m_mailbox + pkt->getAddr() - m_startAddr  );
    pkt->makeTimingResponse();

    if ( pkt->getAddr() - m_startAddr == 0xf * sizeof( uint64_t ) ) {
        schedule( m_syscallEvent, curTick() + 0 );
    }

    return 1;
}

Tick Syscall::read(Packet* pkt)
{
    DBGX(4,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->setData( ( uint8_t*) m_mailbox + pkt->getAddr() - m_startAddr );
    pkt->makeTimingResponse();
    return 1;
}

int64_t Syscall::startFstat( int fd, Addr bufAddr )
{
    DBGX(3, "fd=%d buf=%#lx\n", fd, bufAddr ); 

    struct stat host_stat;
    int64_t retval = ::fstat( fd, &host_stat );
    if ( retval == -1 ) {
        return -errno;
    }

    typedef ISA_OS::tgt_stat tgt_stat_t; 

    m_dmaEvent.buf = (uint8_t*) malloc( sizeof( tgt_stat_t ) );
    assert( m_dmaEvent.buf );

    ISA_OS::tgt_stat*  tgt_stat = (tgt_stat_t*) m_dmaEvent.buf ;

    convertStatBuf< tgt_stat_t*, struct stat>( tgt_stat, &host_stat, (fd==1));

    dmaWrite( bufAddr, sizeof(*tgt_stat), &m_dmaEvent, m_dmaEvent.buf, 0 );

    return retval;
}

int64_t Syscall::startFstat64( int fd, Addr bufAddr )
{
    DBGX(3, "fd=%d buf=%#lx\n", fd, bufAddr ); 
    assert(0);
    
    struct stat64 buf;
    int64_t retval = ::fstat64( fd, &buf );
    if ( retval == -1 ) {
        return -errno;
    }
    return retval;
}

void Syscall::finishFstat()
{
    DBGX(3, "\n" ); 
    free( m_dmaEvent.buf );
}

#if 1
int64_t Syscall::startIoctl( int fd, int request, Addr buf )
{
    DBGX(3, "fd=%d request=%#x buf=%#lx\n", fd, request ,buf ); 
    
    assert( (unsigned int)request == ISA_OS::TCGETS_ );

    return -ENOTTY;

#if 0
    struct termios host_termios;
    uint64_t retval = ::ioctl( fd, TCGETS, &host_termios );
    if ( retval == -1 ) {
        return -errno;
    }

    assert( m_dmaEvent.buf );

    dmaWrite( buf, sizeof( struct stat ), &m_dmaEvent, m_dmaEvent.buf, 0 );
#endif
}
#endif

void Syscall::finishIoctl()
{
    DBGX(3, "\n" ); 
    free( m_dmaEvent.buf );
}

void Syscall::startOpen( Addr path )
{
    DBGX(3, "path=%#lx\n", path ); 

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
        DBGX(3, "path=`%s` flags=%#x mode=%#x\n", path, hostFlags, mode );
        retval = ::open( path, hostFlags, mode );
    } else {
        DBGX(3, "path=`%s` flags=%#x\n", path, hostFlags );
        retval = ::open( path, hostFlags );
    }

    free( m_dmaEvent.buf );

    if ( retval == -1 ) {
        retval = -errno;
    } 

    DBGX(3,"retval=%d\n",retval);

    return retval;
}

int64_t Syscall::startRead( int fildes, Addr addr, size_t nbytes )
{
    DBGX(3, "fd=%d buf=%#lx nbytes=%lu\n", fildes, addr, nbytes );

    if ( nbytes == 0 ) return  0;

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );

    memset(m_dmaEvent.buf,0,nbytes);

    assert( m_dmaEvent.buf );

    m_dmaEvent.retval = ::read( fildes, m_dmaEvent.buf, nbytes );

    //printf("%s\n",m_dmaEvent.buf);

    DBGX(3, "retval=%lu\n", m_dmaEvent.retval );

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
    DBGX(3, "fd=%d nbytes=%lu\n", fildes, nbytes );
    
    free( m_dmaEvent.buf );

    return m_dmaEvent.retval; 
}

int64_t Syscall::startWrite( int fildes, Addr addr, size_t nbytes )
{
    DBGX(3, "fd=%d buf=%#lx nbytes=%lu\n", fildes, addr, nbytes );

    if ( nbytes == 0 ) return 0;

    m_dmaEvent.buf = (uint8_t*) malloc( nbytes );

    assert( m_dmaEvent.buf );

    dmaRead( addr, nbytes, &m_dmaEvent, m_dmaEvent.buf, 0 );

    return nbytes;
}

int64_t Syscall::finishWrite( int fildes, size_t nbytes )
{
    const void* buf = (const void*) m_dmaEvent.buf;

    DBGX( 3, "fd=%d nbytes=%lu\n", fildes, nbytes );

    time_t _time = time(NULL);

//    struct tm* tmp = localtime(&_time);
     
    SST::Simulation *sim = SST::Simulation::getSimulation();

    char _buf[100];
    if ( fildes == 1 ) {
        ::sprintf(_buf,"%lu:%lu <cout> ", sim->getCurrentSimCycle(), _time );
    } else if ( fildes == 2 ) {
        ::sprintf(_buf,"%lu:%lu <cerr> ", sim->getCurrentSimCycle(), _time );
    }
    ::write( fildes, _buf, strlen(_buf));
    int64_t retval = ::write( fildes, buf, nbytes ); 

    DBGX( 3, "retval=%d\n", retval );

    if ( retval == -1 ) {
        retval = -errno; 
    }

    free( m_dmaEvent.buf );

    return retval; 
}

void Syscall::barrierReturn()
{
    DBGX(3,"\n");
    foo( 0 );
}

void Syscall::startSyscall(void)
{
    DBGX(3,"%d\n", isaSwap(m_mailbox[0xf]) - 1 );
    int64_t retval = 0;
    switch ( isaSwap( m_mailbox[0xf] ) - 1 ) 
    {
        case __NR_exit:
            m_comp->exit( isaSwap( m_mailbox[0] ) );
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

        case __NR_ioctl:
            retval = startIoctl( m_mailbox[0], m_mailbox[1], m_mailbox[2] );
            if ( retval < 0 ) foo( retval ); 
            break;

        case __NR_fstat:
        case __NR_fstat64:
            retval = startFstat( m_mailbox[1], m_mailbox[2] );
            if ( retval < 0 ) foo( retval ); 
            break;

#if 0
        case __NR_barrier:
            m_comp->barrier().enter();
            break; 
#endif

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
    DBGX(3,"%d\n", m_mailbox[0xf] - 1 );
    int64_t retval = 0;
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

        case __NR_fstat:
        case __NR_fstat64:
            finishFstat( );
            break;

        case __NR_ioctl:
            finishIoctl( );
            break;
    }
    foo ( retval );
}

const char* Syscall::syscallStr( int num )
{
    switch( num ) {
        case __NR_open: return "open";
        case __NR_read: return "read";
        case __NR_write: return "write";
        case __NR_exit: return "exit";
        case __NR_fstat: return "fstat";
        case __NR_fstat64: return "fstat64";
        case __NR_ioctl: return "ioctl";
        default:
            return "?????";
    }
}
