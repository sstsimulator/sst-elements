// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "hadesStorageController.h"

using namespace SST::Firefly;

HadesStorageController::HadesStorageController(ComponentId_t id, Params& params) :
    SST::SubComponent(id)
{
    m_dbg.init("@t:HadesStorageController::@p():@l ",
        params.find<uint32_t>("verboseLevel", 0),
        params.find<uint32_t>("verboseMask", -1),
        Output::STDOUT);

    m_poolName          = params.find<std::string>("poolName", "default");
    m_storageRankInPool = params.find<int>("storageRankInPool", 0);
    m_poolSize          = params.find<int>("poolSize", 0);
    m_nid               = params.find<int>("nid", -1);

    if (m_poolName.empty()) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesStorageController: poolName must be non-empty\n");
    }
    if (m_poolSize <= 0) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesStorageController(pool=%s): poolSize must be > 0, got %d\n",
            m_poolName.c_str(), m_poolSize);
    }
    if (m_storageRankInPool < 0 || m_storageRankInPool >= m_poolSize) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesStorageController(pool=%s): storageRankInPool=%d out of "
            "range [0,%d)\n",
            m_poolName.c_str(), m_storageRankInPool, m_poolSize);
    }
    if (m_nid < 0) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesStorageController(pool=%s): nid must be >= 0, got %d\n",
            m_poolName.c_str(), m_nid);
    }

    const std::string regionName = "IoPool." + m_poolName;

    // SharedArray<int> with verify_type=INIT_VERIFY (default).
    // Same-value re-writes from peer NICs on this NID coalesce silently
    // (sharedArray.h:350-358); divergent writes fatal loudly.
    m_pool.initialize(regionName, m_poolSize);
    m_pool.write(m_storageRankInPool, m_nid);
    m_pool.publish();

    const std::string permitName = "IoPoolPermit." + m_poolName;
    m_permit.initialize(permitName);
    m_permit.publish();

    m_dbg.verbose(CALL_INFO, 1, 0,
        "published nid=%d -> %s[%d] (poolSize=%d) + permit subscriber %s\n",
        m_nid, regionName.c_str(), m_storageRankInPool, m_poolSize,
        permitName.c_str());
}
