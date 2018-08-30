// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_WAIT_REQ_H
#define COMPONENTS_FIREFLY_CTRL_MSG_WAIT_REQ_H

#include "ctrlMsgCommReq.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class WaitReq {
    struct X {
        X( _CommReq* _req, MP::MessageResponse* _resp = NULL ) : 
            pos(0), req(_req), resp(_resp) {}

        X( int _pos, _CommReq* _req, MP::MessageResponse* _resp = NULL ) : 
            pos(_pos), req(_req), resp(_resp) {}

        int pos;
        _CommReq* req;
        MP::MessageResponse* resp;
    };

  public:
    WaitReq( _CommReq* req ) : indexPtr(NULL) {
        reqQ.push_back( X( req ) ); 
    }

    WaitReq( std::vector<_CommReq*> reqs ) : indexPtr(NULL) {
        for ( unsigned int i = 0; i < reqs.size(); i++ ) {
            reqQ.push_back( X( i, reqs[i] ) ); 
        } 
    }

    WaitReq( MP::MessageRequest req, MP::MessageResponse* resp ) :
        indexPtr(NULL)
    {
        reqQ.push_back( X( static_cast<_CommReq*>(req), resp ) );
    }

    WaitReq( int count, MP::MessageRequest req[], int *index,
                                        MP::MessageResponse* resp ) :
        indexPtr(index)
    {
        for ( int i = 0; i < count; i++ ) {
            reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), resp ) );
        }
    }

    WaitReq( int count, MP::MessageRequest req[],
                                        MP::MessageResponse* resp[] ) : 
        indexPtr(NULL)
    {
        MP::MessageResponse* tmp = (MP::MessageResponse*)resp;
        for ( int i = 0; i < count; i++ ) {
			if ( resp ) {
                reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), &tmp[i] ) );
			} else {
                reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]) ) );
			}
        }
    }

    bool isDone() {
        return reqQ.empty();
    }

    _CommReq* getFiniReq() {
        std::deque<X>::iterator iter = reqQ.begin();

        while ( iter != reqQ.end() ) {
            if ( iter->req->isDone() ) {

                _CommReq* req = iter->req;

                if ( iter->resp ) {
                    *iter->resp = *iter->req->getResp(  );
                }

                // a waitany will have an valid indexPtr
                if ( indexPtr ) { 
                    *indexPtr = iter->pos;
                    reqQ.clear();
                } else {
                    iter = reqQ.erase( iter );
                }
                return req;

            } else {
                ++iter;
            }
        } 
        return NULL;
    }

  private:
    std::deque< X > reqQ;
    int* indexPtr; 
};

}
}
}

#endif
