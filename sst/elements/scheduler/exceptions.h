// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                               

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <exception>
using namespace std;

//TODO: these should take arguments so the errors are more descriptive

class InputFormatException : public exception {
  //thrown when the input (trace) is mis-formatted

  virtual const char* what() const throw() {
    return "Invalidly formatted input: ";
  }
};

class InternalErrorException : public exception {
  //called whenever the simulator detects an invalid state

  virtual const char* what() const throw() {
    return "Internal error";
  }
};

#endif
