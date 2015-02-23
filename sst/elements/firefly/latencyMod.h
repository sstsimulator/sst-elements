
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

#define  RANGELATMOD_DBG 0

namespace SST {
namespace Firefly {

class LatencyMod : public SST::Module {
  public:
    virtual size_t getLatency( size_t value ) = 0;
  private:
};

class RangeLatMod : public LatencyMod { 

    static const double  linearX = (1/4);

    struct Entry {
        size_t start;
        size_t stop;
        double latency;
    };

    enum { None, Mult, Linear } op;

  public:

    RangeLatMod( Params& params) : op( None), base(0) {
        std::string tmpStr = params.find_string( "base" );
        if ( ! tmpStr.empty() ) {
            UnitAlgebra tmp( tmpStr );
            base = tmp.getValue().convert_to<double>();
        }

        tmpStr = params.find_string("op","None");

        if ( 0 == tmpStr.compare("Mult") ) {
            op = Mult; 
        } else if ( 0 == tmpStr.compare("Linear") ) {
            op = Linear; 
        }

#if RANGELATMOD_DBG 
        printf("%s() op=%s base=%.3f\n", __func__, tmpStr.c_str(), base * 1000000000.0);
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
            if ( std::string::npos == pos ) {
                entry.stop = 0;
            } else {
                if ( ! range.substr( pos + 1 ).empty() ) {
                    entry.stop = atoi( range.substr( pos + 1 ).c_str());
                } else {
                    entry.stop = -1;
                }
            }
            //printf("%s start=%lu stop=%ld\n",range.c_str(), entry.start, entry.stop);

#if RANGELATMOD_DBG  
            printf("%s()   %lu - %lu, value=%.3f ns\n", __func__, entry.start,
                        entry.stop, entry.latency * 1000000000.0 );
#endif
            map.push_back( entry );
        }
    }
    ~RangeLatMod(){};

    size_t getLatency( size_t value ) {
        Entry* mid = NULL; 
        Entry* prev = NULL; 
        Entry* next = NULL; 

#if RANGELATMOD_DBG 
        printf("\n%s() value=%lu op=%d\n",__func__,value,op);
#endif
        std::deque<Entry>::iterator iter = map.begin(); 

        for ( ; iter != map.end(); ++iter ) {

        //    printf("%s() %p\n",__func__,&*(iter));
            if ( value >= iter->start && 
                        ( -1 == (signed)iter->stop || value <= iter->stop ) ) {
#if RANGELATMOD_DBG 
                printf("%s() found, start %lu, stop %lu, value %.3f ns\n",
                    __func__, iter->start, iter->stop,
                    iter->latency * 1000000000.0);
#endif
                mid = &*iter;
                if ( (iter+1) != map.end() ) {
                    next = &*(iter+1);
                }
                if ( iter != map.begin() ) {
                    prev = &*(iter-1);
                }
                break; 
            }
        } 

        double tmp = 0;
        if ( mid ) {
#if 0
            printf("%s() %lu %lu %f\n",__func__,mid->start,mid->stop,mid->latency * 1000000000);
            if ( next ) {
                printf("%s() %lu %lu %f\n",__func__,next->start,next->stop,next->latency * 1000000000);
            }
#endif
            switch ( op ) {
              case Mult:
                tmp = mid->latency * (double) value;
                break;
              case Linear:
                tmp = calcLinear( value, prev, mid, next ) * (double) value;
                break;
              case None:
                tmp = mid->latency;
                break;
            }
        }

        size_t ret = llround( ( base + tmp ) * 1000000000.0 );
#if RANGELATMOD_DBG 
        printf("%s() value=%lu -> base %.3f + tmp %.3f = %lu ns\n", __func__,
                    value, base * 1000000000.0, tmp * 1000000000.0, ret);
#endif
        return ret;
    }
    double calcLinear( size_t x, Entry* prev, Entry* mid, Entry* next  ) {
        double latency = 0;
        if ( prev && ( -1 != (signed)mid->stop ) && x < mid->start + (mid->stop - mid->start) * linearX ) {
            latency =  xxx( x, *prev, *mid );
        } else if ( next && ( -1 != (signed)mid->stop ) && x > mid->start + (mid->stop - mid->start) * (1.0 - linearX ) ) {
            latency =  xxx(  x, *mid, *next );
        } else if ( mid ) {
            latency = mid->latency;
        }
        return latency;
    }

    double xxx( size_t x, Entry& one, Entry& two  )
    {
        double start = one.start + (double) (one.stop - one.start) * (1.0 - linearX);
        double stop = two.start + (double) (two.stop - two.start) * linearX;
        double slope = (one.latency - two.latency)/(start - stop);

        return (slope * (x - start ) + one.latency);
    }
    
  private:
    double base;
    std::deque< Entry > map;
};

}
}

#endif
