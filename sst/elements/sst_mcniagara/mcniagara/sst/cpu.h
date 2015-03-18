// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CPU_H
#define _CPU_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <OffCpuIF.h>
#include <McNiagara.h>

using namespace SST;

#define DBG_CPU 1

#if DBG_CPU
#define _CPU_DBG( fmt, args...)\
         printf( "%d:Cpu::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_DBG( fmt, args...)
#endif

class Cpu : public Component, OffCpuIF {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        virtual void  memoryAccess(OffCpuIF::access_mode mode,
                                   unsigned long long address,
                                   unsigned long data_size);
        virtual void  NICAccess(OffCpuIF::access_mode mode, unsigned long data_size);
        Cpu( ComponentId_t id, Simulation* sim, Params& params ) :
            Component( id, sim ),
            params( params ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_DBG( "new id=%lu\n", id );
            inputfile  = "./notavail_insthist.dat";
            iprobfile  = "./notavail_instprob.dat";
            perffile  = "./notavail_perfcnt.dat";
            outputfile = "./mc_output";
            registerAsPrimaryComponent();
            primaryComponentDoNotEndSim();

            Params::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
/*                     sscanf( it->second.c_str(), "%f", &frequency ); */
                    frequency = it->second;
                } else if ( ! it->first.compare("mccpu_ihistfile") ) {
                    inputfile = (it->second.c_str());
                } else if ( ! it->first.compare("mccpu_outputfile") ) {
                    outputfile = (it->second.c_str());
                } else if ( ! it->first.compare("mccpu_iprobfile") ) {
                    iprobfile = (it->second.c_str());
                } else if ( ! it->first.compare("mccpu_perffile") ) {
                    perffile = (it->second.c_str());
                }
                ++it;
            } 
            
//            mem = LinkAdd( "MEM" );
/*          handler = new EventHandler< Cpu, bool, Cycle_t, Time_t > */
/*                                      ( this, &Cpu::clock ); */
            handler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );
            _CPU_DBG("-->frequency=%s\n",frequency.c_str());

            memHandler = new EventHandler< Cpu, bool, Event*>
                                                (this, &Cpu::memEvent);
            memLink = LinkAdd( "memory", memHandler);

            // MUST be done after all links are created!
            TimeConverter* tc = registerClock( frequency, handler );

            //SST::_debug_flags |= DBG_ALL; // DBG_LINK | DBG_LINKMAP;
            mcCpu = new McNiagara();
            cyclesAtLastClock = 0;
            printf("CPU period: %ld\n",tc->getFactor());
            _CPU_DBG("Done registering clock\n");            
        }
        void setup() { 
            _CPU_DBG(" (%s) (%s) (%s) (%s)\n", inputfile, iprobfile,
                     perffile, outputfile);
            mcCpu->init(inputfile, this, iprobfile, perffile, 0, 0);
        }
        void finish() {
            char filename[128];
            _CPU_DBG("\n");
            sprintf(filename, "%s.%d", outputfile, Id());
            mcCpu->fini(filename);
        }

    private:

        Cpu( const Cpu& c );

/*         bool clock( Cycle_t, Time_t ); */
        bool clock( Cycle_t );
        ClockHandler_t* handler;
        bool handler1( Time_t time, Event *e );
        Event::Handler_t* memHandler;
        bool memEvent( Event*  );
        McNiagara *mcCpu;
        unsigned long cyclesAtLastClock;

        Params    params;
        Link*       memLink;
        state_t     state;
        who_t       who;
        std::string frequency;
        const char *inputfile;
        const char *iprobfile;
        const char *perffile;
        const char *outputfile;

#if WANT_CHECKPOINT_SUPPORT	
        BOOST_SERIALIZE {
	    printf("cpu::serialize()\n");
            _AR_DBG( Cpu, "start\n" );
	    printf("  doing void cast\n");
            BOOST_VOID_CAST_REGISTER( Cpu*, Component* );
	    printf("  base serializing: component\n");
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
	    printf("  serializing: mem\n");
            ar & BOOST_SERIALIZATION_NVP( mem );
	    printf("  serializing: handler\n");
            ar & BOOST_SERIALIZATION_NVP( handler );
            _AR_DBG( Cpu, "done\n" );
        }
        SAVE_CONSTRUCT_DATA( Cpu ) {
	    printf("cpu::save_construct_data\n");
            _AR_DBG( Cpu, "\n" );

            ComponentId_t   id     = t->_id;
/*             Clock*          clock  = t->_clock; */
            Simulation*     sim  = t->simulation;
            Params        params = t->params;

            ar << BOOST_SERIALIZATION_NVP( id );
            ar << BOOST_SERIALIZATION_NVP( sim );
            ar << BOOST_SERIALIZATION_NVP( params );
        } 
        LOAD_CONSTRUCT_DATA( Cpu ) {
	    printf("cpu::load_construct_data\n");
            _AR_DBG( Cpu, "\n" );

            ComponentId_t   id;
            Simulation*     sim;
            Params        params;

            ar >> BOOST_SERIALIZATION_NVP( id );
            ar >> BOOST_SERIALIZATION_NVP( sim );
            ar >> BOOST_SERIALIZATION_NVP( params );

            ::new(t)Cpu( id, sim, params );
        }
#endif
};

#endif
