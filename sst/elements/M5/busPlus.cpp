#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <busPlus.h>
#include <memLink.h>
#include <paramHelp.h>

extern "C" {
    SimObject* create_BusPlus( SST::Component*, std::string name, 
                                                    SST::Params& params );
}

SimObject* create_BusPlus( SST::Component* comp, std::string name,
                                                    SST::Params& params )
{
    BusPlusParams& busP = *new BusPlusParams;

    busP.name = name;

    INIT_INT( busP, params, clock);
    INIT_BOOL( busP, params, responder_set );
    INIT_INT( busP, params, block_size);
    INIT_INT( busP, params, bus_id );
    INIT_INT( busP, params, header_cycles);
    INIT_INT( busP, params, width);

    busP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    busP.params = params.find_prefix_params( "link." );

    return new BusPlus( &busP );
}

BusPlus::BusPlus( const BusPlusParams *p ) : 
    Bus( p )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );

    if ( p->params.empty() ) {
        return;
    }

    Port* port = getPort( "port" );
    assert( port );

    SST::Params params = p->params.find_prefix_params( "0." );

    m_links.push_back( MemLink::create( name(), p->m5Comp, port, params ) );
}
