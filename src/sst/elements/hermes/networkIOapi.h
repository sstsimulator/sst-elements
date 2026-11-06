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

#include <sst/core/sst_types.h>
#include <functional>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include "hermes.h"

namespace SST {
namespace Hermes {
namespace NetworkIO {

// ========================================================================
// Status codes (PR 6d + PR 9)
// ========================================================================
//
// PR 6d (storage-side permit / AckDeny): callers must check status to
// distinguish a successful storage op from a permission-denied AckDeny
// returned by the storage NIC's receive-path permit check.
//
// PR 9 (file-handle API): extends the enum to surface error conditions
// reported via IOStatus.error from open/close/read_at/write_at:
//   ShortRead       -- read past EOF on a sized file
//   NoSpace         -- write past capacity on a sized file
//   BadFileHandle   -- read_at/write_at on a closed/unknown FileHandle
//
// Wire-level encoding: status is carried as `int` (cast to/from this
// enum). The legacy `void(int) Callback` typedef stays unchanged so PR
// 6/7/8 motifs and out-of-tree consumers compile without source edits.
// File-handle operations use the new `StatusCallback = void(IOStatus)`
// typedef which delivers the full IOStatus struct (bytes_completed +
// error code).
enum class Status : int {
    OK               = 0,
    PermissionDenied = 1,
    ShortRead        = 2,
    NoSpace          = 3,
    BadFileHandle    = 4,
};

// ========================================================================
// IOStatus -- structured completion result (PR 9)
// ========================================================================
//
// Mirrors the success/short/error trio carried by POSIX pread/pwrite
// return codes and MPI_Status. v1 callers inspect `error` first; if OK,
// `bytes_completed` should equal the requested length (short ops surface
// as Status::ShortRead with bytes_completed < requested).
struct IOStatus {
    uint64_t bytes_completed = 0;
    int      error           = static_cast<int>(Status::OK);
};

// ========================================================================
// Open modes (PR 9)
// ========================================================================
//
// Subset of MPI_File_open access flags. v1 honours ReadOnly/WriteOnly/
// ReadWrite for routing; Create is reserved for PR 10+ (HadesNetworkIO
// currently ignores it -- files are pre-sized at SDL build time).
enum class OpenMode : int {
    ReadOnly  = 0,
    WriteOnly = 1,
    ReadWrite = 2,
    Create    = 3,
};

// ========================================================================
// FileInfo -- MPI_Info-shaped hint map (PR 9)
// ========================================================================
//
// v1 honours two hints (others ignored, forward-compatible):
//   "striping_unit"   -- bytes per stripe slot (default 1 MiB; matches
//                        Lustre LOV_DESC_STRIPE_SIZE_DEFAULT). Same
//                        semantics as MPI_Info "striping_unit" hint.
//   "stripe_pool"     -- pool name override. Default: HadesNetworkIO's
//                        configured nodeMapper.poolName (PR 6c). Lets
//                        motifs target a non-default pool for an open()
//                        without re-binding the api subcomponent.
struct FileInfo {
    std::map<std::string, std::string> hints;
};

// ========================================================================
// FileDescriptor + FileHandle (PR 9)
// ========================================================================
//
// Pointer-typed handle. Pattern matches MessageRequestBase* in
// hermes/msgapi.h:66-71. The opaque struct is sized so out-of-tree
// motifs that only care about the handle identity don't need to know
// HadesNetworkIO::FileMetadata internals.
//
// fileId == 0 is the legacy/shim sentinel returned by the back-compat
// networkIORead/Write path; HadesNetworkIO::open() assigns ids >= 1
// monotonically via m_nextFileId. See pr-9-design.md §3 Q2 cross-check.
struct FileDescriptor {
    uint32_t    fileId  = 0;
    OpenMode    mode    = OpenMode::ReadOnly;
    std::string path;
};

using FileHandle = FileDescriptor*;

// StatusCallback delivers IOStatus (PR 9 file-handle ops). Separate
// typedef from the legacy `Callback` to avoid breaking PR 6/7/8 paths.
typedef std::function<void(IOStatus)> StatusCallback;

// ========================================================================
// IORequest -- opaque handle for non-blocking I/O (PR 10)
// ========================================================================
//
// Pointer-typed handle. Pattern matches Hermes::MP::MessageRequest at
// src/sst/elements/hermes/msgapi.h:66-71. HadesNetworkIO subclasses
// IORequestBase (as IORequestImpl) to attach implementation-private
// state (respKey, complete_slot atomic, per-request IOStatus, etc).
//
// Lifetime contract (mirrors MPI 4.1 §3.7.3 MPI_REQUEST_NULL semantics
// + L-1..L-5 invariants in docs/pr-10-design.md §3):
//   L-1  every IORequest returned by iread_at/iwrite_at is consumed by
//        exactly one wait/waitall. test() is non-consuming.
//   L-2  test() reads state without erasing; the request stays valid
//        until a subsequent wait/waitall.
//   L-3  cancel() is silent success (MPI 4.1 §4.8.4 + MPICH ROMIO
//        mpiu_greq.c:16-50 + OMPI common_ompio_request.c:63-76). No
//        Status::Cancelled enum value is introduced.
//   L-4  waitall fills statuses[] in REQUEST-ARRAY INDEX order, not
//        completion order, preceded by an acquire fence (MPICH
//        request_impl.c:750-780; OMPI req_wait.c:281-323).
//   L-5  testany/waitany on count==0 short-circuit: done=true, idx=-1
//        (MPI_UNDEFINED analogue), status zeroed (MPI 4.1 §4.7.5).
//
// After wait/waitall returns, the IORequest is invalid (analogue of
// MPI_REQUEST_NULL); motifs must not dereference it.
class IORequestBase {
  public:
    virtual ~IORequestBase() = default;
};

using IORequest = IORequestBase*;

// ========================================================================
// Interface (PR 6 + PR 9)
// ========================================================================
class Interface : public Hermes::Interface {
  public:
    // PR 6d: legacy status callback (status carried as int; cast via
    // static_cast<Status>(s) to inspect). Used by networkIORead/Write
    // shims for back-compat with PR 6/7/8 motifs.
    typedef std::function<void(int)> Callback;

