// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLE_PARAM_COMPONENT_H
#define _SIMPLE_PARAM_COMPONENT_H

#include <sst/core/component.h>

namespace SST {
namespace SimpleParamComponent {

class simpleParamComponent : public SST::Component 
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        simpleParamComponent,
        "simpleElementExample",
        "simpleParamComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Param Check Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )
    
    SST_ELI_DOCUMENT_PARAMS(
 	{ "int-param",  "Check for integer values", "-1" },
	{ "str-param",  "Check for string values",  "test" },
	{ "bool-param", "Check for bool values", "true" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )
    
    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    simpleParamComponent(SST::ComponentId_t id, SST::Params& params);
    void setup()  { }
    void finish() { }

private:
    simpleParamComponent();  // for serialization only
    simpleParamComponent(const simpleParamComponent&); // do not implement
    void operator=(const simpleParamComponent&); // do not implement
};

} // namespace simpleParamComponent
} // namespace SST

#endif /* simpleParamComponent */
