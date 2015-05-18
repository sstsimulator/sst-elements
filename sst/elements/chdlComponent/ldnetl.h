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

#ifndef CHDL_LDNETL_H
#define CHDL_LDNETL_H

#include <string>
#include <map>

#include <chdl/chdl.h>

// Load a CHDL netlist, providing access to all of the ports.
void ldnetl(std::map<std::string, std::vector<chdl::node> > &outputs,
            std::map<std::string, std::vector<chdl::node> > &inputs,
            std::map<std::string, std::vector<chdl::tristatenode> > &inout, 
            std::string filename, bool tap_io);

#endif
