// -*- mode: c++ -*-

// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_MERLIN_H
#define COMPONENTS_MERLIN_MERLIN_H

#include <cctype>
#include <string>
#include <sst/core/simulation.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>


using namespace SST;

namespace SST {
namespace Merlin {

    // Library wide Output object.  Mostly used for fatal() calls
    static Output merlin_abort("Merlin: ", 5, -1, Output::STDERR);
    static Output merlin_abort_full("Merlin: @f, line @l: ", 5, -1, Output::STDOUT);
    
// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.

    // static TimeConverter* getTimeConverterFromGbps(const std::string gbps) {
    //     // Check to make sure this is a valid floating point number.
    //     // Just need to check each character to see if it is a number
    //     // or a period.  Plus, can have only one period.
    //     bool decimal = false;
    //     bool start_num = false;
    //     bool end_num = false;
    //     for ( int i = 0; i < gbps.length(); i++ ) {
    //         if ( !start_num ) {
    //             // Only valid characters are white space, which we
    //             // ignore, or a digit, which starts the number.
    //             if ( isspace(gbps[i]) ) continue;
    //             else if ( isdigit(gbps[i]) ) {
    //                 start_num = true;
    //                 // Decrement index so that it will look at it next
    //                 // loop, but now with start_num set to true.
    //                 i--;
    //             }
    //             else {
    //                 return NULL;
    //             }
    //         }
    //         else if ( !end_num ) {
    //             // Valid characters are digits, decimal point (if a
    //             // decimal point hasn't already occurred) and white
    //             // space (which ends the number).
    //             if ( gbps[i] == '.' ) {
    //                 if ( decimal ) return NULL; // Second decimal point, ERROR
    //                 else {
    //                     decimal = true;
    //                     continue;
    //                 }
    //             }
    //             else if ( isspace(gbps[i]) ) {
    //                 end_num == true;
    //                 continue;
    //             }
    //             else {
    //                 if ( !isdigit(gbps[i] ) ) return NULL;
    //             }
    //         }
    //         else {
    //             // Number is done, only whitespace allowed
    //             if ( !isspace(gbps[i]) ) return NULL;
    //         }
    //     }

    //     // Add "GHz" to end of string and send to getTimeConverter
    //     std::string temp = gbps;
    //     temp.append("GHz");
    //     return Simulation::getSimulation()->getTimeLord()->getTimeConverter(temp);
    // }

}
}

#endif // COMPONENTS_MERLIN_MERLIN_H
