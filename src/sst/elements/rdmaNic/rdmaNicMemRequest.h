// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

    struct MemRequest {

        typedef std::function<void(Interfaces::StandardMem::Request*, int )> Callback;
        
        enum Op { Write, Read, Fence } m_op;
        MemRequest( int src, uint64_t addr, int dataSize, uint8_t* data, Callback* callback = NULL  ) : 
            callback(callback), src(src), m_op(Write), addr(addr), dataSize(dataSize) { 
            buf.resize( dataSize );
            memcpy( buf.data(), data, dataSize );
        }
        MemRequest( int src, uint64_t addr, int dataSize, uint64_t data, Callback* callback = NULL  ) : 
            callback(callback), src(src), m_op(Write), addr(addr), dataSize(dataSize), data(data) { }

        MemRequest( int src, uint64_t addr, int dataSize, int id, Callback* callback = NULL ) :
            callback(callback), src(src), m_op(Read), addr(addr), dataSize(dataSize), id(id) { }

        MemRequest( int src, Callback* callback = NULL ) : callback(callback), src(src), m_op(Fence), dataSize(0) {} 
        ~MemRequest() { }
        bool isFence() { return m_op == Fence; }
        uint64_t reqTime;

        virtual void handleResponse( Interfaces::StandardMem::Request* resp ) { 
            if ( callback ) {
                (*callback)( resp, id );
            } else {
                delete resp; 
            }
        }

		int id;
        Callback* callback;
        int      src;
        uint64_t addr;
        int      dataSize;
        uint64_t data;
        std::vector<uint8_t> buf;
    };

