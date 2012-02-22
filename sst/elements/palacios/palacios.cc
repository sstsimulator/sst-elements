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

#include "palacios.h"

#include <sys/select.h>

void PalaciosIF::sigHandler(int)
{
}

void* PalaciosIF::thread1( void* ptr )
{
    return ((PalaciosIF*)ptr)->thread2();
}

void* PalaciosIF::thread2( )
{
    DBGX(x,"enter\n");

    struct palacios_host_dev_host_request_response *req;
    struct palacios_host_dev_host_request_response *resp;

    while( m_threadRun ) {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(m_fd,&readset);
        
        int ret = select(m_fd+1, &readset, 0, 0, 0);
    
        if ( ret > 0 && FD_ISSET(m_fd,&readset) ) {
    
            pthread_mutex_lock( &m_mutex );
            int ret;
            ret = v3_user_host_dev_pull_request( m_fd, &req );
            assert( ret != -1);
            do_work(req, &resp);
            ret = v3_user_host_dev_push_response( m_fd, resp );
            assert( ret != -1);
            free(resp);
            free(req);
            pthread_mutex_unlock( &m_mutex );
        }
    }   
    DBGX(x,"leave\n");
    return NULL;
}

int PalaciosIF::do_work(
        struct palacios_host_dev_host_request_response *req,
        struct palacios_host_dev_host_request_response **resp)
{
    uint64_t datasize;
 
#if 0 
    DBGX(x,"%s data_len=%d len=%d port=%d gpa=%p op_len=%d %d\n",
        getType(req->type), req->data_len, req->len, req->port,
        req->gpa, req->op_len, sizeof(*req));
#endif

    switch (req->type) {
        case PALACIOS_HOST_DEV_HOST_REQUEST_READ_MEM:
        {
            int len = sizeof(struct
                       palacios_host_dev_host_request_response) + req->op_len;
            *resp = (struct palacios_host_dev_host_request_response *)
                                            malloc( len );
            assert(*resp);
            **resp = *req;
            (*resp)->data_len = len;
            readMem( (unsigned long) req->gpa, (*resp)->data, req->op_len );
        }
            break;
        case PALACIOS_HOST_DEV_HOST_REQUEST_WRITE_MEM:
        {
            writeMem( (unsigned long) req->gpa, req->data, req->op_len );
            int len = sizeof(struct palacios_host_dev_host_request_response);
            *resp = (struct palacios_host_dev_host_request_response *)
                                            malloc( len );
            assert(*resp);
            **resp=*req;
            (*resp)->len = (*resp)->data_len = len;
            (*resp)->op_len = req->op_len;
        }
            break;

        default:
            DBGX(x,"Huh?  Unknown request %d\n", req->type);
    }
    return 0;
}
