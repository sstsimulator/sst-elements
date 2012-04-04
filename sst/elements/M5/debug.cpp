
#include <debug.h>

bool SST_M5_debug = false;
Log<1> _dbg( std::cerr, "M5:", false );
Log<1> _info( std::cout, "M5:", false );

#include <assert.h>
#include <deque>
#include <base/trace.hh>
#include <base/debug.hh>
static void enableDebugFlags(std::string str);

void enableDebug( std::string name )
{
    bool all = false;
    if ( name.find( "all") != std::string::npos) {
        all = true;
    }

    if ( all || name.find( "SST") != std::string::npos) {
        SST_M5_debug = true;
    }
    Trace::enabled = true;

    extern void enableDebugFlags( std::string );
    enableDebugFlags( name );
}

static void
enableDebugFlags(std::string str)
{
    using namespace Debug;

    std::deque< std::string > add;
    std::deque< std::string > remove;
    bool all = false;
    size_t cur = 0;

    str = str.substr( str.find_first_not_of(" ") );

    while ( ( cur = str.find(' ') ) != std::string::npos || str.length() ) {

        if ( ! str.substr(0,cur).compare("All") ) {
            all = true;
        } else if ( str.substr(0,cur).at(0) == '-' ) {
            remove.push_back( str.substr(1,cur-1) );
        } else {
            add.push_back( str.substr(0,cur) );
        }

        if ( cur == std::string::npos ) {
            break;
        }
        str = str.substr(cur);
        if ( str.find_first_not_of(" ") == std::string::npos ) {
            break;
        }
        str = str.substr( str.find_first_not_of(" ") );
    }

    std::deque<std::string>::iterator strIter;
    FlagsMap::iterator i = allFlags().begin();
    FlagsMap::iterator end = allFlags().end();
    for (; i != end; ++i) {
        SimpleFlag *f = dynamic_cast<SimpleFlag *>(i->second);

        if ( f ) {
            if ( all ) {
                f->enable();
            } else {
                strIter = add.begin();
                for ( ; strIter != add.end(); ++strIter ) {
                    if ( ! (*strIter).compare(f->name()) ) {
                        f->enable();
                    }
                }
            }

            strIter = remove.begin();
            strIter = remove.begin();
            for ( ; strIter != remove.end(); ++strIter ) {
                if ( ! (*strIter).compare(f->name()) ) {
                    f->disable();
                }
            }
        }
    }
}
