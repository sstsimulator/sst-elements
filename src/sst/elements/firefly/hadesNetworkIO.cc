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

#include "hades.h"
#include "hadesNetworkIO.h"
#include "networkIOMapper.h"
#include "virtNic.h"

#include <cstdlib>
#include <memory>
#include <sstream>

using namespace SST::Firefly;

namespace {
constexpr uint64_t kDefaultStripeUnit = 1ULL << 20;   // 1 MiB
} // anon

HadesNetworkIO::HadesNetworkIO(ComponentId_t id, Params& params) :
    Hermes::NetworkIO::Interface(id)
{
    m_dbg.init("@t:HadesNetworkIO::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",-1),
        Output::STDOUT );

    Params mapperParams = params.get_scoped_params("nodeMapper");
    std::string mapperName = mapperParams.find<std::string>("name", "");
    m_poolName             = mapperParams.find<std::string>("poolName", "");

    if ( mapperName.empty() ) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesNetworkIO: nodeMapper.name is required as of PR 6c "
            "(legacy numStorageNodes/ssd_start_node dispatch removed).\n");
    }
    if ( m_poolName.empty() ) {
        m_dbg.fatal(CALL_INFO, -1,
            "HadesNetworkIO: nodeMapper.poolName is required as of PR 6c. "
            "EmberMPIJob.useNetworkIO(...) emits this automatically; "
            "out-of-tree configs must set it explicitly.\n");
    }

    m_suppressPermitInsert =
        params.find<bool>("suppressPermitInsert", false);

    m_mapper = loadModule<NetworkIOMapper>( mapperName, mapperParams );
    if ( ! m_mapper ) {
        m_dbg.fatal(CALL_INFO, -1,
            "Failed to load NetworkIOMapper module '%s'\n", mapperName.c_str());
    }
    m_mapper->setDbg(&m_dbg);

    if ( m_mapper->numNodes() == 0 ) {
        m_dbg.fatal(CALL_INFO, -1,
            "NetworkIOMapper '%s' has empty ioNidList; configure nodeMapper.ioNidList\n",
            mapperName.c_str());
    }

    m_pool.initialize("IoPool." + m_poolName, 0);
    m_pool.publish();

    m_permit.initialize("IoPoolPermit." + m_poolName);

    m_dbg.verbose(CALL_INFO, 1, 0,
                  "subscribed mapper='%s' pool='%s' nidCount=%zu\n",
                  mapperName.c_str(), m_poolName.c_str(),
                  m_mapper->numNodes());
}

void HadesNetworkIO::setOS( Hermes::OS* os )
{
    m_osPtr = dynamic_cast<Hades*>(os);
    assert(m_osPtr);
    m_nicPtr = m_osPtr->getNic();
}

void HadesNetworkIO::init(unsigned int phase)
{
    if (phase == 2 && m_nid < 0 && m_osPtr) {
        m_nid = m_osPtr->getNodeNum();
        assert(m_nid >= 0 &&
               "HadesNetworkIO::init(phase=2): VirtNic::init(phase=1) "
               "must have assigned m_realNicId before this point");
        if (!m_suppressPermitInsert) {
            m_permit.insert(m_nid);
        }
        m_permit.publish();
        m_dbg.verbose(CALL_INFO, 1, 0,
                      "publish permit: pool='%s' nid=%d suppress=%d\n",
                      m_poolName.c_str(), m_nid,
                      (int)m_suppressPermitInsert);
    }
}

void HadesNetworkIO::setup()
{
    assert(m_permit.isFullyPublished() &&
           "HadesNetworkIO::setup(): permit set must be fully published; "
           "EmberEngine::init() must propagate init(phase) to API subcomponents.");
}

