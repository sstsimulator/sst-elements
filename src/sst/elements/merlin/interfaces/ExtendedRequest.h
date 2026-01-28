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

#ifndef COMPONENTS_MERLIN_EXTENDEDREQUEST_H
#define COMPONENTS_MERLIN_EXTENDEDREQUEST_H

#include "sst/core/interfaces/simpleNetwork.h"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <deque>

namespace SST {
namespace Merlin {

// Common metadata structures used by plugins

// Source Routing metadata
struct SourceRoutingMetadata {
    std::deque<int> path = {};

    SourceRoutingMetadata() {}
    SourceRoutingMetadata(const std::deque<int>& p) : path(p) {}

    void serialize_order(SST::Core::Serialization::serializer& ser) {
        SST_SER(path);
    }
};

// Reorder metadata
struct ReorderMetadata {
    uint32_t seq_number;

    ReorderMetadata() : seq_number(0) {}
    ReorderMetadata(uint32_t seq) : seq_number(seq) {}

    void serialize_order(SST::Core::Serialization::serializer& ser) {
        SST_SER(seq_number);
    }
};

// Variant type for all supported metadata types
// Add new metadata types here as needed
using MetadataVariant = std::variant<
    std::monostate,  // Empty/uninitialized state
    SourceRoutingMetadata,
    ReorderMetadata
>;

// Extended Request that plugins can attach metadata to
class ExtendedRequest : public SST::Interfaces::SimpleNetwork::Request {
private:
    // Map from plugin name/key to metadata (variant-based for serialization)
    std::map<std::string, MetadataVariant> metadata;

    ImplementSerializable(SST::Merlin::ExtendedRequest)

public:
    ExtendedRequest() : Request() {}

    ExtendedRequest(SST::Interfaces::SimpleNetwork::nid_t dest,
        SST::Interfaces::SimpleNetwork::nid_t src, size_t size_in_bits,
                    bool head, bool tail, Event* payload = nullptr) :
        Request(dest, src, size_in_bits, head, tail, payload) {}

    // Create ExtendedRequest from a base Request
    ExtendedRequest(Request* req) :
        Request(req->dest, req->src, req->size_in_bits, req->head, req->tail)
    {
        givePayload(req->takePayload());
        trace = req->getTraceType();
        traceID = req->getTraceID();

        // If req is already an ExtendedRequest, copy its metadata
        ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
        if (ext_req) {
            // Copy metadata (variant handles copying automatically)
            metadata = ext_req->metadata;
        }
    }

    ~ExtendedRequest() {}

    // Set plugin-specific metadata
    template<typename T>
    void setMetadata(const std::string& key, const T& data) {
        metadata[key] = data;
    }

    // Get plugin-specific metadata
    // Returns true if metadata exists and was retrieved, false otherwise
    template<typename T>
    bool getMetadata(const std::string& key, T& data) const {
        auto it = metadata.find(key);
        if (it == metadata.end()) return false;

        // Try to get the value from the variant
        if (auto* val = std::get_if<T>(&it->second)) {
            data = *val;
            return true;
        }
        return false;
    }

    // Get a const pointer to plugin-specific metadata without copying
    // Returns nullptr if metadata doesn't exist or type mismatch
    template<typename T>
    const T* getMetadataPtr(const std::string& key) const {
        auto it = metadata.find(key);
        if (it == metadata.end()) return nullptr;

        // Return pointer to value in variant
        return std::get_if<T>(&it->second);
    }

    // Get a mutable pointer to plugin-specific metadata without copying
    // Returns nullptr if metadata doesn't exist or type mismatch
    template<typename T>
    T* getMetadataPtr(const std::string& key) {
        auto it = metadata.find(key);
        if (it == metadata.end()) return nullptr;

        // Return pointer to value in variant
        return std::get_if<T>(&it->second);
    }

    // Check if metadata exists for a given key
    bool hasMetadata(const std::string& key) const {
        return metadata.find(key) != metadata.end();
    }

    // Remove metadata for a given key
    void removeMetadata(const std::string& key) {
        metadata.erase(key);
    }

    // Clear all metadata
    void clearMetadata() {
        metadata.clear();
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Request::serialize_order(ser);

        // SST serializer now has built-in support for std::variant and std::map
        // Use SST_SER macro for automatic serialization
        SST_SER(metadata);
    }
};

} // namespace Merlin
} // namespace SST

#endif
