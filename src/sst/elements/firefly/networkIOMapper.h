// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_FIREFLY_NETWORK_IO_MAPPER
#define _H_FIREFLY_NETWORK_IO_MAPPER

#include <sst/core/module.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace SST {
namespace Firefly {

// NetworkIOMapper -- pluggable storage-node selection policy used by
// HadesNetworkIO. Each concrete mapper is given an offset (and optionally a
// length and a respKey) and returns the global NID that should service the
// request.
//
// Mirrors the FamNodeMapper module API (see shmem/famNodeMapper.h) so
// it is configured via params.get_scoped_params("nodeMapper") and loaded
// with loadModule<NetworkIOMapper>().
class NetworkIOMapper : public SST::Module {
  public:
    SST_ELI_REGISTER_MODULE_API(SST::Firefly::NetworkIOMapper)

    // Return the global NID that should service the request.
    // Mappers that don't need length or respKey may ignore them.
    //
    // NOTE (PR 6a, 2026-05-13): this API is retained for back-compat with
    // the legacy single-pool dispatch path in `HadesNetworkIO`. New work
    // (per-job storage pools, see `pr-6-design.md` rev 2) calls
    // `calcStripeIdx()` below and resolves the NID via a pool descriptor
    // outside the mapper. Mappers continue to implement `calcNode()` until
    // PR 6c migrates the dispatch site.
    virtual int calcNode(uint64_t offset, uint64_t length, uint32_t respKey) = 0;

    // Return the bare stripe index (no `% numNodes()`). HadesNetworkIO is
    // expected to apply the modulo against whatever pool descriptor it has
    // (single-pool nidList today, SharedArray pool in PR 6c onwards).
    //
    // PR 6c (this PR): this is now the *canonical* dispatch path. The
    // legacy `calcNode()` API is deprecated and will be removed in a
    // future release. The base-class fallback below previously approximated
    // `calcNode(...) % numNodes()` and returned 0, which silently routed
    // un-migrated out-of-tree mappers' traffic to stripe 0. Per the
    // Oracle review of PR 6c (see `pr-6-design.md` §3.3 update), we now
    // `fatal()` here instead so that any mapper which has not overridden
    // `calcStripeIdx()` fails loudly at first use rather than corrupting
    // the storage dispatch silently.
    //
    // All three in-tree mappers (Block, RR, Hash) override this method.
    // Out-of-tree mappers that only override `calcNode()` must migrate.
    virtual size_t calcStripeIdx(uint64_t offset, uint64_t length) {
        (void)offset;
        (void)length;
        if (m_dbg) m_dbg->fatal(CALL_INFO, -1,
            "NetworkIOMapper::calcStripeIdx() base-class fallback hit -- "
            "loaded mapper must override calcStripeIdx() (PR 6c removed "
            "the legacy calcNode-based approximation; see pr-6-design.md "
            "§3.3)\n");
        std::abort();
    }

    // Number of distinct storage NIDs in the configured list. Used by
    // HadesNetworkIO for sanity checks and debug logging.
    virtual size_t numNodes() const = 0;

    virtual void setDbg(Output* output) {
        m_dbg = output;
    }

  protected:
    Output* m_dbg = nullptr;

    // Helper shared by concrete mappers: parse "2,3,5,7" -> {2,3,5,7}.
    // Empty / all-whitespace input returns an empty vector.
    static std::vector<int> parseNidList(const std::string& csv) {
        std::vector<int> out;
        std::stringstream ss(csv);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            size_t b = tok.find_first_not_of(" \t");
            size_t e = tok.find_last_not_of(" \t");
            if (b == std::string::npos) continue;
            tok = tok.substr(b, e - b + 1);
            if (tok.empty()) continue;
            out.push_back(std::stoi(tok));
        }
        return out;
    }
};

// Block striping: offset/bytesPerNode picks the NID. This is the closest
// match to the existing (deprecated) HadesNetworkIO behaviour.
class Block_NetworkIOMapper : public NetworkIOMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        Block_NetworkIOMapper,
        "firefly",
        "Block_NetworkIOMapper",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Block-striped NetworkIO node mapper (offset/bytesPerNode % nidList.size())",
        SST::Firefly::NetworkIOMapper
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"ioNidList",    "Comma-separated list of global NIDs that host I/O nodes", ""},
        {"bytesPerNode", "Stripe granularity (each NID owns this many bytes)",       "1GiB"},
    )

    Block_NetworkIOMapper(Params& params) {
        m_nidList = parseNidList(params.find<std::string>("ioNidList", ""));
        m_bytesPerNode = (uint64_t) params.find<SST::UnitAlgebra>("bytesPerNode", "1GiB").getRoundedValue();
        if (m_bytesPerNode == 0) m_bytesPerNode = 1;
    }

    int calcNode(uint64_t offset, uint64_t /*length*/, uint32_t /*respKey*/) override {
        assert(!m_nidList.empty());
        size_t idx = (offset / m_bytesPerNode) % m_nidList.size();
        if (m_dbg) {
            m_dbg->debug(CALL_INFO, 3, 0,
                         "offset=%" PRIu64 " bytesPerNode=%" PRIu64 " idx=%zu nid=%d\n",
                         offset, m_bytesPerNode, idx, m_nidList[idx]);
        }
        return m_nidList[idx];
    }

    size_t calcStripeIdx(uint64_t offset, uint64_t /*length*/) override {
        assert(!m_nidList.empty());
        return (offset / m_bytesPerNode) % m_nidList.size();
    }

    size_t numNodes() const override { return m_nidList.size(); }

  private:
    std::vector<int> m_nidList;
    uint64_t         m_bytesPerNode = 0;
};

