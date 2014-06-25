// Copyright 2009-2014 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package.  For lucense
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <Python.h>  // Before serialization to prevent spurrious warnings
#include <sst/core/serialization.h>

#include "gem5.h"

// System headers
#include <boost/tokenizer.hpp>
#include <string>       // C++11 feature used

// Gem5 Headers
#include <sim/core.hh>
#include <sim/init.hh>
#include <sim/simulate.hh>
#include <sim/system.hh>
#include <sim/sim_object.hh>
#include <base/misc.hh>
#include <base/debug.hh>

#ifdef fatal  // Gem5 sets this, unfortunately
#undef fatal
#endif

// More SST Headers
#include <sst/core/params.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/debug.h>

using namespace SST;
using namespace SST::gem5;

Gem5Comp::Gem5Comp(ComponentId_t id, Params &params) :
    SST::Component(id)
{
    dbg.init("@t:Gem5:@p():@l " + getName() + ": ", 0, 0,
            (Output::output_location_t)params.find_integer("comp_debug", 0));
    info.init("Gem5:" + getName() + ": ", 0, 0, Output::STDOUT);

    TimeConverter *clock = registerClock(
            params.find_string("frequency", "1GHz"),
            new Clock::Handler<Gem5Comp>(this, &Gem5Comp::clockTick));

    // This sets how many Gem5 cycles we'll need to simulate per clock tick
    sim_cycles = clock->getFactor();


    // Disable Gem5's inform() messages.
    want_info = false;

    std::string cmd = params.find_string("cmd", "");
    if ( cmd.empty() ) {
        _abort(Gem5Comp, "Component %s must have a 'cmd' parameter set.\n", getName().c_str());
    }

    std::vector<char*> args;
    args.push_back(const_cast<char*>("sst.x")); // TODO:  Can we get this from anywhere?
    splitCommandArgs(cmd, args);
    args.push_back(const_cast<char*>("--initialize-only"));
    dbg.output(CALL_INFO, "Command string:  [sst.x %s --initialize-only]\n", cmd.c_str());
    for ( size_t i = 0 ; i < args.size() ; i++ ) {
        dbg.output(CALL_INFO, "  Arg [%02zu] = %s\n", i, args[i]);
    }

    std::vector<char*> flags;
    std::string gem5DbgFlags = params.find_string("gem5DebugFlags", "");
    splitCommandArgs(gem5DbgFlags, flags);
    for ( size_t i = 0 ; i < flags.size() ; i++ ) {
        dbg.output(CALL_INFO, "  Setting Debug Flag [%s]\n", flags[i]);
        setDebugFlag(flags[i]);
    }

    initPython(args.size(), &args[0]);

    // Look for ExtConnectors
    std::string conns = params.find_string("connectors", "");
    std::vector<std::string> vconns;
    splitConnectors(conns, vconns);

    int numPorts = vconns.size();
    for ( int i = 0 ; i < numPorts ; ++i ) {
        std::string &name = vconns[i];
        dbg.output(CALL_INFO, "Looking for connector %s\n", name.c_str());
        ::SimObject *obj = ::SimObject::find(name.c_str());
        if ( !obj )
            info.fatal(CALL_INFO, 1,
                "Unable to find ExtConnector '%s' in the GEM5 configuration.\n",
                name.c_str());

        Gem5Connector *conn = new Gem5Connector(this, info, obj, name);
        connectors.push_back(conn);
    }


    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    clocks_processed = 0;
}


Gem5Comp::~Gem5Comp(void)
{
    Py_Finalize();
}

void Gem5Comp::init(unsigned int phase)
{
    for ( std::vector<Gem5Connector*>::iterator i = connectors.begin() ; i != connectors.end() ; ++i ) {
        (*i)->init(phase);
    }
}

void Gem5Comp::setup(void)
{
    // Switch connectors from initData to regular Sends
    for (std::vector<Gem5Connector*>::iterator i = connectors.begin() ; i != connectors.end() ; ++i ) {
        (*i)->setup();
    }
}

void Gem5Comp::finish(void)
{
    info.output("Complete.  Clocks Processed:  %" PRIu64 "\n", clocks_processed);
}



bool Gem5Comp::clockTick(Cycle_t cycle)
{
    dbg.output(CALL_INFO, "Cycle %lu\n", cycle);

    SimLoopExitEvent *event = simulate( sim_cycles );
    ++clocks_processed;
    if ( event->getCode() != 256 ) {
        info.output("exiting: curTick()=%lu cause=`%s` code=%d\n",
                curTick(), event->getCause().c_str(), event->getCode() );
        primaryComponentOKToEndSim();
        return true;
    }

    return false;
}


void Gem5Comp::splitCommandArgs(std::string &cmd, std::vector<char *> &args)
{
    std::string sep1("\\");
    std::string sep2(" ");
    std::string sep3("\"\'");

    boost::escaped_list_separator<char> els(sep1,sep2,sep3);
    boost::tokenizer<boost::escaped_list_separator<char>> tok(cmd, els);

    for(boost::tokenizer<boost::escaped_list_separator<char>>::iterator beg=tok.begin(); beg!=tok.end();++beg)
    {
        args.push_back(strdup(beg->c_str()));
    }

}

void Gem5Comp::splitConnectors(std::string &cmd, std::vector<std::string> &ports)
{
    boost::char_separator<char> sep(", \r\n\t");
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    tokenizer tokens(cmd, sep);
    for ( tokenizer::iterator tok = tokens.begin() ; tok != tokens.end() ; ++tok ) {
        ports.push_back(*tok);
    }

}


void Gem5Comp::initPython( int argc, char *argv[] )
{
    const char * m5MainCommands[] = {
        "import m5",
        "m5.main()",
        0 // sentinel is required
    };

    PyObject *mainModule,*mainDict;

    Py_SetProgramName(argv[0]);  /* optional but recommended */

    Py_Initialize();

    int ret = initM5Python();
    if ( 0 != ret ) {
        _abort(Gem5Comp, "Python failed to initialize.  Code: %d\n", ret);
    }

    PySys_SetArgv( argc,  argv );

    mainModule = PyImport_AddModule("__main__");
    assert( mainModule );

    mainDict = PyModule_GetDict( mainModule );
    assert( mainDict );

    PyObject *result;
    const char **command = m5MainCommands;

    // evaluate each command in the m5MainCommands array (basically a
    // bunch of python statements.
    while (*command) {
        result = PyRun_String(*command, Py_file_input, mainDict, mainDict);
        if (!result) {
            PyErr_Print();
            break;
        }
        Py_DECREF(result);

        command++;
    }
}
