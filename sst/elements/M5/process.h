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

    process.cmd.resize(1);
    process.env.resize(1);
    process.cmd[0] = "";
    process.env[0] = "";

    SST::Params tmp = params.find_prefix_params( "cmd." ); 
    if ( tmp.size() ) {
        process.cmd.resize( tmp.size() );
        SST::Params::iterator iter = tmp.begin();
        for ( int i = 0; i < process.cmd.size(); i++ ) { 
            process.cmd[i] = (*iter).second;
            ++iter;
            F_STR( process, cmd[i] );
        }
    }

    tmp = params.find_prefix_params( "env." ); 
    if ( tmp.size() ) {
        process.env.resize( tmp.size() );
        SST::Params::iterator iter = tmp.begin();
        for ( int i = 0; i < process.env.size(); i++ ) { 
            process.env[i] = (*iter).second;
            ++iter;
            F_STR( process, env[i] );
        }
    }

    INIT_INT( process, params, simpoint );

    return process.create();
}
