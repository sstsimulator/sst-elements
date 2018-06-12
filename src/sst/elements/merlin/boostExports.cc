// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "topology/mesh.h"
#include "topology/torus.h"
#include "topology/dragonfly.h"
#include "topology/dragonfly2.h"
#include "reorderLinkControl.h"

// put all of the exports for events derived from interal_router_event 
// in one file because when located in seperate files the serialization 
// code for a internal_router_event threw a "what():  unregistered class"
BOOST_CLASS_EXPORT(SST::Merlin::internal_router_event)
BOOST_CLASS_EXPORT(SST::Merlin::topo_mesh_event)
BOOST_CLASS_EXPORT(SST::Merlin::topo_torus_event)
BOOST_CLASS_EXPORT(SST::Merlin::topo_dragonfly_event)
BOOST_CLASS_EXPORT(SST::Merlin::topo_dragonfly2_event)
BOOST_CLASS_EXPORT(SST::Merlin::ReorderRequest)