// Round-robin striping at blockSize granularity: every blockSize bytes
// rotates to the next NID. Fine-grained interleave, good for spreading
// load on small-block workloads.
class RR_NetworkIOMapper : public NetworkIOMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        RR_NetworkIOMapper,
        "firefly",
        "RR_NetworkIOMapper",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Round-robin NetworkIO node mapper (offset/blockSize % nidList.size())",
        SST::Firefly::NetworkIOMapper
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"ioNidList", "Comma-separated list of global NIDs that host I/O nodes", ""},
        {"blockSize", "Round-robin stripe block size",                            "4KiB"},
    )

    RR_NetworkIOMapper(Params& params) {
        m_nidList = parseNidList(params.find<std::string>("ioNidList", ""));
        m_blockSize = (uint64_t) params.find<SST::UnitAlgebra>("blockSize", "4KiB").getRoundedValue();
        if (m_blockSize == 0) m_blockSize = 1;
    }

    int calcNode(uint64_t offset, uint64_t /*length*/, uint32_t /*respKey*/) override {
        assert(!m_nidList.empty());
        size_t idx = (offset / m_blockSize) % m_nidList.size();
        if (m_dbg) {
            m_dbg->debug(CALL_INFO, 3, 0,
                         "offset=%" PRIu64 " blockSize=%" PRIu64 " idx=%zu nid=%d\n",
                         offset, m_blockSize, idx, m_nidList[idx]);
        }
        return m_nidList[idx];
    }

    size_t calcStripeIdx(uint64_t offset, uint64_t /*length*/) override {
        assert(!m_nidList.empty());
        return (offset / m_blockSize) % m_nidList.size();
    }

    size_t numNodes() const override { return m_nidList.size(); }

  private:
    std::vector<int> m_nidList;
    uint64_t         m_blockSize = 0;
};

// Hash striping: hash(offset/blockSize) % nidList.size(). Useful when
// access patterns are correlated with offset (e.g. consecutive ranks
// hammering adjacent offsets); breaks that correlation.
class Hash_NetworkIOMapper : public NetworkIOMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        Hash_NetworkIOMapper,
        "firefly",
        "Hash_NetworkIOMapper",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Hash-striped NetworkIO node mapper (hash(offset/blockSize) % nidList.size())",
        SST::Firefly::NetworkIOMapper
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"ioNidList", "Comma-separated list of global NIDs that host I/O nodes", ""},
        {"blockSize", "Hash bucket size",                                         "4KiB"},
    )

    Hash_NetworkIOMapper(Params& params) {
        m_nidList = parseNidList(params.find<std::string>("ioNidList", ""));
        m_blockSize = (uint64_t) params.find<SST::UnitAlgebra>("blockSize", "4KiB").getRoundedValue();
        if (m_blockSize == 0) m_blockSize = 1;
    }

    int calcNode(uint64_t offset, uint64_t /*length*/, uint32_t /*respKey*/) override {
        assert(!m_nidList.empty());
        uint64_t bucket = offset / m_blockSize;
        size_t idx = std::hash<uint64_t>{}(bucket) % m_nidList.size();
        if (m_dbg) {
            m_dbg->debug(CALL_INFO, 3, 0,
                         "offset=%" PRIu64 " blockSize=%" PRIu64 " bucket=%" PRIu64 " idx=%zu nid=%d\n",
                         offset, m_blockSize, bucket, idx, m_nidList[idx]);
        }
        return m_nidList[idx];
    }

    size_t calcStripeIdx(uint64_t offset, uint64_t /*length*/) override {
        assert(!m_nidList.empty());
        return std::hash<uint64_t>{}(offset / m_blockSize) % m_nidList.size();
    }

    size_t numNodes() const override { return m_nidList.size(); }

  private:
    std::vector<int> m_nidList;
    uint64_t         m_blockSize = 0;
};

} // namespace Firefly
} // namespace SST

#endif
