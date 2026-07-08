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

//
// HadesStorageController — PR 6c (per pr-6-design.md rev 2 §3.5)
//
// One-shot publisher subcomponent loaded *on storage-side NICs only* (via
// StorageNicConfiguration in pyfirefly.py).  Mirrors the SharedArray
// publish/subscribe idiom established by Hades::m_sreg (see hades.cc:90-125):
//
//   * Subscribes to the SST::Shared::SharedArray<int> named "IoPool.<name>"
//     with verify_type = INIT_VERIFY (default).  Size is the full pool's
//     storage-NID count, which every controller in the pool agrees on.
//   * Writes its own NID into slot `storageRankInPool`.  Multiple NICs on
//     the same storage NID will write the same (rank, nid) pair, which
//     INIT_VERIFY coalesces silently (same-value re-writes do not fatal,
//     divergent writes do).  See sharedArray.h:350-358.
//   * Calls publish() unconditionally.  publish() is idempotent and is
//     required for the all-published gate to release at _componentSetup
//     (see sharedArray.h:155-170 and the Hades::m_sreg precedent).
//
// The compute side (HadesNetworkIO, also PR 6c) is the matching subscriber:
// it calls initialize() with length==0 and publish(), then reads
// m_pool[m_mapper->calcStripeIdx(offset, length)] at simulation runtime.
//
// This subcomponent has no event loop, no clock, no statistics (per design
// §7 R-12).  Its entire job is to bridge Python-time pool configuration
// (which knows the {pool_name, rank, nid} triple) into the SharedArray
// namespace that HadesNetworkIO subscribes to.
//
// Oracle review (PR 6c, 2026-05-15) shifted four points from the original
// design:
//   * No coreId==0 guard — rely on INIT_VERIFY same-value coalescing.
//   * Single dispatch in HadesNetworkIO::networkIORead/Write (no legacy
//     branch on empty poolName); poolName is now a hard precondition.
//   * No uint16_t poolId on NetworkIOMsgHdr in PR 6c — receive side has no
//     consumer for it; deferred to PR 6d's permit-deny path.
//   * Subscriber lifecycle (length-0 initialize + publish) is correct as-is.
//
// See also: networkIOMapper.h §3.3 (calcStripeIdx fallback now fatals).

#pragma once

#include <sst/core/subcomponent.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include <sst/core/shared/sharedArray.h>
#include <sst/core/shared/sharedSet.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace SST {
namespace Firefly {

struct FileSlot {
    std::string         poolName;
    std::vector<int>    stripeNids;
    uint64_t            stripeUnit = 0;
    uint64_t            capacity   = 0;
    uint64_t            bytesIn    = 0;
    uint64_t            bytesOut   = 0;
    uint32_t            refCount   = 0;
};

class HadesStorageController : public SST::SubComponent {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::HadesStorageController)

    SST_ELI_REGISTER_SUBCOMPONENT(
        HadesStorageController,
        "firefly",
        "HadesStorageController",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Storage-side pool publisher: writes this NID into the "
        "Shared::SharedArray<int> 'IoPool.<poolName>' at slot "
        "storageRankInPool. Mirrors the Hades::m_sreg publish/subscribe "
        "idiom; consumed by HadesNetworkIO on the compute side. No clock, "
        "no events, no statistics — all work happens in the constructor.",
        SST::Firefly::HadesStorageController
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"poolName",
         "Name of the I/O pool this storage NID belongs to. Joined with "
         "the literal prefix 'IoPool.' to form the SharedArray name. "
         "Must match the pool name the compute-side HadesNetworkIO "
         "subscribes to (set via EmberMPIJob.useNetworkIO(pool=...)).",
         "default"},
        {"storageRankInPool",
         "Zero-based index of this storage NID within its pool's "
         "sorted-NID list. The SharedArray slot at this index will hold "
         "this NID. Multiple NICs on the same storage NID write the same "
         "(rank, nid) pair; INIT_VERIFY coalesces same-value writes.",
         "0"},
        {"poolSize",
         "Total number of storage NIDs in this pool. All controllers in "
         "the pool must agree (enforced by SharedArray INIT_VERIFY on "
         "init_data via setSize at sharedArray.h:319-323).",
         "1"},
        {"nid",
         "Global NID of this storage endpoint. Written into "
         "SharedArray slot `storageRankInPool` so the compute side can "
         "resolve stripeIdx -> NID at dispatch time.",
         "0"},
        {"verboseLevel", "Debug verbosity (0=off)", "0"},
        {"verboseMask",  "Debug mask", "-1"},
    )

    HadesStorageController(ComponentId_t id, Params& params);

    ~HadesStorageController() override = default;

    // PR 6d: NIC accessor — read-only view of the pool's permit set for
    // building the m_permitSnapshot hash on the receive hot path.
    const std::string&            getPoolName() const { return m_poolName; }
    Shared::SharedSet<int>&       getPermit()         { return m_permit; }

    // PR 9: per-fileId slot table populated by Open/Close wire ops.
    // Idempotent open bumps refCount; close decrements + erases at zero.
    // Returns slot or nullptr when missing (BadFileHandle).
    FileSlot* findFileSlot(uint32_t fileId) {
        auto it = m_fileSlots.find(fileId);
        return (it == m_fileSlots.end()) ? nullptr : &it->second;
    }
    void addFileSlot(uint32_t fileId,
                     const std::string& poolName,
                     const std::vector<int>& stripeNids,
                     uint64_t stripeUnit,
                     uint64_t capacity)
    {
        auto it = m_fileSlots.find(fileId);
        if (it != m_fileSlots.end()) {
            ++it->second.refCount;
            return;
        }
        FileSlot s;
        s.poolName   = poolName;
        s.stripeNids = stripeNids;
        s.stripeUnit = stripeUnit;
        s.capacity   = capacity;
        s.refCount   = 1;
        m_fileSlots.emplace(fileId, std::move(s));
    }
    bool removeFileSlot(uint32_t fileId) {
        auto it = m_fileSlots.find(fileId);
        if (it == m_fileSlots.end()) return false;
        if (it->second.refCount > 0) --it->second.refCount;
        if (it->second.refCount == 0) {
            m_fileSlots.erase(it);
            return true;
        }
        return false;
    }

  private:
    SST::Output                              m_dbg;
    std::string                              m_poolName;
    int                                      m_storageRankInPool = 0;
    int                                      m_poolSize          = 0;
    int                                      m_nid               = 0;
    Shared::SharedArray<int>                 m_pool;
    Shared::SharedSet<int>                   m_permit;
    std::unordered_map<uint32_t, FileSlot>   m_fileSlots;
};

} // namespace Firefly
} // namespace SST
