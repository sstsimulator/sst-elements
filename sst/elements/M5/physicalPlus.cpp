#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <physicalPlus.h>
#include <memLink.h>
#include <paramHelp.h>
#include <loadMemory.h>

extern "C" {
    SimObject* create_PhysicalPlus( SST::Component*, std::string name,
                                                    SST::Params& params );
}

SimObject* create_PhysicalPlus( SST::Component* comp, std::string name,
                                                    SST::Params& _params )
{
    PhysicalMemoryPlusParams& params = *new PhysicalMemoryPlusParams;

    params.name = name;

    INIT_HEX( params, _params, range.start );
    INIT_HEX( params, _params, range.end );
    INIT_INT( params, _params, latency );
    INIT_INT( params, _params, latency_var );
    INIT_BOOL( params, _params, null );
    INIT_BOOL( params, _params, zero );
    INIT_STR( params, _params, file );

    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.params["name"]        = _params.find_string( "link.name" );
    params.params["range.start"] = _params.find_string("range.start");
    params.params["range.end"]   = _params.find_string("range.end");

    PhysicalMemory* physmem = new PhysicalMemoryPlus( &params );

    loadMemory( name, physmem, _params );

    return physmem;
}

PhysicalMemoryPlus::PhysicalMemoryPlus( const PhysicalMemoryPlusParams *p ) :
    PhysicalMemory( p )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );
    Port* port = getPort( "port", 0 );
    assert( port );

    m_link =  MemLink::create( name(), p->m5Comp, port, p->params );
}
