#ifndef _util_h
#define _util_h

class SimObject;

class M5;

typedef std::map< std::string, SimObject* > objectMap_t;

extern objectMap_t buildConfig( M5*, std::string, std::string, SST::Params& );

static inline std::string resolveString( std::string str )      
{
    std::string retStr;

    do {
        size_t pos1,pos2;
        pos1 = str.find( "${" );
        if ( pos1 == std::string::npos ) {
            retStr += str;
            break;
        }
        pos2 = str.find( "}" );

        retStr += str.substr( 0, pos1 );
        retStr += getenv( str.substr( pos1+2, pos2-(pos1+2) ).c_str() );

        str = str.substr( pos2 + 1 );

    } while ( ! str.empty() );

    return retStr;
}


#endif