    Interface( ComponentId_t id ) : Hermes::Interface(id) {}

    virtual ~Interface() = default;

    // --------------------------------------------------------------------
    // Legacy stripe-cyclic API (PR 6) -- preserved as back-compat shim.
    // --------------------------------------------------------------------
    // HadesNetworkIO::networkIORead/Write lazily open a single
    // '__legacy__' file at first call and route through the new
    // file-handle path with fileId=0. See PR 9 design §3 R-5.
    virtual void networkIORead(Vaddr dest, uint64_t offset, uint64_t length, Callback) { assert(0); }
    virtual void networkIOWrite(uint64_t offset, Vaddr src, uint64_t length, Callback) { assert(0); }

    // --------------------------------------------------------------------
    // File-handle API (PR 9)
    // --------------------------------------------------------------------
    // open(): allocate a FileHandle bound to (path, mode, info hints).
    // Sends Open ops to every storage NIC in the chosen stripe set;
    // callback fires once all open-ACKs have returned. On success,
    // IOStatus.bytes_completed carries the file's stripe count (number
    // of storage NICs in its stripe set).
    virtual void open(const std::string& path, OpenMode mode,
                      const FileInfo& info, FileHandle* outHandle,
                      StatusCallback callback) { (void)path; (void)mode;
                      (void)info; (void)outHandle; (void)callback; assert(0); }

    // close(): release the FileHandle and tell every storage NIC in the
    // stripe set to drop its m_fileSlots entry. Idempotent on already-
    // closed handles (returns OK with bytes_completed=0). After close,
    // any pending read_at/write_at against this handle is undefined --
    // motifs are responsible for ordering close after completions.
    virtual void close(FileHandle handle, StatusCallback callback) {
        (void)handle; (void)callback; assert(0); }

    // read_at(): pread-shaped read against a FileHandle at the given
    // absolute byte offset. Routes to the storage NIC that owns the
    // stripe slot for (offset, length). On short-read (offset+length >
    // file capacity), callback fires with Status::ShortRead and
    // bytes_completed < length.
    virtual void read_at(FileHandle handle, uint64_t offset,
                         Vaddr dest, uint64_t length,
                         StatusCallback callback) {
        (void)handle; (void)offset; (void)dest; (void)length;
        (void)callback; assert(0); }