void HadesNetworkIO::finish()
{
    // PR 9 §9 Q3: auto-close any handles still open at end-of-sim. Warn
    // (no fatal) per leaked handle. Cleanup is best-effort: motifs that
    // already returned from generate() will not see the close ACKs, but
    // storage-side slots are reclaimed via the wire Close op.
    if (!m_files.empty()) {
        m_dbg.verbose(CALL_INFO, 1, 0,
            "finish(): %zu leaked FileHandle(s) at end-of-sim\n",
            m_files.size());
        for (auto& kv : m_files) {
            FileMetadata* meta = kv.second.get();
            m_dbg.verbose(CALL_INFO, 1, 0,
                "  leak fileId=%u path='%s' bytesIn=%" PRIu64
                " bytesOut=%" PRIu64 "\n",
                meta->fileId, meta->path.c_str(),
                meta->bytesIn, meta->bytesOut);
        }
        m_files.clear();
    }
    if (!m_requests.empty()) {
        m_dbg.verbose(CALL_INFO, 1, 0,
            "finish(): %zu leaked IORequest(s) at end-of-sim\n",
            m_requests.size());
        for (auto& kv : m_requests) {
            IORequestImpl* impl = kv.second.get();
            m_dbg.verbose(CALL_INFO, 1, 0,
                "  leak request cookie=0x%x respKey=%u %s "
                "complete=%d\n",
                impl->cookie, impl->respKey,
                impl->isWrite ? "iwrite_at" : "iread_at",
                impl->complete_slot.load(std::memory_order_acquire)
                    == sentinelCompleted());
        }
        m_requests.clear();
    }
    m_legacyHandle = nullptr;
}

uint64_t HadesNetworkIO::parseHintUint(const Hermes::NetworkIO::FileInfo& info,
                                       const std::string& key,
                                       uint64_t defv) const
{
    auto it = info.hints.find(key);
    if (it == info.hints.end()) return defv;
    try {
        return static_cast<uint64_t>(std::stoull(it->second));
    } catch (...) {
        return defv;
    }
}

std::string HadesNetworkIO::parseHintStr(const Hermes::NetworkIO::FileInfo& info,
                                         const std::string& key,
                                         const std::string& defv) const
{
    auto it = info.hints.find(key);
    if (it == info.hints.end()) return defv;
    return it->second;
}

int HadesNetworkIO::chooseStripeNid(const FileMetadata* meta,
                                    uint64_t offset, uint64_t /*length*/) const
{
    assert(!meta->stripeNids.empty());
    uint64_t idx = (meta->stripeUnit > 0)
                 ? (offset / meta->stripeUnit) % meta->stripeNids.size()
                 : 0;
    return meta->stripeNids[idx];
}

void HadesNetworkIO::open(const std::string& path,
                          Hermes::NetworkIO::OpenMode mode,
                          const Hermes::NetworkIO::FileInfo& info,
                          Hermes::NetworkIO::FileHandle* outHandle,
                          Hermes::NetworkIO::StatusCallback callback)
{
    auto meta = std::make_unique<FileMetadata>();
    meta->path       = path;
    meta->mode       = mode;
    meta->poolName   = parseHintStr(info, "stripe_pool", m_poolName);
    meta->stripeUnit = parseHintUint(info, "striping_unit", kDefaultStripeUnit);
    meta->capacity   = parseHintUint(info, "capacity", 0);

    meta->stripeNids.clear();
    for (size_t i = 0; i < m_mapper->numNodes(); ++i) {
        meta->stripeNids.push_back(m_pool[i]);
    }
    auto sn_it = info.hints.find("stripe_nids");
    if (sn_it != info.hints.end() && !sn_it->second.empty()) {
        meta->stripeNids.clear();
        std::stringstream ss(sn_it->second);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            if (!tok.empty()) meta->stripeNids.push_back(std::stoi(tok));
        }
    }

    meta->fileId          = m_nextFileId++;
    meta->desc.fileId     = meta->fileId;
    meta->desc.mode       = mode;
    meta->desc.path       = path;
    FileHandle handle     = &meta->desc;

    m_dbg.verbose(CALL_INFO, 1, 0,
        "open path='%s' fileId=%u stripeNids=%zu stripeUnit=%" PRIu64
        " capacity=%" PRIu64 "\n",
        path.c_str(), meta->fileId, meta->stripeNids.size(),
        meta->stripeUnit, meta->capacity);

    FileMetadata* meta_raw = meta.get();
    m_files.emplace(handle, std::move(meta));
    if (outHandle) *outHandle = handle;

    sendOpenToStripeNids(meta_raw, callback);
}

