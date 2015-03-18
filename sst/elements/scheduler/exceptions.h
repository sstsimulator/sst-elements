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

#ifndef SST_SCHEDULER_EXCEPTIONS_H__
#define SST_SCHEDULER_EXCEPTIONS_H__

#include <exception>

namespace SST {
    namespace Scheduler {

        //FIXME: these should take arguments so the errors are more descriptive

        class InputFormatException : public std::exception {
            //thrown when the input (trace) is mis-formatted

            virtual const char* what() const throw() {
                return "Invalidly formatted input: ";
            }
        };

        class InternalErrorException : public std::exception {
            //called whenever the simulator detects an invalid state

            virtual const char* what() const throw() {
                return "Internal error";
            }
        };

    }
}
#endif
