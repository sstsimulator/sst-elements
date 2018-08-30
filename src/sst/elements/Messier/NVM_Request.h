// Copyright 2009-2018 NTESS. Under the terms
// // of Contract DE-NA0003525 with NTESS, the U.S.
// // Government retains certain rights in this software.
// //
// // Copyright (c) 2009-2018, NTESS
// // All rights reserved.
// //
// // This file is part of the SST software package. For license
// // information, see the LICENSE file in the top level directory of the
// // distribution.
// //
//


// /* Author: Amro Awad
//  * E-mail: aawad@sandia.gov
//   */
//


#ifndef _H_SST_NVM_REQUEST
#define _H_SST_NVM_REQUEST
#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>

using namespace SST;

namespace SST{ namespace MessierComponent{

class NVM_Request
{

	public:
		NVM_Request() {}
		NVM_Request(long long id, bool R, int size, long long int Add) { req_ID = id; Read = R; Size = size; Address = Add;}
		long long int req_ID;
		bool Read;
		int Size;
		long long int Address;
		int meta_data;

};

}}
#endif
