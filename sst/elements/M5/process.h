#include <sim/system.hh>
#include <sim/process.hh>
#include <sst/core/params.h>
#include <params/LiveProcess.hh>

#include <debug.h>
#include <paramHelp.h>

static inline Process* newProcess( const std::string name, 
                                    const SST::Params& params,
                                    System* system  )
{
    LiveProcessParams& process  = * new LiveProcessParams;

    process.name = name;

    // ProcessParams
    process.system = system;
    INIT_STR( process, params, errout );
    if ( process.errout.empty() ) {
        process.errout = "cerr";
    }
    INIT_STR( process, params, input );
    if ( process.input.empty() ) {
        process.input = "cin";
    }
    INIT_STR( process, params, output );
    if ( process.output.empty() ) {
        process.output = "cout";
    }

    INIT_HEX( process, params, max_stack_size );

    // LiveProcessParams
    INIT_INT( process, params, egid );
    INIT_INT( process, params, euid );
    INIT_INT( process, params, gid );
    INIT_INT( process, params, pid );
    INIT_INT( process, params, ppid );
    INIT_INT( process, params, uid );
    INIT_STR( process, params, cwd );
    INIT_STR( process, params, executable );

    process.cmd.resize( 1 );
    process.cmd[0] = params.find_string( "cmd" );
    F_STR( process, cmd[0] );

    process.env.resize( 1 );
    process.env[0] = params.find_string( "env" );
    F_STR( process, env[0] );

    INIT_INT( process, params, simpoint );

    return process.create();
}
