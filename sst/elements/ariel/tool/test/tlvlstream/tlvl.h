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


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef int tlvl_Tag;

void* tlvl_malloc(size_t size, int level);
void  tlvl_free(void* ptr);
tlvl_Tag tlvl_memcpy(void* dest, void* src, size_t length);
void tlvl_waitComplete(tlvl_Tag in);
void tlvl_set_pool(int pool);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
