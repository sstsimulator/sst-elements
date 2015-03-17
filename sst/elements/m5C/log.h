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

#ifndef _log_H
#define _log_H

#include <cstdarg>
#include <iostream>
#include <fstream>
#include <stdlib.h>

namespace SST {
namespace M5 {

template < int ENABLE = 1 >
class Log {
    public:
        Log( std::ostream& stream, std::string prefix = "", bool enable = true ) :
            m_ostream( stream ), m_prefix( prefix ), m_level( 0 )
        {}
    
        int level( int level ) {
            int tmp = m_level;
            m_level = level;
            return tmp;
        }
        
        void enable() {
            m_level = 1;
        }

        int level() const {
            return m_level;
        }

        void prepend( std::string str ) {
            m_prefix = str + m_prefix; 
        }

        inline void write(  const char* fmt, ... )
        {
            if ( ENABLE ) {

                if ( m_level ) return;
                char* ptr;

                va_list ap;
                va_start( ap,fmt );
                vasprintf( &ptr, fmt, ap ); 
                va_end( ap);

                m_ostream << m_prefix << ptr;
                free(ptr);
            }
        }
        inline void write( int level, const char* fmt, ... )
        {
            if ( ENABLE ) {

                if ( level > m_level ) return;
                char* ptr;

                va_list ap;
                va_start( ap,fmt );
                vasprintf( &ptr, fmt, ap ); 
                va_end( ap);

                m_ostream << m_prefix << ptr;
                free(ptr);
            }
        }

    private:
        std::ostream&     m_ostream;
        std::string m_prefix;
        int         m_level;
};

}
}
#endif