void HadesNetworkIO::sendOpenToStripeNids(FileMetadata* meta,
                                          Hermes::NetworkIO::StatusCallback callback)
{
    auto pending = std::make_shared<uint32_t>(
        static_cast<uint32_t>(meta->stripeNids.size()));
    auto status  = std::make_shared<IOStatus>();
    status->bytes_completed = meta->stripeNids.size();
    status->error           = static_cast<int>(Hermes::NetworkIO::Status::OK);

    for (int sn : meta->stripeNids) {
        auto perAck = [callback, pending, status](int retval) mutable {
            if (retval != 0 &&
                status->error == static_cast<int>(Hermes::NetworkIO::Status::OK)) {
                status->error = retval;
            }
            if (--(*pending) == 0) {
                if (callback) callback(*status);
            }
        };
        m_nicPtr->networkIOOpen(sn, meta->fileId, meta->stripeUnit,
                                meta->capacity, meta->stripeNids,
                                meta->poolName, std::move(perAck));
    }
    if (meta->stripeNids.empty()) {
        if (callback) callback(*status);
    }
}

void HadesNetworkIO::sendCloseToStripeNids(FileMetadata* meta,
                                            Hermes::NetworkIO::StatusCallback callback)
{
    auto pending = std::make_shared<uint32_t>(
        static_cast<uint32_t>(meta->stripeNids.size()));
    auto status  = std::make_shared<IOStatus>();
    status->bytes_completed = 0;
    status->error           = static_cast<int>(Hermes::NetworkIO::Status::OK);

    for (int sn : meta->stripeNids) {
        auto perAck = [callback, pending, status](int retval) mutable {
            if (retval != 0 &&
                status->error == static_cast<int>(Hermes::NetworkIO::Status::OK)) {
                status->error = retval;
            }
            if (--(*pending) == 0) {
                if (callback) callback(*status);
            }
        };
        m_nicPtr->networkIOClose(sn, meta->fileId, std::move(perAck));
    }
    if (meta->stripeNids.empty()) {
        if (callback) callback(*status);
    }
}

void HadesNetworkIO::close(Hermes::NetworkIO::FileHandle handle,
                            Hermes::NetworkIO::StatusCallback callback)
{
    auto it = m_files.find(handle);
    if (it == m_files.end()) {
        IOStatus st;
        st.error = static_cast<int>(Hermes::NetworkIO::Status::BadFileHandle);
        if (callback) callback(st);
        return;
    }
    FileMetadata* meta = it->second.get();
    m_dbg.verbose(CALL_INFO, 1, 0,
        "close fileId=%u path='%s'\n", meta->fileId, meta->path.c_str());

    auto erase_self = [this, handle, callback](IOStatus st) {
        m_files.erase(handle);
        if (callback) callback(st);
    };
    sendCloseToStripeNids(meta, std::move(erase_self));
}

