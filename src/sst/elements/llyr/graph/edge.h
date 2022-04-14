// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _LLYR_G_EDGE_H
#define _LLYR_G_EDGE_H

#include <sst/core/sst_config.h>
#include <sst/core/output.h>

#include <cstdint>

namespace SST {
namespace Llyr {

struct EdgeProperties
{
    float weight_;
};

class Edge
{
private:
    EdgeProperties* properties_;
    uint32_t destinationVertex_;

protected:

public:
    explicit Edge( uint32_t vertexIn )
    {
        properties_ = NULL;
        destinationVertex_ = vertexIn;
    }
    explicit Edge( EdgeProperties* properties, uint32_t vertexIn )
    {
        properties_ = properties;
        destinationVertex_ = vertexIn;
    }
    ~Edge();

    bool setProperties( EdgeProperties* properties )
    {
        properties_ = properties;
        return true;
    }

    EdgeProperties* getProperties( void ) const
    {
        return properties_;
    }

    uint32_t getDestination( void ) const
    {
        return destinationVertex_;
    }

}; //END Edge




} // namespace LLyr
} // namespace SST

#endif



