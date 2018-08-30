
// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_SCALELATMOD_H
#define COMPONENTS_FIREFLY_SCALELATMOD_H

#include "latencyMod.h"
#include <sst/core/elementinfo.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

namespace SST {
namespace Firefly {

#define SCALELATMOD_DBG 0

class ScaleLatMod : public LatencyMod { 

  public:
    SST_ELI_REGISTER_MODULE(
        ScaleLatMod,
        "firefly",
        "ScaleLatMod",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Firefly::ScaleLatMod"
    )
  private:

    struct Entry {
        uint64_t start;
        uint64_t stop;
        double valueStart;
        double valueStop;
    };

  public:

    ScaleLatMod( Params& params)  {
        Params range = params.find_prefix_params("range.");
        range.enableVerify(false);

        std::set<std::string> keys = range.getKeys();

        std::set<std::string>::iterator iter = keys.begin();
        for ( ; iter != keys.end(); ++iter ) {
            Entry entry;
            std::string tmp = range.find<std::string>(*iter);

            std::size_t pos = tmp.find(":");
            std::string range = tmp.substr( 0, pos );
            std::string value = tmp.substr( pos + 1 );
#if SCALELATMOD_DBG 
            printf("%s %s\n",range.c_str(), value.c_str());
#endif

            pos = range.find("-");
            entry.start = atoi(range.substr(0, pos ).c_str());
            entry.stop = atoi(range.substr( pos + 1 ).c_str());

            pos = value.find("-");
            UnitAlgebra start( value.substr( 0, pos) );
            entry.valueStart = start.getValue().convert_to<double>();

            UnitAlgebra stop( value.substr( pos + 1) );
            entry.valueStop = stop.getValue().convert_to<double>();

            m_deque.push_back( entry );
        }
    }
    ~ScaleLatMod(){};

    size_t getLatency( size_t value ) {
        double mult = 0;
        std::deque<Entry>::iterator iter = m_deque.begin(); 
        Entry* entry = NULL;
        for ( ; iter != m_deque.end(); ++ iter ) {
            if ( value >= iter->start && value < iter->stop ) {
                entry = &(*iter);
                break; 
            }
        }
#if SCALELATMOD_DBG 
        printf("value=%lu\n",value);
#endif
        if ( entry ) {
            mult = entry->valueStart;

#if SCALELATMOD_DBG 
            printf("range: %lu - %lu, value: %.3f ns - %.3f ns \n",entry->start, entry->stop, 
                    entry->valueStart * 1000000000.0, entry->valueStop * 1000000000.0); 
#endif
            double percent = (double) (value - entry->start) / 
                            (double) (entry->stop - entry->start);
            
#if SCALELATMOD_DBG 
            printf("%.12f percent\n",percent);
#endif
            
            if ( percent > 0  ) {

                double delta = entry->valueStop - entry->valueStart;
                double log = log10( percent * 10 );
                double adjLog = log + 9 ;
                double foo = adjLog/10.0;
#if SCALELATMOD_DBG 
                printf( "value=%lu log=%f delta=%.3f ns, foo=%.2f %.3f\n",  value,
                            log, delta * 1000000000.0, foo, foo * delta * 1000000000.0  );
#endif
                mult = entry->valueStart + foo * delta;
            }
        }
        // convert from seconds to nanoseconds
        mult *= 1000000000.0;
#if SCALELATMOD_DBG 
        printf("value=%lu, mult=%.2f, return %lu ns\n",value, mult, 
                                (size_t)( (float) value * mult ));
#endif
        return mult * value;
    }
  private:
    std::deque<Entry> m_deque;
};

}
}

#endif