void HadesNetworkIO::read_at(Hermes::NetworkIO::FileHandle handle,
                              uint64_t offset, Hermes::Vaddr dest,
                              uint64_t length,
                              Hermes::NetworkIO::StatusCallback callback)
{
    auto it = m_files.find(handle);
    if (it == m_files.end()) {
        IOStatus st;
        st.error = static_cast<int>(Hermes::NetworkIO::Status::BadFileHandle);
        if (callback) callback(st);
        return;
    }
    FileMetadata* meta = it->second.get();
    int targetNid = chooseStripeNid(meta, offset, length);
    uint32_t fileId = meta->fileId;
    m_dbg.verbose(CALL_INFO, 1, 0,
        "read_at fileId=%u off=%" PRIu64 " len=%" PRIu64 " -> nid=%d\n",
        fileId, offset, length, targetNid);

    auto cb = [callback, length](int retval) {
        IOStatus st;
        st.bytes_completed = (retval == 0)
                            ? length
                            : (retval ==
                               static_cast<int>(Hermes::NetworkIO::Status::ShortRead)
                                ? length
                                : 0);
        st.error = retval;
        if (callback) callback(st);
    };
    m_nicPtr->networkIORead(targetNid, dest, length, cb, fileId);
}

void HadesNetworkIO::write_at(Hermes::NetworkIO::FileHandle handle,
                               uint64_t offset, Hermes::Vaddr src,
                               uint64_t length,
                               Hermes::NetworkIO::StatusCallback callback)
{
    auto it = m_files.find(handle);
    if (it == m_files.end()) {
        IOStatus st;
        st.error = static_cast<int>(Hermes::NetworkIO::Status::BadFileHandle);
        if (callback) callback(st);
        return;
    }
    FileMetadata* meta = it->second.get();
    int targetNid = chooseStripeNid(meta, offset, length);
    uint32_t fileId = meta->fileId;
    m_dbg.verbose(CALL_INFO, 1, 0,
        "write_at fileId=%u off=%" PRIu64 " len=%" PRIu64 " -> nid=%d\n",
        fileId, offset, length, targetNid);

    auto cb = [callback, length](int retval) {
        IOStatus st;
        st.bytes_completed = (retval == 0) ? length : 0;
        st.error = retval;
        if (callback) callback(st);
    };
    m_nicPtr->networkIOWrite(targetNid, src, length, cb, fileId);
}

void HadesNetworkIO::networkIORead(Hermes::Vaddr dest, uint64_t offset,
                                    uint64_t length, Callback callback)
{
    // R-5: legacy shim MUST NOT recurse through Interface::read_at. We call
    // VirtNic::networkIORead directly with fileId=0 (legacy sentinel) so
    // the storage side falls through to the mapper-only path (PR 6c
    // behaviour). This keeps the 10 baseline testsuites byte-identical
    // modulo the +4B fileId wire field (rebaselined in step k).
    m_dbg.verbose(CALL_INFO, 1, 0,
        "legacy network_read: dest=%" PRIx64 " offset=%" PRIu64 " length=%" PRIu64 "\n",
        (uint64_t)dest, offset, length);
    int targetNid = m_pool[m_mapper->calcStripeIdx(offset, length)];
    m_nicPtr->networkIORead(targetNid, dest, length, callback, /*fileId=*/0);
}

void HadesNetworkIO::networkIOWrite(uint64_t offset, Hermes::Vaddr src,
                                     uint64_t length, Callback callback)
{
    m_dbg.verbose(CALL_INFO, 1, 0,
        "legacy network_write: offset=%" PRIu64 " src=%" PRIx64 " length=%" PRIu64 "\n",
        offset, (uint64_t)src, length);
    int targetNid = m_pool[m_mapper->calcStripeIdx(offset, length)];
    m_nicPtr->networkIOWrite(targetNid, src, length, callback, /*fileId=*/0);
}

// ============================================================================
// PR 10: non-blocking file-handle API (IORequest + wait/waitall/test/testany
// /cancel). Mirrors OMPI ompi_wait_sync_t pattern; see docs/pr-10-design.md
// §3 (lifetime invariants L-1..L-5) and §4.2 (rev-2 redesign rationale).
// ============================================================================

