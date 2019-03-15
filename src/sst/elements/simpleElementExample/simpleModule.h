// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLE_MODULE_H
#define _SIMPLE_MODULE_H

#include<vector>

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/module.h>
#include <sst/core/link.h>

namespace SST {
namespace SimpleModule {

class SimpleModuleExample : public SST::Module {

public:
	SimpleModuleExample(SST::Params& params);

	SST_ELI_REGISTER_MODULE(
		SimpleModuleExample,
		"simpleElementExample",
		"SimpleModule",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Simple module to demonstrate interface.",
		"SST::SimpleModule::SimpleModuleInterface"
	)

	SST_ELI_DOCUMENT_PARAMS(
        	{"modulename", "Name to give this module", ""},
    	)

	void printName();

private:
	std::string modName;

};

}
} // namespace SST

#endif /* _SIMPLE_MODULE_H */
