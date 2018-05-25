// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#ifndef _H_SST_NVM_RANK
#define _H_SST_NVM_RANK


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>
#include "Bank.h"
// Defines the state of the bank

// This class structure represents NVM memory Bank 
class RANK
{ 

	// This determines the rank busy until time, this is used to enforce timing parameters, such as maximum number of bank activation per unit of time
	// It also covers the time when the shared circutary between banks is used (e.g., data output circuits of NVM chips)
	long long int BusyUntil;

	int num_banks;

	BANK * banks;
	public: 

	RANK(int numBanks) {BusyUntil = 0; num_banks = numBanks; banks = new BANK[numBanks];}

	// Get a specific bank inside this rank
	BANK * getBank(int ind) { return &banks[ind];} 
	void setBusyUntil(long long int x) {BusyUntil = x;}
	long long int getBusyUntil() { return BusyUntil; }

};

#endif
