#ifndef CHDL_LDNETL_H
#define CHDL_LDNETL_H

#include <string>
#include <map>

#include <chdl/chdl.h>

// Load a CHDL netlist, providing access to all of the ports.
void ldnetl(std::map<std::string, std::vector<chdl::node> > &outputs,
            std::map<std::string, std::vector<chdl::node> > &inputs,
            std::map<std::string, std::vector<chdl::tristatenode> > &inout, 
            std::string filename);

#endif
