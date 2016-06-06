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

#include "reciever.h"
#include <iostream>
using std::cout;
using std::endl;

void Reciever::processInput(){
  cout<<"sc_in_ = "<<sc_in_.read()<<endl;
  cout<<"native_in_ = "<<native_in_.read()<<endl;
}
