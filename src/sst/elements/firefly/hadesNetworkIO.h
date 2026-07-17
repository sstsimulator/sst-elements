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

// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <sst/core/output.h>
#include <sst/core/shared/sharedArray.h>
#include <sst/core/shared/sharedSet.h>
#include <sst/elements/hermes/networkIOapi.h>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace SST {
namespace Firefly {

class Hades;
class VirtNic;
class NetworkIOMapper;

struct FileMetadata {
    std::string                        path;
    Hermes::NetworkIO::OpenMode        mode = Hermes::NetworkIO::OpenMode::ReadOnly;
    std::string                        poolName;
    std::vector<int>                   stripeNids;
    uint64_t                           stripeUnit = (1ULL << 20);
    uint64_t                           capacity   = 0;
    uint64_t                           bytesIn    = 0;
    uint64_t                           bytesOut   = 0;
    uint32_t                           fileId     = 0;
    Hermes::NetworkIO::FileDescriptor  desc;
};

class HadesNetworkIO : public Hermes::NetworkIO::Interface {

  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        HadesNetworkIO,
        "firefly",
        "hadesNetworkIO",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Network Storage I/O API",
        SST::Hermes::Interface
    )
    SST_ELI_DOCUMENT_PARAMS(
        {"nodeMapper.name",
         "Name of the NetworkIOMapper module to use (e.g. firefly.Block_NetworkIOMapper, "
         "firefly.RR_NetworkIOMapper, firefly.Hash_NetworkIOMapper). Required as of PR 6c.",
         ""},
        {"nodeMapper.poolName",
         "Name of the I/O pool this compute job is bound to. Joined with the "
         "literal prefix 'IoPool.' to form the Shared::SharedArray<int> name "
         "this subcomponent subscribes to. Required as of PR 6c; matches the "
         "pool name passed to EmberMPIJob.useNetworkIO(pool=...).",
         ""},
        {"suppressPermitInsert",
         "PR 8 debug-misconfig hook (DO NOT use in production). When non-zero, "
         "skips m_permit.insert(m_nid) during init(phase=2) but still calls "
         "m_permit.publish() so the SharedSet's all-published gate releases "
         "at _componentSetup. Lets pools testsuite synthesise rogue-NID and "
         "empty-permit scenarios without touching production code paths.",
         "0"},
        {"verboseLevel","Sets the level of debug verbosity","0"},
        {"verboseMask","Sets the debug mask","-1"},
    )

    HadesNetworkIO(ComponentId_t id, Params& params);

    ~HadesNetworkIO() {}

    void setOS( Hermes::OS* os ) override;

    void init(unsigned int phase) override;

    void setup() override;

    void finish() override;

    std::string getName() override { return "networkIO"; }

    std::string getType() override { return "networkIO"; }

    void networkIORead(Hermes::Vaddr dest, uint64_t offset, uint64_t length,
                      Callback callback) override;

    void networkIOWrite(uint64_t offset, Hermes::Vaddr src, uint64_t length,
                       Callback callback) override;

    void open(const std::string& path, Hermes::NetworkIO::OpenMode mode,
              const Hermes::NetworkIO::FileInfo& info,
              Hermes::NetworkIO::FileHandle* outHandle,
              Hermes::NetworkIO::StatusCallback callback) override;

    void close(Hermes::NetworkIO::FileHandle handle,
               Hermes::NetworkIO::StatusCallback callback) override;

    void read_at(Hermes::NetworkIO::FileHandle handle, uint64_t offset,
                 Hermes::Vaddr dest, uint64_t length,
                 Hermes::NetworkIO::StatusCallback callback) override;

    void write_at(Hermes::NetworkIO::FileHandle handle, uint64_t offset,
                  Hermes::Vaddr src, uint64_t length,
                  Hermes::NetworkIO::StatusCallback callback) override;

    // PR 10: non-blocking file-handle API.
    void iread_at(Hermes::NetworkIO::FileHandle handle, uint64_t offset,
                  Hermes::Vaddr dest, uint64_t length,
                  Hermes::NetworkIO::IORequest* outReq,
                  Hermes::NetworkIO::StatusCallback callback) override;

    void iwrite_at(Hermes::NetworkIO::FileHandle handle, uint64_t offset,
                   Hermes::Vaddr src, uint64_t length,
                   Hermes::NetworkIO::IORequest* outReq,
                   Hermes::NetworkIO::StatusCallback callback) override;

