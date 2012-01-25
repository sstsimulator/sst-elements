// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _PALACIOS_H
#define _PALACIOS_H

#include <signal.h>
#include <string>
#include <vector>
#include <assert.h>
#include <pthread.h>
#include "v3_user_host_dev.h"

#include <stdint.h>

class WriteFunctorBase {
public:
    virtual void operator()( unsigned long ) = 0;
};

template <class XX> class WriteFunctor : public WriteFunctorBase
{
public:
    WriteFunctor( XX* pt2object, void(XX::*fpt)( unsigned long ) ) {
        m_pt2object = pt2object; 
        m_fpt = fpt;
    }
    virtual void operator()(unsigned long addr ) {
        (*m_pt2object.*m_fpt)(addr);
    } 

private:
    void (XX::*m_fpt)(unsigned long);
    XX* m_pt2object;
};

class PalaciosIF {
public:
    PalaciosIF( std::string vm, std::string dev, 
                                uint8_t* backing,
                                uint64_t backingLen,
                                uint64_t addr,
                                WriteFunctorBase& writeFunc ) :
        m_threadRun( true ),
        m_backing( backing ),
        m_backingLen( backingLen ),
        m_backingAddr( addr ),
        m_writeFunc( writeFunc )
    {
        int ret;
        printf("%s() this=%p\n", __func__, this ); 
        printf("%s() vm=%s dev=%s\n", __func__, vm.c_str(), dev.c_str() );

        m_fd = v3_user_host_dev_rendezvous( vm.c_str(), dev.c_str() );
        assert( m_fd != -1 );
       
printf("MAIN %d\n",getpid());
        ret = pthread_create( &m_thread, NULL, thread1, this ); 
        assert( ret == 0 );
        printf("%s():%d\n",__func__,__LINE__);
    }
    ~PalaciosIF(){
        int ret;
        printf("%s():%d\n",__func__,__LINE__);
        
        m_threadRun = false;

        ret = pthread_join( m_thread, NULL );
        assert( ret == 0 );

        ret = v3_user_host_dev_depart( m_fd );
        assert( ret == 0 );
        printf("%s():%d\n",__func__,__LINE__);
    }


    inline uint64_t writeGuestMemVirt( uint64_t gva, void* data, int count );
    inline uint64_t readGuestMemVirt( uint64_t gva, void* data, int count );

private:
    static void* thread1( void* );
    void* thread2();
    int do_work(
        struct palacios_host_dev_host_request_response *req,
        struct palacios_host_dev_host_request_response **resp);

    inline char* getType( int type );
    inline void printData( uint64_t offset, void* data, int count );
    inline void readMem( uint64_t gpa, void* data, int count );
    inline void writeMem( uint64_t gpa, void* data, int count );
    inline uint32_t virt2phys( uint64_t );

    int                 m_fd;
    pthread_t           m_thread;
    bool                m_threadRun;
    uint8_t*            m_backing;
    uint64_t            m_backingLen;
    uint64_t            m_backingAddr;
    WriteFunctorBase&   m_writeFunc;
};

void PalaciosIF::printData( uint64_t offset, void* data, int count ) {
    switch (count) {
        case 1:
            printf("%s() offset=%#lx %#010x\n", __func__, offset,  
                                        *((uint8_t*)data) );
            break;
        case 2:
            printf("%s() offset=%#lx %#010x\n", __func__, offset,  
                                        *((uint16_t*)data) );
            break;
        case 4:
            printf("%s() offset=%#lx %#010x\n", __func__, offset,  
                                        *((uint32_t*)data) );
            break;
        case 8:
            printf("%s() offset=%#lx %#018lx\n", __func__, offset,
                                        *((uint64_t*)data) );
            break;
        default:
            printf("PalaciosIF::printData() count=%d ???????\n",count);
    }
}

void PalaciosIF::readMem( uint64_t gpa, void* data, int count )
{   
    uint64_t offset = gpa - m_backingAddr;
    if ( offset >= m_backingLen ) {
        printf("Warn: %s() offset %#lx > m_backingLen %#x\n",__func__, 
                                                offset, m_backingLen );
        return; 
    }
    memcpy( data, m_backing + offset, count );
    
    printData(offset, data, count );
}


void PalaciosIF::writeMem( uint64_t gpa, void* data, int count )
{   
    uint64_t offset = gpa - m_backingAddr;

    if ( offset >= m_backingLen ) {
        printf("Warn: %s() offset %#lx > m_backingLen %#x\n",__func__, 
                                                offset, m_backingLen );
        return; 
    }

    memcpy( m_backing + offset, data, count );
    
    printData(offset, data, count );
    
    (m_writeFunc)( offset );
}

uint32_t PalaciosIF::virt2phys( uint64_t gva ) {
    return v3_user_host_dev_virt2phys( m_fd, (void*) gva );
}

uint64_t  PalaciosIF::writeGuestMemVirt( uint64_t gva, void* data, int count )
{
    return v3_user_host_dev_write_guest_mem( m_fd, 
                    (void*) virt2phys(gva), data, count );
}

uint64_t  PalaciosIF::readGuestMemVirt( uint64_t gva, void* data, int count )
{
    return v3_user_host_dev_read_guest_mem( m_fd, 
                    (void*) virt2phys(gva), data, count );
}

char* PalaciosIF::getType( int type )
{           
    switch(type) {
        case PALACIOS_HOST_DEV_HOST_REQUEST_READ_IO:
            return "READ_IO";
        case PALACIOS_HOST_DEV_HOST_REQUEST_WRITE_IO:
            return "WRITE_IO";
        case PALACIOS_HOST_DEV_HOST_REQUEST_READ_MEM:
            return "READ_MEM";
        case PALACIOS_HOST_DEV_HOST_REQUEST_WRITE_MEM:
            return "WRITE_MEM";
        case PALACIOS_HOST_DEV_HOST_REQUEST_READ_CONF:
            return "READ_CONF";
        case PALACIOS_HOST_DEV_HOST_REQUEST_WRITE_CONF:
            return "WRITE_CONF";
    }    
    return "???? type";
}


#endif /* _PALACIOS_H */
