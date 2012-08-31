#include <debug.h>
#include <dll/gem5dll.hh>

bool SST_M5_debug = false;
Log<1> _dbg( std::cerr, "M5:", false );
Log<1> _info( std::cout, "M5:", false );


void enableDebug( std::string name )
{
    bool all = false;
    if ( name.find( "all") != std::string::npos) {
        all = true;
    }

    if ( all || name.find( "SST") != std::string::npos) {
        SST_M5_debug = true;
    }
    libgem5::EnableDebugFlags( name );
}
