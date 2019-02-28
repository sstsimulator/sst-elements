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

//#include <assert.h>

#include "sst_config.h"
#include "simpleParamComponent.h"

namespace SST {
namespace SimpleParamComponent {

simpleParamComponent::simpleParamComponent(ComponentId_t id, Params& params) :
  Component(id) 
{
	int32_t i32v = params.find<int32_t>("int32t-param");
	printf("int32_t      value=%" PRId32 "\n", i32v);

	uint32_t u32v = params.find<uint32_t>("uint32t-param");
	printf("uint32_t     value=%" PRIu32 "\n", u32v);

	int64_t i64v = params.find<int64_t>("int64t-param");
	printf("int64_t      value=%" PRId64 "\n", i64v);

	uint64_t u64v = params.find<uint64_t>("uint64t-param");
	printf("uint64_t     value=%" PRIu64 "\n", u64v);

	bool btruev = params.find<bool>("bool-true-param");
	printf("bool-true    value=%s\n", (btruev ? "true" : "false"));

	bool bfalsev = params.find<bool>("bool-false-param");
	printf("bool-false    value=%s\n", (bfalsev ? "true" : "false"));

	float f32v = params.find<float>("float-param");
	printf("float         value=%f\n", f32v);

	double f64v = params.find<double>("double-param");
	printf("double        value=%f\n", f64v);

	std::string strv = params.find<std::string>("string-param");
	printf("string        value=\"%s\"\n", strv.c_str());

}

simpleParamComponent::simpleParamComponent() :
    Component(-1)
{

}

} // namespace simpleParamComponent
} // namespace SST


