#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <DRAMSimWrap2Plus.h>
#include <memLink.h>
#include <paramHelp.h>

extern "C" {
    SimObject* create_DRAMSimWrap2Plus( SST::Component*, std::string name,
                                                    SST::Params& params );
}

SimObject* create_DRAMSimWrap2Plus( SST::Component* comp, std::string name,
                                                    SST::Params& _params )
{
    DRAMSimWrap2PlusParams& params = *new DRAMSimWrap2PlusParams;

    params.name = name;

    INIT_HEX( params, _params, backdoorRange.start );
    INIT_HEX( params, _params, backdoorRange.end );
    INIT_HEX( params, _params, rank );
    INIT_HEX( params, _params, range.start );
    INIT_HEX( params, _params, range.end );
    INIT_STR( params, _params, info );
    INIT_STR( params, _params, debug );
    INIT_STR( params, _params, frequency );
    INIT_STR( params, _params, pwd );
    INIT_STR( params, _params, printStats);
    INIT_STR( params, _params, deviceIniFilename );
    INIT_STR( params, _params, systemIniFilename );

    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.exeParams = _params.find_prefix_params("exe.");
    params.linkName = _params.find_string( "link.name" );

    return new DRAMSimWrap2Plus( &params );
}

DRAMSimWrap2Plus::DRAMSimWrap2Plus( const DRAMSimWrap2PlusParams *p ) :
    DRAMSimWrap2( p )
{
    DBGX( 2, "name=`%s`\n", name().c_str() );
    Port* port = getPort( "backdoor", 0 );
    assert( port );

    SST::Params params; 
    params["name"]        = p->linkName;
    params["range.start"] = p->range.start;
    params["range.end"]   = p->range.end;

    m_link =  MemLink::create( name(), p->m5Comp, port, params );
}
