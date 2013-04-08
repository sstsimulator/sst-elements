// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

enum lua_value_types
{
  L_INT=1,
  L_DOUBLE,
  L_LONG,
  L_STRING,
  L_BOOLEAN
};