    // write_at(): pwrite-shaped write. Mirrors read_at. NoSpace error
    // surfaces if offset+length > file capacity.
    virtual void write_at(FileHandle handle, uint64_t offset,
                          Vaddr src, uint64_t length,
                          StatusCallback callback) {
        (void)handle; (void)offset; (void)src; (void)length;
        (void)callback; assert(0); }

    // --------------------------------------------------------------------
    // Non-blocking file-handle API (PR 10)
    // --------------------------------------------------------------------
    // iread_at / iwrite_at: issue a non-blocking read/write at an
    // absolute byte offset; write *outReq with an opaque IORequest the
    // caller subsequently consumes via wait/waitall/test/testany/cancel.
    // The per-issue StatusCallback fires on local completion (telemetry
    // only; it does NOT consume the request). Mirrors MPI 4.1 §14.4.4
    // MPI_File_iread_at / MPI_File_iwrite_at signatures.
    virtual void iread_at(FileHandle handle, uint64_t offset,
                          Vaddr dest, uint64_t length,
                          IORequest* outReq, StatusCallback callback) {
        (void)handle; (void)offset; (void)dest; (void)length;
        (void)outReq; (void)callback; assert(0); }

    virtual void iwrite_at(FileHandle handle, uint64_t offset,
                           Vaddr src, uint64_t length,
                           IORequest* outReq, StatusCallback callback) {
        (void)handle; (void)offset; (void)src; (void)length;
        (void)outReq; (void)callback; assert(0); }

    // wait: block until a single IORequest completes; deliver its
    // IOStatus into *outStatus and fire callback (single-request
    // convenience wrapper over waitall(1, ...)). After callback fires
    // the IORequest is invalid (L-1 single-consume).
    virtual void wait(IORequest req, IOStatus* outStatus,
                      StatusCallback callback) {
        (void)req; (void)outStatus; (void)callback; assert(0); }

    // waitall: block until ALL n requests complete; fill statuses[] in
    // request-array INDEX order (L-4) and fire callback with an
    // aggregate IOStatus (sum of bytes_completed, first non-OK error
    // code). Pass statuses=nullptr for STATUSES_IGNORE fast path.
    // All requests in reqs[] become invalid after callback (L-1).
    virtual void waitall(int count, IORequest reqs[],
                         IOStatus statuses[],
                         StatusCallback callback) {
        (void)count; (void)reqs; (void)statuses; (void)callback; assert(0); }

    // test: non-consuming completion check. Sets *outDone to true iff
    // the request has completed; if done, copies status into *outStatus.
    // Caller still owes a wait/waitall to release the request (L-2).
    virtual void test(IORequest req, bool* outDone, IOStatus* outStatus,
                      StatusCallback callback) {
        (void)req; (void)outDone; (void)outStatus; (void)callback;
        assert(0); }

    // testany: non-consuming scan. On count==0, short-circuits with
    // done=true, idx=-1, status zeroed (L-5; MPI 4.1 §4.7.5).
    // Otherwise sets *outIdx to the first completed request's index
    // (in array order); if none completed, done=false, idx=-1.
    virtual void testany(int count, IORequest reqs[],
                         int* outIdx, bool* outDone,
                         IOStatus* outStatus,
                         StatusCallback callback) {
        (void)count; (void)reqs; (void)outIdx; (void)outDone;
        (void)outStatus; (void)callback; assert(0); }

    // cancel: best-effort, silent-success per MPI 4.1 §4.8.4 (cancel
    // for I/O deliberately undefined) + MPICH ROMIO mpiu_greq.c:16-50
    // + OMPI common_ompio_request.c:63-76. Returns Status::OK
    // unconditionally for valid handles, Status::BadFileHandle for
    // unknown ones. Does NOT touch the in-flight wire packet; the
    // request still completes via the normal ACK path. L-3 invariant.
    virtual void cancel(IORequest req, StatusCallback callback) {
        (void)req; (void)callback; assert(0); }
};

} // namespace NetworkIO
} // namespace Hermes
} // namespace SST