void HadesNetworkIO::issueIO(FileHandle handle, uint64_t offset,
                              Hermes::Vaddr addr, uint64_t length,
                              IORequestImpl* impl, bool isWrite)
{
    auto it = m_files.find(handle);
    if (it == m_files.end()) {
        IOStatus st;
        st.error =
            static_cast<int>(Hermes::NetworkIO::Status::BadFileHandle);
        onRequestComplete(impl, st);
        return;
    }
    FileMetadata* meta = it->second.get();
    int targetNid = chooseStripeNid(meta, offset, length);
    uint32_t fileId = meta->fileId;
    impl->handle  = handle;
    impl->isWrite = isWrite;
    m_dbg.verbose(CALL_INFO, 1, 0,
        "%s fileId=%u off=%" PRIu64 " len=%" PRIu64 " -> nid=%d req=%p\n",
        isWrite ? "iwrite_at" : "iread_at",
        fileId, offset, length, targetNid, (void*)impl);

    auto cb = [this, impl, length](int retval) {
        IOStatus st;
        if (retval == 0) {
            st.bytes_completed = length;
            st.error = 0;
        } else if (retval ==
                   static_cast<int>(Hermes::NetworkIO::Status::ShortRead)) {
            st.bytes_completed = length;
            st.error = retval;
        } else {
            st.bytes_completed = 0;
            st.error = retval;
        }
        onRequestComplete(impl, st);
    };

    if (isWrite) {
        m_nicPtr->networkIOWrite(targetNid, addr, length, cb, fileId);
    } else {
        m_nicPtr->networkIORead(targetNid, addr, length, cb, fileId);
    }
}

void HadesNetworkIO::iread_at(Hermes::NetworkIO::FileHandle handle,
                               uint64_t offset, Hermes::Vaddr dest,
                               uint64_t length,
                               Hermes::NetworkIO::IORequest* outReq,
                               Hermes::NetworkIO::StatusCallback callback)
{
    auto impl = std::make_unique<IORequestImpl>();
    impl->userCb = std::move(callback);
    IORequestImpl* raw = impl.get();
    if (outReq) *outReq = static_cast<IORequest>(raw);
    m_requests.emplace(static_cast<IORequest>(raw), std::move(impl));
    issueIO(handle, offset, dest, length, raw, /*isWrite=*/false);
}

void HadesNetworkIO::iwrite_at(Hermes::NetworkIO::FileHandle handle,
                                uint64_t offset, Hermes::Vaddr src,
                                uint64_t length,
                                Hermes::NetworkIO::IORequest* outReq,
                                Hermes::NetworkIO::StatusCallback callback)
{
    auto impl = std::make_unique<IORequestImpl>();
    impl->userCb = std::move(callback);
    IORequestImpl* raw = impl.get();
    if (outReq) *outReq = static_cast<IORequest>(raw);
    m_requests.emplace(static_cast<IORequest>(raw), std::move(impl));
    issueIO(handle, offset, src, length, raw, /*isWrite=*/true);
}

void HadesNetworkIO::onRequestComplete(IORequestImpl* impl, IOStatus status)
{
    // Mirror of OMPI ompi_request_complete SWAP at request.h:460-487:
    // SWAP complete_slot to COMPLETED and recover any pending waiter
    // (WaitSync pointer) for synchronous decrement.
    //
    // PR 10 rev-2 Fix A lifetime: wait_sync_update may invoke the
    // WaitSync's on_zero closure, which calls m_requests.erase() on
    // every request in the group INCLUDING `impl`. After that call,
    // `impl` is a dangling pointer. Snapshot userCb to a local before
    // wait_sync_update; do not dereference impl afterwards.
    impl->status = status;
    Hermes::NetworkIO::StatusCallback userCb_copy = impl->userCb;
    WaitSync* old = impl->complete_slot.exchange(sentinelCompleted(),
        std::memory_order_acq_rel);
    m_dbg.verbose(CALL_INFO, 1, 0,
        "onRequestComplete req=%p err=%d bytes=%" PRIu64 " old_slot=%p\n",
        (void*)impl, status.error, status.bytes_completed, (void*)old);
    if (old != sentinelPending() && old != sentinelCompleted()) {
        wait_sync_update(old, 1);
    }
    if (userCb_copy) {
        userCb_copy(status);
    }
}

