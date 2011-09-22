#ifndef _trace_h
#define _trace_h

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <deque>
#include <assert.h>

#include <cxxabi.h>

static inline char const* __demangle( std::type_info const& ti ) {
    static char* buf = NULL;
    if ( buf ) free( buf );
    return  buf = abi::__cxa_demangle(ti.name(),NULL,NULL,NULL);
}

static inline char const* __printf( char const* format, ... )
{
    static char* buf = NULL;
    if ( buf ) free( buf );
    va_list ap;
    va_start(ap,format);
    vasprintf( &buf, format, ap ); 
    va_end( ap );
    return buf;
}

#define OBJ_NAME __demangle(typeid(*this)) 
#define DBG0  __func__ << "():" << __LINE__
#define DBG1( fmt, args... ) DBG0 << __printf( fmt, ##args )
#define DBG2( fmt, args... ) OBJ_NAME << "::" << DBG1( fmt, ##args )

#define DBG_STREAM std::cout
#define DBG( fmt, args... )  DBG_STREAM << DBG2(fmt,##args)

class _Trace {
  public:

    _Trace( FILE* fp = stderr  ) :
        m_fp( fp ),
        m_all( false )  
    {
        char* env = getenv( "TRACE_FLAGS" );
        if ( ! env ) return;
        //::printf("TRACE_FLAGS=`%s`\n",env);
        std::string str = env;
        std::deque< std::string > tmp;

        size_t cur = 0;
        str = str.substr( str.find_first_not_of(" ") );
        while ( ( cur = str.find(' ') ) != std::string::npos ) {
            tmp.push_back( str.substr(0,cur) );
            str = str.substr(cur);
            if ( str.find_first_not_of(" ") == std::string::npos ) {
                break;
            }
            str = str.substr( str.find_first_not_of(" ") );
        }

        tmp.push_back( str );

        std::deque<std::string>::iterator iter = tmp.begin();

        while ( iter != tmp.end() ) {
            if ( (*iter)[0] == '-' ) {
                m_excludeM[ (*iter).substr(1) ];
                //::printf("Trace: exclude %s\n",(*iter).c_str());
            }
            if ( (*iter).compare( "all" ) == 0 ) {
                m_all = true;
                //::printf("Trace: all\n");
            }
            ++iter;
        }
    }

    void setPrefix( std::string const prefix ) {
        m_prefix = prefix;
    }

    void printf( char const* name, char const* format, ... ) {
        char* bufp;
        va_list ap;
        va_start(ap,format);
        vasprintf( &bufp, format, ap ); 
        va_end( ap );

        write( name, bufp, strlen( bufp ) ); 
        free( bufp );
    }

    void write( std::string const name, std::string const str ) {
        write( name, &str.at(0), str.size() );
    } 

    void write( std::string const name, char const* buf, size_t count ) {
        bool flag = false;
        if ( m_all ) {
            if( m_excludeM.find(name) == m_excludeM.end() ) {
                flag = true;
            }
        } else if ( m_nameM.find( name ) != m_nameM.end() ) {
            if ( m_nameM[name] ) {
                flag = true;
            }
        }
        if ( flag ) {
            fwrite( m_prefix.c_str(), 1, strlen( m_prefix.c_str() ), m_fp );
            fwrite( buf, 1, count, m_fp );        
        }
    }

    void add( std::string const name, bool enable = true ) {
        //::printf("Trace::add() %s\n",name.c_str());
        //assert ( m_nameM.find( name ) == m_nameM.end() ); 
        if ( m_nameM.find( name ) == m_nameM.end() ) {
            m_nameM[name] = enable;
        }
    }

    void disable( std::string const name ) {
        if ( m_nameM.find( name ) != m_nameM.end() ) {
            m_nameM[name] = false;
        }
    }
     
  private:
    std::string m_prefix;
    FILE*       m_fp;
    std::map<std::string,bool> m_nameM;
    std::map<std::string,bool> m_excludeM;
    bool        m_all;
};

extern _Trace __trace;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define TRACE_ADD( x ) __trace.add( TOSTRING(x) )
#define TRACE_WRITE( x, y ) __trace.write( TOSTRING(x), y )
#define TRACE_PRINTF( x, fmt, args... ) \
    __trace.printf( TOSTRING(x), fmt, ##args )
#define TRACE_DISABLE( x ) __trace.disable( TOSTRING(x) )
#define TRACE_SET_PREFIX( x ) __trace.setPrefix( x )

#define PRINT_AT( x, fmt, args... ) \
    TRACE_PRINTF( x, "%s::%s():%d "fmt, OBJ_NAME, __func__, __LINE__, ##args ) 

#endif
