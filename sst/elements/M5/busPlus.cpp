#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <busPlus.h>
#include <paramHelp.h>
#include <memLink.h>

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

    busP.m5Comp = static_cast<M5*>(static_cast<void*>(comp));

    busP.params["linkName"] = "mem2bus"; 
    busP.params["range.start"] = "0x00100000"; 
//    busP.params["range.end"] = "0x1fffffff"; 
    busP.params["range.end"] =   "0x20000000"; 

    return new BusPlus( &busP );
}

BusPlus::BusPlus( const BusPlusParams *p ) : 
    Bus( p )
{
    DBGX(2,"name=`%s`\n",name().c_str());
    m_links.push_back( addMemLink( p->m5Comp, p->params ) );
}

BusPlus::linkInfo* BusPlus::addMemLink( M5* comp, const SST::Params& params )
{
    MemLinkParams& memLinkParams = *new MemLinkParams;

    memLinkParams.name = name() + ".link-" + params.find_string("linkName");
    memLinkParams.m5Comp = comp;

    INIT_STR( memLinkParams, params, linkName );
    INIT_HEX( memLinkParams, params, range.start );
    INIT_HEX( memLinkParams, params, range.end );

    linkInfo* info = new linkInfo;

    info->memLink = new MemLink( &memLinkParams );

    info->busPort = getPort( "port" ); 
    assert( info->busPort );

    Port* port = info->memLink->getPort( "" );
    assert( port );

    port->setPeer( info->busPort ); 
    info->busPort->setPeer( port );

    return info;
}