    void wait(Hermes::NetworkIO::IORequest req,
              Hermes::NetworkIO::IOStatus* outStatus,
              Hermes::NetworkIO::StatusCallback callback) override;

    void waitall(int count, Hermes::NetworkIO::IORequest reqs[],
                 Hermes::NetworkIO::IOStatus statuses[],
                 Hermes::NetworkIO::StatusCallback callback) override;

    void test(Hermes::NetworkIO::IORequest req, bool* outDone,
              Hermes::NetworkIO::IOStatus* outStatus,
              Hermes::NetworkIO::StatusCallback callback) override;

    void testany(int count, Hermes::NetworkIO::IORequest reqs[],
                 int* outIdx, bool* outDone,
                 Hermes::NetworkIO::IOStatus* outStatus,
                 Hermes::NetworkIO::StatusCallback callback) override;

    void cancel(Hermes::NetworkIO::IORequest req,
                Hermes::NetworkIO::StatusCallback callback) override;

  private:
    using FileHandle = Hermes::NetworkIO::FileHandle;
    using IOStatus   = Hermes::NetworkIO::IOStatus;
    using IORequest  = Hermes::NetworkIO::IORequest;

    // PR 10 internals. Mirror of OMPI ompi_wait_sync_t pattern
    // (opal/mca/threads/wait_sync.h:41-49). complete_slot atomic in
    // IORequestImpl carries one of three values:
    //   REQUEST_PENDING   -- no waiter, no completion yet
    //   &WaitSync         -- a waitall has published its sync pointer
    //   REQUEST_COMPLETED -- ACK has arrived and SWAPped the slot
    // See docs/pr-10-design.md §3 + §4.2 (rev 2).
    struct WaitSync;

    struct IORequestImpl : public Hermes::NetworkIO::IORequestBase {
        uint32_t        respKey   = 0;
        IOStatus        status;
        Hermes::NetworkIO::StatusCallback userCb;
        FileHandle      handle    = nullptr;
        bool            isWrite   = false;
        uint64_t        issueTime = 0;
        std::atomic<WaitSync*> complete_slot{nullptr};
        uint32_t        cookie    = 0xDEADBEEFu;
    };

    struct WaitSync {
        std::atomic<int> count{0};
        std::function<void()> on_zero;
        std::vector<IORequest> reqsCopy;
        Hermes::NetworkIO::IOStatus* statuses = nullptr;
        bool wantStatuses = false;
        Hermes::NetworkIO::StatusCallback callback;
    };

    static WaitSync* sentinelPending()   { return nullptr; }
    static WaitSync* sentinelCompleted() {
        return reinterpret_cast<WaitSync*>(static_cast<uintptr_t>(1));
    }
    static bool isSentinel(WaitSync* p) {
        return p == sentinelPending() || p == sentinelCompleted();
    }

    void onRequestComplete(IORequestImpl* impl, IOStatus status);
    void wait_sync_update(WaitSync* sync, int ndone);
    void issueIO(FileHandle handle, uint64_t offset, Hermes::Vaddr addr,
                 uint64_t length, IORequestImpl* impl, bool isWrite);

    void sendOpenToStripeNids(FileMetadata* meta,
                              Hermes::NetworkIO::StatusCallback callback);

    void sendCloseToStripeNids(FileMetadata* meta,
                               Hermes::NetworkIO::StatusCallback callback);

    int  chooseStripeNid(const FileMetadata* meta,
                         uint64_t offset, uint64_t length) const;

    uint64_t parseHintUint(const Hermes::NetworkIO::FileInfo& info,
                           const std::string& key, uint64_t defv) const;

    std::string parseHintStr(const Hermes::NetworkIO::FileInfo& info,
                             const std::string& key,
                             const std::string& defv) const;

    Hades*                       m_osPtr  = nullptr;
    VirtNic*                     m_nicPtr = nullptr;
    SST::Output                  m_dbg;
    NetworkIOMapper*             m_mapper = nullptr;
    std::string                  m_poolName;
    int                          m_nid = -1;
    bool                         m_suppressPermitInsert = false;
    Shared::SharedArray<int>     m_pool;
    Shared::SharedSet<int>       m_permit;

    std::map<FileHandle, std::unique_ptr<FileMetadata>> m_files;
    uint32_t                                            m_nextFileId = 1;
    FileHandle                                          m_legacyHandle = nullptr;

    std::map<IORequest, std::unique_ptr<IORequestImpl>> m_requests;
};

} // namespace Firefly
} // namespace SST
