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

#include <cstddef>
#include <boost/archive/detail/common_oarchive.hpp>

#include "pymodule.h"
#include "pyproto.h"

class PyEvent_oarchive :
    public boost::archive::detail::common_oarchive<complete_oarchive>
{
    // permit serialization system privileged access to permit
    // implementation of inline templates for maximum speed.
    friend class boost::archive::save_access;

    // member template for saving primitive types.
    // Specialize for any types/templates that require special treatment
    template<class T>
    void save(T & t)
    {
        fprintf(stderr, "save of type %s called\n", typename(T));
    }

public:
    //////////////////////////////////////////////////////////
    // public interface used by programs that use the
    // serialization library

    // archives are expected to support this function
    void save_binary(void *address, std::size_t count)
    {
        fprintf(stderr, "save_binary of %p[%zu] called\n", address, count);
    }

    // Called before a save()
    void save_start(char const *name)
    {
        fprintf(stderr, "save_start(%s)\n", name);
    }

    void save_end(char const *)
    {
        fprintf(stderr, "save_end(%s)\n", name);
    }
};