void HadesNetworkIO::wait_sync_update(WaitSync* sync, int ndone)
{
    // Mirror of OMPI wait_sync_update at wait_sync.h:135-148: atomic
    // fetch_sub on the per-waitall count; when it reaches zero, fire
    // the on_zero closure (which fills statuses[] in INDEX order and
    // invokes the user callback).
    int prev = sync->count.fetch_sub(ndone, std::memory_order_release);
    if (prev == ndone) {
        if (sync->on_zero) sync->on_zero();
    }
}

void HadesNetworkIO::wait(Hermes::NetworkIO::IORequest req,
                           Hermes::NetworkIO::IOStatus* outStatus,
                           Hermes::NetworkIO::StatusCallback callback)
{
    // Single-request convenience wrapper. Build a one-shot waitall.
    Hermes::NetworkIO::IORequest reqs[1] = { req };
    if (outStatus) {
        waitall(1, reqs, outStatus, std::move(callback));
    } else {
        waitall(1, reqs, nullptr, std::move(callback));
    }
}

void HadesNetworkIO::waitall(int count,
                              Hermes::NetworkIO::IORequest reqs[],
                              Hermes::NetworkIO::IOStatus statuses[],
                              Hermes::NetworkIO::StatusCallback callback)
{
    // L-5 short-circuit: empty array is silent success.
    if (count <= 0) {
        if (callback) {
            IOStatus agg;
            callback(agg);
        }
        return;
    }

    auto* sync = new WaitSync();
    sync->count.store(count, std::memory_order_release);

    // PR 10 lifetime model: WaitSync, the request-handle vector and the
    // user callback are all heap-owned by `sync`. on_zero (run on the
    // last ACK from onRequestComplete) frees them in order. L-4:
    // index-order fill.
    sync->reqsCopy.assign(reqs, reqs + count);
    sync->callback = std::move(callback);
    sync->wantStatuses = (statuses != nullptr);
    sync->statuses = statuses;
    sync->on_zero = [this, sync]() {
        std::atomic_thread_fence(std::memory_order_acquire);
        IOStatus agg;
        for (size_t i = 0; i < sync->reqsCopy.size(); ++i) {
            auto it = m_requests.find(sync->reqsCopy[i]);
            assert(it != m_requests.end() &&
                   "PR 10 L-1: waitall on unknown IORequest "
                   "(double-consume or stale handle)");
            IORequestImpl* impl = it->second.get();
            assert(impl->cookie == 0xDEADBEEFu &&
                   "PR 10 L-1: IORequest cookie corrupt");
            if (sync->wantStatuses) sync->statuses[i] = impl->status;
            if (agg.error == 0 && impl->status.error != 0) {
                agg.error = impl->status.error;
            }
            agg.bytes_completed += impl->status.bytes_completed;
            m_requests.erase(it);
        }
        Hermes::NetworkIO::StatusCallback cb = std::move(sync->callback);
        delete sync;
        if (cb) cb(agg);
    };

    // Publish &sync into each request's complete_slot via CAS. Race
    // with ACK: if CAS fails because slot already == COMPLETED, the
    // ACK ran first; decrement the sync synchronously. Mirror of OMPI
    // wait_all CAS loop at req_wait.c:233-252.
    for (int i = 0; i < count; ++i) {
        auto it = m_requests.find(reqs[i]);
        assert(it != m_requests.end() &&
               "PR 10 L-1: waitall on unknown IORequest");
        IORequestImpl* impl = it->second.get();
        WaitSync* expected = sentinelPending();
        if (impl->complete_slot.compare_exchange_strong(
                expected, sync,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            // Successfully published; completion will wake us.
        } else {
            // CAS failed -- expected now holds the observed value.
            assert(expected == sentinelCompleted() &&
                   "PR 10 L-1: waitall race with another waitall on "
                   "same request (double-consume)");
            wait_sync_update(sync, 1);
        }
    }
}

void HadesNetworkIO::test(Hermes::NetworkIO::IORequest req,
                           bool* outDone,
                           Hermes::NetworkIO::IOStatus* outStatus,
                           Hermes::NetworkIO::StatusCallback callback)
{
    // L-2: non-consuming. Reads slot via acquire load; if COMPLETED,
    // publishes status to caller but does NOT erase the request.
    auto it = m_requests.find(req);
    if (it == m_requests.end()) {
        if (outDone) *outDone = true;
        if (outStatus) {
            IOStatus st;
            st.error = static_cast<int>(
                Hermes::NetworkIO::Status::BadFileHandle);
            *outStatus = st;
        }
        if (callback) callback(IOStatus{});
        return;
    }
    IORequestImpl* impl = it->second.get();
    WaitSync* slot = impl->complete_slot.load(std::memory_order_acquire);
    if (slot == sentinelCompleted()) {
        if (outDone) *outDone = true;
        if (outStatus) *outStatus = impl->status;
    } else {
        if (outDone) *outDone = false;
    }
    if (callback) callback(IOStatus{});
}

void HadesNetworkIO::testany(int count,
                              Hermes::NetworkIO::IORequest reqs[],
                              int* outIdx, bool* outDone,
                              Hermes::NetworkIO::IOStatus* outStatus,
                              Hermes::NetworkIO::StatusCallback callback)
{
    // L-5: count==0 short-circuit per MPI 4.1 §4.7.5. Mirror of OMPI
    // ompi_request_default_wait_any at req_wait.c:96-99 (the very
    // first check) and MPICH MPIR_Testany at request_impl.c:416-434.
    if (count <= 0) {
        if (outDone) *outDone = true;
        if (outIdx)  *outIdx  = -1;
        if (outStatus) *outStatus = IOStatus{};
        if (callback) callback(IOStatus{});
        return;
    }
    for (int i = 0; i < count; ++i) {
        auto it = m_requests.find(reqs[i]);
        if (it == m_requests.end()) continue;
        IORequestImpl* impl = it->second.get();
        WaitSync* slot =
            impl->complete_slot.load(std::memory_order_acquire);
        if (slot == sentinelCompleted()) {
            if (outIdx) *outIdx = i;
            if (outDone) *outDone = true;
            if (outStatus) *outStatus = impl->status;
            if (callback) callback(IOStatus{});
            return;
        }
    }
    if (outIdx)  *outIdx  = -1;
    if (outDone) *outDone = false;
    if (callback) callback(IOStatus{});
}

void HadesNetworkIO::cancel(Hermes::NetworkIO::IORequest req,
                             Hermes::NetworkIO::StatusCallback callback)
{
    // L-3 silent success per MPI 4.1 §4.8.4 (cancel for I/O
    // deliberately undefined) + MPICH ROMIO mpiu_greq.c:16-50 (greq
    // cancel_fn returns MPI_SUCCESS) + OMPI common_ompio_request.c:63-76
    // (one-line return OMPI_SUCCESS). We deliberately do NOT touch
    // complete_slot; the in-flight wire packet still completes via the
    // normal ACK path, and the motif's subsequent wait/waitall sees the
    // actual completion status. We deliberately do NOT introduce
    // Status::Cancelled.
    if (req == nullptr || m_requests.find(req) == m_requests.end()) {
        IOStatus st;
        st.error = static_cast<int>(
            Hermes::NetworkIO::Status::BadFileHandle);
        if (callback) callback(st);
        return;
    }
    IOStatus st;
    if (callback) callback(st);
}
