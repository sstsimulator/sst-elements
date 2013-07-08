// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "funcCtx/debug.h"
#include "funcCtx/rank.h"
#include "hades.h"

using namespace SST::Firefly;
using namespace Hermes;

RankCtx::RankCtx( Communicator group, int* rank, 
        Functor* retFunc, FunctionType type, Hades* obj ) : 
    FunctionCtx( retFunc, type, obj ),
    m_rank( rank ),
    m_group( group )
{ }

bool RankCtx::run( ) 
{
    DBGX("\n");
    *m_rank = m_obj->m_groupMap[m_group]->getMyRank(); 
    return true;
}
