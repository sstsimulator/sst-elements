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

#ifndef VAULTGLOBALS_H
#define VAULTGLOBALS_H

static const unsigned int LL_SHIFT = 12; // min hash between cubes in 1024
static const unsigned int VAULT_SHIFT = 7; // min hash for vaults in 16
static const unsigned int VAULT_MASK = ((1<<VAULT_SHIFT)-1);


#endif
