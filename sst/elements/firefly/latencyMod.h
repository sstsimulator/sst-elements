
// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_LATENCYMOD_H
#define COMPONENTS_FIREFLY_LATENCYMOD_H

#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

#include "ioVec.h"

//#define  RANGELATMOD_DBG 1

namespace SST {
namespace Firefly {

class LatencyMod : public SST::Module {
  public:
    virtual size_t getLatency( size_t value ) = 0;
  private:
};

class RangeLatMod : public LatencyMod { 

    struct Entry {
        size_t start;
        size_t stop;
        double latency;
    };

    enum { None, Mult } op;

  public:

    RangeLatMod( Params& params) : op( None), base(0) {
        std::string tmpStr = params.find_string( "base" );
        if ( ! tmpStr.empty() ) {
            UnitAlgebra tmp( tmpStr );
            base = tmp.getValue().convert_to<double>();
        }

        tmpStr = params.find_string("op");

        if ( 0 == tmpStr.compare("Mult") ) {
            op = Mult; 
        }

#if RANGELATMOD_DBG 
        printf("%s() base=%.3f ns\n", __func__, base * 1000000000.0);
        printf("%s() op=%s\n", __func__, tmpStr.c_str());
#endif

        Params range = params.find_prefix_params("range."); 

        Params::iterator iter = range.begin(); 
        for ( ; iter != range.end(); ++iter ) {
            Entry entry;

            std::size_t pos = iter->second.find(":");
            UnitAlgebra tmp( iter->second.substr( pos + 1 ) );

            entry.latency = tmp.getValue().convert_to<double>();

            std::string range = iter->second.substr(0, pos );
            pos = range.find("-");

            entry.start = atoi(range.substr(0, pos ).c_str()); 
            entry.stop = atoi(range.substr( pos + 1 ).c_str());

#if RANGELATMOD_DBG  
            printf("%s() %lu - %lu, value=%.3f ns\n", __func__, entry.start,
                        entry.stop, entry.latency * 1000000000.0 );
#endif
            map.push_back( entry );
        }
    }
    ~RangeLatMod(){};

    size_t getLatency( size_t value ) {
        double tmp = 0;

#if RANGELATMOD_DBG 
        printf("%s() value=%lu\n",__func__,value);
#endif
        std::deque<Entry>::iterator iter = map.begin(); 

        for ( ; iter != map.end(); ++iter ) {

            if ( value >= iter->start && 
                        (iter->stop == 0 || value <= iter->stop ) ) {
#if RANGELATMOD_DBG 
                printf("%s() found, start %lu, stop %lu, value %.3f ns\n",
                    __func__, iter->start, iter->stop,
                    iter->latency * 1000000000.0);
#endif
                tmp = iter->latency;
                break; 
            }
        } 
        switch ( op ) {
          case Mult:
            tmp *= (double)value;
            break;
          case None:
            break;
        }

        size_t ret = llround( ( base + tmp ) * 1000000000.0 );
#if RANGELATMOD_DBG 
        printf("%s() value=%lu -> base %.3f + tmp %.3f = %lu ns\n", __func__,
                    value, base * 1000000000.0, tmp * 1000000000.0, ret);
#endif
        return ret;
    }
    
  private:
    double base;
    std::deque< Entry > map;
};

}
}

#endif
